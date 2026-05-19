/*
 * AnalogPinScanner.ino
 * Minimal ESP32 analog signal scanner for TFT boards.
 *
 * Keep this sketch broad and easy to tweak. Device-specific knowledge lives in
 * the small settings block below; the scanner itself only looks for movement.
 */

#include <TFT_eSPI.h>

// ===== DEVICE SETTINGS =====

#define PROFILE_GENERIC 0
#define PROFILE_ESP32_2432S028 1

#ifndef PIN_SCANNER_PROFILE
#define PIN_SCANNER_PROFILE PROFILE_GENERIC
#endif

#if PIN_SCANNER_PROFILE == PROFILE_ESP32_2432S028
#define BACKLIGHT_PIN 21
#else
#define BACKLIGHT_PIN 21
#endif

const char* deviceName() {
#if PIN_SCANNER_PROFILE == PROFILE_ESP32_2432S028
  return "ESP32-2432S028";
#else
  return "Generic ESP32";
#endif
}

const char* footerHint() {
#if PIN_SCANNER_PROFILE == PROFILE_ESP32_2432S028
  return "Known working: IO35, IO22, IO27. Auto-sorts.";
#else
  return "Yellow = most signal movement. Rails are less useful.";
#endif
}

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define ADC_MAX_VALUE 4095
#define HOT_MOVEMENT_MIN 20
#define SORT_INTERVAL_MS 3000
#define SORT_HYSTERESIS 8

#define COLOR_BG 0x0000
#define COLOR_TEXT 0xFFFF
#define COLOR_DIM 0x7BEF
#define COLOR_GRID 0x2104
#define COLOR_BAR 0x07FF
#define COLOR_HOT 0xFBE0
#define COLOR_WARN 0xF800

TFT_eSPI tft = TFT_eSPI();

struct AnalogPin {
  const char* label;
  uint8_t pin;
  int value;
  int minValue;
  int maxValue;
  int movement;
};

AnalogPin pins[] = {
#if PIN_SCANNER_PROFILE == PROFILE_ESP32_2432S028
  {"P3  IO35", 35, 0, ADC_MAX_VALUE, 0, 0},
  {"IO22", 22, 0, ADC_MAX_VALUE, 0, 0},
  {"CN1 IO27", 27, 0, ADC_MAX_VALUE, 0, 0},
  {"LDR IO34", 34, 0, ADC_MAX_VALUE, 0, 0},
  {"IO32", 32, 0, ADC_MAX_VALUE, 0, 0},
  {"IO33", 33, 0, ADC_MAX_VALUE, 0, 0},
#else
  {"IO32", 32, 0, ADC_MAX_VALUE, 0, 0},
  {"IO33", 33, 0, ADC_MAX_VALUE, 0, 0},
  {"IO34", 34, 0, ADC_MAX_VALUE, 0, 0},
  {"IO35", 35, 0, ADC_MAX_VALUE, 0, 0},
  {"IO36", 36, 0, ADC_MAX_VALUE, 0, 0},
  {"IO39", 39, 0, ADC_MAX_VALUE, 0, 0},
#endif
};

const int pinCount = sizeof(pins) / sizeof(pins[0]);

void drawStaticUI();
void updateReadings();
void drawReadings();
void drawPinRow(int index, bool hot);
void maybeSortPins();
int hottestPinIndex();
bool isRailed(int value);

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);

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
  static unsigned long lastSerial = 0;

  updateReadings();
  maybeSortPins();

  if (millis() - lastDraw >= 100) {
    lastDraw = millis();
    drawReadings();
  }

  if (millis() - lastSerial >= 500) {
    lastSerial = millis();
    for (int i = 0; i < pinCount; i++) {
      Serial.printf("%s=%4d d%4d%s",
                    pins[i].label,
                    pins[i].value,
                    pins[i].movement,
                    i == pinCount - 1 ? "\n" : "  ");
    }
  }
}

void drawStaticUI() {
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(10, 8);
  tft.print("Analog Pin Scanner");

  tft.setTextSize(1);
  tft.setTextColor(COLOR_DIM, COLOR_BG);
  tft.setCursor(10, 32);
  tft.print(deviceName());
  tft.print("  raw ADC 0..4095");

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

    // Slowly relax the observed range so old movement does not dominate.
    if (pins[i].minValue < pins[i].value) pins[i].minValue++;
    if (pins[i].maxValue > pins[i].value) pins[i].maxValue--;
  }
}

void drawReadings() {
  int hotIndex = hottestPinIndex();

  for (int i = 0; i < pinCount; i++) {
    drawPinRow(i, i == hotIndex && pins[i].movement > HOT_MOVEMENT_MIN);
  }

  tft.setTextSize(1);
  tft.setTextColor(COLOR_DIM, COLOR_BG);
  tft.fillRect(10, 224, 300, 12, COLOR_BG);
  tft.setCursor(10, 224);
  tft.print(footerHint());
}

void maybeSortPins() {
  static unsigned long lastSort = 0;

  if (millis() - lastSort < SORT_INTERVAL_MS) return;
  lastSort = millis();

  for (int pass = 0; pass < pinCount - 1; pass++) {
    for (int i = 0; i < pinCount - 1 - pass; i++) {
      if (pins[i + 1].movement > pins[i].movement + SORT_HYSTERESIS) {
        AnalogPin temp = pins[i];
        pins[i] = pins[i + 1];
        pins[i + 1] = temp;
      }
    }
  }
}

void drawPinRow(int index, bool hot) {
  const int y = 58 + index * 27;
  const int barX = 82;
  const int barY = y + 3;
  const int barW = 150;
  const int barH = 14;
  const int value = constrain(pins[index].value, 0, ADC_MAX_VALUE);
  const int fillW = map(value, 0, ADC_MAX_VALUE, 0, barW);
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

  if (isRailed(value)) {
    tft.setTextColor(COLOR_WARN, COLOR_BG);
    tft.setCursor(292, y + 5);
    tft.print("rail");
  }
}

int hottestPinIndex() {
  int hotIndex = 0;
  for (int i = 1; i < pinCount; i++) {
    if (pins[i].movement > pins[hotIndex].movement) hotIndex = i;
  }
  return hotIndex;
}

bool isRailed(int value) {
  return value < 20 || value > ADC_MAX_VALUE - 20;
}
