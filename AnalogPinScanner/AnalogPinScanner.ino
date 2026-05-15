/*
 * AnalogPinScanner.ino
 * Simple CYD diagnostic sketch for finding which ESP32 GPIO has a PulseSensor.
 *
 * It reads raw ADC values directly, without the PulseSensor library.
 * Watch the bars while touching/moving the PulseSensor lead. The active pin
 * should show the largest movement and a changing raw value.
 */

#include <TFT_eSPI.h>

#define BACKLIGHT 21
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define COLOR_BG 0x0000
#define COLOR_TEXT 0xFFFF
#define COLOR_DIM 0x7BEF
#define COLOR_GRID 0x2104
#define COLOR_BAR 0x07FF
#define COLOR_HOT 0xFBE0
#define COLOR_WARN 0xF800

TFT_eSPI tft = TFT_eSPI();

void setup();
void loop();
void drawStaticUI();
void updateReadings();
void drawReadings();
void drawPinRow(int index, bool hot);

struct AnalogPin {
  const char* label;
  uint8_t pin;
  int value;
  int minValue;
  int maxValue;
  int movement;
};

AnalogPin pins[] = {
  {"IO32", 32, 0, 4095, 0, 0},
  {"IO33", 33, 0, 4095, 0, 0},
  {"IO34", 34, 0, 4095, 0, 0},
  {"IO35", 35, 0, 4095, 0, 0},
  {"IO36", 36, 0, 4095, 0, 0},
  {"IO39", 39, 0, 4095, 0, 0},
};

const int pinCount = sizeof(pins) / sizeof(pins[0]);

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, HIGH);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  for (int i = 0; i < pinCount; i++) {
    pinMode(pins[i].pin, INPUT);
  }

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(COLOR_BG);
  drawStaticUI();
}

void loop() {
  static unsigned long lastDraw = 0;

  updateReadings();

  if (millis() - lastDraw >= 100) {
    lastDraw = millis();
    drawReadings();

    Serial.printf("32=%4d 33=%4d 34=%4d 35=%4d 36=%4d 39=%4d\n",
                  pins[0].value, pins[1].value, pins[2].value,
                  pins[3].value, pins[4].value, pins[5].value);
  }
}

void drawStaticUI() {
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(10, 8);
  tft.print("CYD Analog Pin Scanner");

  tft.setTextSize(1);
  tft.setTextColor(COLOR_DIM, COLOR_BG);
  tft.setCursor(10, 32);
  tft.print("Move/touch sensor. Yellow row = most movement.");

  tft.drawFastHLine(0, 48, SCREEN_WIDTH, COLOR_GRID);
}

void updateReadings() {
  for (int i = 0; i < pinCount; i++) {
    long total = 0;
    for (int sample = 0; sample < 8; sample++) {
      total += analogRead(pins[i].pin);
      delayMicroseconds(150);
    }

    pins[i].value = total / 8;

    if (pins[i].value < pins[i].minValue) pins[i].minValue = pins[i].value;
    if (pins[i].value > pins[i].maxValue) pins[i].maxValue = pins[i].value;

    pins[i].movement = pins[i].maxValue - pins[i].minValue;

    // Slowly relax the observed range so old movement does not dominate forever.
    if (pins[i].minValue < pins[i].value) pins[i].minValue++;
    if (pins[i].maxValue > pins[i].value) pins[i].maxValue--;
  }
}

void drawReadings() {
  int hotIndex = 0;
  for (int i = 1; i < pinCount; i++) {
    if (pins[i].movement > pins[hotIndex].movement) hotIndex = i;
  }

  for (int i = 0; i < pinCount; i++) {
    drawPinRow(i, i == hotIndex && pins[i].movement > 20);
  }

  tft.setTextSize(1);
  tft.setTextColor(COLOR_DIM, COLOR_BG);
  tft.fillRect(10, 224, 300, 12, COLOR_BG);
  tft.setCursor(10, 224);
  tft.print("PulseSensor signal is usually a wiggling mid-range value.");
}

void drawPinRow(int index, bool hot) {
  const int y = 58 + index * 27;
  const int barX = 82;
  const int barY = y + 3;
  const int barW = 150;
  const int barH = 14;
  const int value = constrain(pins[index].value, 0, 4095);
  const int fillW = map(value, 0, 4095, 0, barW);
  const uint16_t rowColor = hot ? COLOR_HOT : COLOR_TEXT;
  const uint16_t barColor = hot ? COLOR_HOT : COLOR_BAR;

  tft.fillRect(0, y - 2, SCREEN_WIDTH, 23, COLOR_BG);

  tft.setTextSize(1);
  tft.setTextColor(rowColor, COLOR_BG);
  tft.setCursor(10, y + 5);
  tft.print(pins[index].label);

  tft.drawRect(barX, barY, barW, barH, COLOR_GRID);
  tft.fillRect(barX + 1, barY + 1, max(0, fillW - 2), barH - 2, barColor);

  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setCursor(244, y + 1);
  tft.printf("%4d", value);

  tft.setTextColor(hot ? COLOR_HOT : COLOR_DIM, COLOR_BG);
  tft.setCursor(244, y + 12);
  tft.printf("d%4d", pins[index].movement);

  if (value < 20 || value > 4075) {
    tft.setTextColor(COLOR_WARN, COLOR_BG);
    tft.setCursor(292, y + 5);
    tft.print("rail");
  }
}
