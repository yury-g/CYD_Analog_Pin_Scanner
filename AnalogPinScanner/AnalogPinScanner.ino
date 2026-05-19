/*
 * AnalogPinScanner.ino
 * CYD connector diagnostic sketch for the ESP32-2432S028.
 *
 * This reads raw connector pins directly, without PulseSensorPlayground.
 * Use it to test the P3 and CN1 pigtails before changing the dashboard.
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
#define COLOR_OK 0x07E0

TFT_eSPI tft = TFT_eSPI();

enum RowKind {
  ROW_ANALOG,
  ROW_DIGITAL,
  ROW_NOTE
};

struct ScanRow {
  const char* label;
  const char* detail;
  uint8_t pin;
  RowKind kind;
  int value;
  int minValue;
  int maxValue;
  int movement;
};

ScanRow rows[] = {
  {"P3  IO35", "ADC1 signal", 35, ROW_ANALOG, 0, 4095, 0, 0},
  {"CN1 IO27", "ADC2 signal", 27, ROW_ANALOG, 0, 4095, 0, 0},
  {"IO34 LDR", "onboard ADC", 34, ROW_ANALOG, 0, 4095, 0, 0},
  {"IO22", "digital only", 22, ROW_DIGITAL, 0, 0, 1, 0},
  {"P3  IO21", "backlight: do not use", 21, ROW_NOTE, 1, 0, 1, 0},
  {"CN1 3V3", "power rail + GND", 255, ROW_NOTE, 1, 0, 1, 0},
};

const int rowCount = sizeof(rows) / sizeof(rows[0]);

void drawStaticUI();
void updateReadings();
void drawReadings();
void drawRow(int index, bool hot);
void drawAnalogRow(const ScanRow& row, int y, bool hot);
void drawDigitalRow(const ScanRow& row, int y);
void drawNoteRow(const ScanRow& row, int y);

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, HIGH);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  for (int i = 0; i < rowCount; i++) {
    if (rows[i].kind == ROW_ANALOG) {
      pinMode(rows[i].pin, INPUT);
    } else if (rows[i].kind == ROW_DIGITAL) {
      pinMode(rows[i].pin, INPUT);
    }
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

  if (millis() - lastDraw >= 100) {
    lastDraw = millis();
    drawReadings();
  }

  if (millis() - lastSerial >= 500) {
    lastSerial = millis();
    Serial.printf("P3_IO35=%4d d%4d  CN1_IO27=%4d d%4d  IO34=%4d d%4d  IO22=%d\n",
                  rows[0].value, rows[0].movement,
                  rows[1].value, rows[1].movement,
                  rows[2].value, rows[2].movement,
                  rows[3].value);
  }
}

void drawStaticUI() {
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(10, 8);
  tft.print("CYD Connector Scanner");

  tft.setTextSize(1);
  tft.setTextColor(COLOR_DIM, COLOR_BG);
  tft.setCursor(10, 32);
  tft.print("Pulse signal should wiggle mid-range. Yellow = most movement.");

  tft.drawFastHLine(0, 48, SCREEN_WIDTH, COLOR_GRID);
}

void updateReadings() {
  for (int i = 0; i < rowCount; i++) {
    if (rows[i].kind == ROW_ANALOG) {
      long total = 0;
      for (int sample = 0; sample < 8; sample++) {
        total += analogRead(rows[i].pin);
        delayMicroseconds(150);
      }

      rows[i].value = total / 8;
      if (rows[i].value < rows[i].minValue) rows[i].minValue = rows[i].value;
      if (rows[i].value > rows[i].maxValue) rows[i].maxValue = rows[i].value;
      rows[i].movement = rows[i].maxValue - rows[i].minValue;

      // Slowly relax the observed range so old movement does not dominate.
      if (rows[i].minValue < rows[i].value) rows[i].minValue++;
      if (rows[i].maxValue > rows[i].value) rows[i].maxValue--;
    } else if (rows[i].kind == ROW_DIGITAL) {
      rows[i].value = digitalRead(rows[i].pin);
      rows[i].movement = 0;
    }
  }
}

void drawReadings() {
  int hotIndex = 0;
  for (int i = 1; i < rowCount; i++) {
    if (rows[i].kind == ROW_ANALOG && rows[i].movement > rows[hotIndex].movement) {
      hotIndex = i;
    }
  }

  for (int i = 0; i < rowCount; i++) {
    drawRow(i, i == hotIndex && rows[i].kind == ROW_ANALOG && rows[i].movement > 20);
  }

  tft.setTextSize(1);
  tft.setTextColor(COLOR_DIM, COLOR_BG);
  tft.fillRect(10, 224, 300, 12, COLOR_BG);
  tft.setCursor(10, 224);
  tft.print("Try Pulse purple on IO35, then IO27. Avoid IO21.");
}

void drawRow(int index, bool hot) {
  const int y = 58 + index * 27;
  tft.fillRect(0, y - 2, SCREEN_WIDTH, 23, COLOR_BG);

  if (rows[index].kind == ROW_ANALOG) {
    drawAnalogRow(rows[index], y, hot);
  } else if (rows[index].kind == ROW_DIGITAL) {
    drawDigitalRow(rows[index], y);
  } else {
    drawNoteRow(rows[index], y);
  }
}

void drawAnalogRow(const ScanRow& row, int y, bool hot) {
  const int barX = 92;
  const int barY = y + 3;
  const int barW = 136;
  const int barH = 14;
  const int value = constrain(row.value, 0, 4095);
  const int fillW = map(value, 0, 4095, 0, barW);
  const uint16_t rowColor = hot ? COLOR_HOT : COLOR_TEXT;
  const uint16_t barColor = hot ? COLOR_HOT : COLOR_BAR;

  tft.setTextSize(1);
  tft.setTextColor(rowColor, COLOR_BG);
  tft.setCursor(10, y + 1);
  tft.print(row.label);
  tft.setTextColor(COLOR_DIM, COLOR_BG);
  tft.setCursor(10, y + 12);
  tft.print(row.detail);

  tft.drawRect(barX, barY, barW, barH, COLOR_GRID);
  tft.fillRect(barX + 1, barY + 1, max(0, fillW - 2), barH - 2, barColor);

  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setCursor(238, y + 1);
  tft.printf("%4d", value);

  tft.setTextColor(hot ? COLOR_HOT : COLOR_DIM, COLOR_BG);
  tft.setCursor(238, y + 12);
  tft.printf("d%4d", row.movement);

  if (value < 20 || value > 4075) {
    tft.setTextColor(COLOR_WARN, COLOR_BG);
    tft.setCursor(288, y + 5);
    tft.print("rail");
  }
}

void drawDigitalRow(const ScanRow& row, int y) {
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setCursor(10, y + 1);
  tft.print(row.label);
  tft.setTextColor(COLOR_DIM, COLOR_BG);
  tft.setCursor(10, y + 12);
  tft.print(row.detail);

  tft.setTextColor(row.value ? COLOR_OK : COLOR_WARN, COLOR_BG);
  tft.setCursor(108, y + 5);
  tft.print(row.value ? "HIGH" : "LOW ");

  tft.setTextColor(COLOR_DIM, COLOR_BG);
  tft.setCursor(174, y + 5);
  tft.print("not ADC");
}

void drawNoteRow(const ScanRow& row, int y) {
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setCursor(10, y + 1);
  tft.print(row.label);
  tft.setTextColor(COLOR_DIM, COLOR_BG);
  tft.setCursor(10, y + 12);
  tft.print(row.detail);
}
