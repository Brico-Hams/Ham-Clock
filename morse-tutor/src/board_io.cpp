#include "board_io.h"
#include <Arduino.h>
#include <Preferences.h>
#include <SD.h>
#if BRICO_PANEL == 778932
#include <esp_timer.h>
#endif
#include "ControlLogic.h"

namespace board {
TFT_eSPI display;
namespace {
SPIClass cardBus(VSPI);
struct Calibration { int xStart, xEnd, yStart, yEnd; bool swap; };
Calibration calibration = {};
bool wasDown = false, previousTone = false;
int previousPitch = 0;
uint32_t polledAt = 0;
Touch lastTouch;
#if BRICO_PANEL == 778932
esp_timer_handle_t audioTimer = nullptr;
bool highPhase = false;
void dacTick(void*) {
  highPhase = !highPhase;
  dacWrite(25, highPhase ? 144 : 112);
  dacWrite(26, highPhase ? 144 : 112);
}
#endif

uint8_t transfer(uint8_t value) {
  uint8_t result = 0;
  for (int bit = 7; bit >= 0; --bit) {
    digitalWrite(TOUCH_MOSI, (value >> bit) & 1);
    digitalWrite(TOUCH_SCLK, HIGH);
    result = (result << 1) | digitalRead(TOUCH_MISO);
    digitalWrite(TOUCH_SCLK, LOW);
  }
  return result;
}
uint16_t axis(uint8_t command) {
  transfer(command);
  uint16_t high = transfer(0);
  return ((high << 8) | transfer(0)) >> 3;
}
bool rawTouch(uint16_t& x, uint16_t& y) {
#if TOUCH_SCLK == TFT_SCLK
  if (display.getTouchRawZ() < 300) return false;
  display.getTouchRaw(&x, &y);
  return display.getTouchRawZ() >= 300;
#else
  if (digitalRead(TOUCH_IRQ) == HIGH) return false;
  digitalWrite(TOUCH_CS, LOW);
  delayMicroseconds(2);
  uint32_t sx = 0, sy = 0;
  for (int i = 0; i < 4; ++i) { sx += axis(0xd0); sy += axis(0x90); }
  digitalWrite(TOUCH_CS, HIGH);
  x = sx / 4; y = sy / 4;
  return digitalRead(TOUCH_IRQ) == LOW && x > 100 && x < 4000 && y > 100 && y < 4000;
#endif
}

void calibrate() {
  uint16_t x[3], y[3];
  const int targets[][2] = {{24, 24}, {DISPLAY_W - 25, 24}, {24, DISPLAY_H - 25}};
  while (true) {
    for (int i = 0; i < 3; ++i) {
      display.fillScreen(TFT_BLACK);
      display.setTextColor(TFT_ORANGE);
      display.setTextSize(1);
      display.setCursor(55, DISPLAY_H / 2);
      display.print("Bricohams: toca las 3 cruces");
      display.drawLine(targets[i][0] - 12, targets[i][1], targets[i][0] + 12, targets[i][1], TFT_WHITE);
      display.drawLine(targets[i][0], targets[i][1] - 12, targets[i][0], targets[i][1] + 12, TFT_WHITE);
      uint16_t releaseX, releaseY;
      while (rawTouch(releaseX, releaseY)) delay(1);
      delay(150);
      while (!rawTouch(x[i], y[i])) delay(1);
      while (rawTouch(releaseX, releaseY)) delay(1);
    }
    calibration.swap = abs(int(y[1]) - y[0]) > abs(int(x[1]) - x[0]);
    calibration.xStart = calibration.swap ? y[0] : x[0];
    calibration.xEnd = calibration.swap ? y[1] : x[1];
    calibration.yStart = calibration.swap ? x[0] : y[0];
    calibration.yEnd = calibration.swap ? x[2] : y[2];
    if (abs(calibration.xEnd - calibration.xStart) > 500 &&
        abs(calibration.yEnd - calibration.yStart) > 500) return;
    Serial.println("Touch: invalid calibration; repeat targets");
  }
}
}

void begin() {
  pinMode(TOUCH_CS, OUTPUT); digitalWrite(TOUCH_CS, HIGH);
  pinMode(BRICO_SD_CS, OUTPUT); digitalWrite(BRICO_SD_CS, HIGH);
  display.init(); display.setRotation(1); display.setTextWrap(false);
  ledcSetup(BRICO_BACKLIGHT_CHANNEL, 5000, 8);
  ledcAttachPin(TFT_BL, BRICO_BACKLIGHT_CHANNEL);
  setBrightness(80);
#if BRICO_PANEL == 778932
  pinMode(BRICO_AUDIO_ENABLE, OUTPUT); digitalWrite(BRICO_AUDIO_ENABLE, LOW);
  dacWrite(25, 128); dacWrite(26, 128);
  esp_timer_create_args_t timer = {};
  timer.callback = dacTick; timer.name = "cw-dac";
  if (esp_timer_create(&timer, &audioTimer) != ESP_OK) {
    audioTimer = nullptr; Serial.println("Audio: Freenove DAC timer unavailable");
  }
#else
  ledcSetup(BRICO_AUDIO_CHANNEL, 700, 10);
  ledcAttachPin(BRICO_AUDIO_PIN, BRICO_AUDIO_CHANNEL);
#endif
  tone(false, 700);
  pinMode(0, INPUT_PULLUP);
  if (BRICO_DIT_PIN >= 0) pinMode(BRICO_DIT_PIN, BRICO_DIT_PIN < 34 ? INPUT_PULLUP : INPUT);
  if (BRICO_DAH_PIN >= 0) pinMode(BRICO_DAH_PIN, BRICO_DAH_PIN < 34 ? INPUT_PULLUP : INPUT);
  if (TOUCH_IRQ >= 0) pinMode(TOUCH_IRQ, INPUT);
#if TOUCH_SCLK != TFT_SCLK
  pinMode(TOUCH_SCLK, OUTPUT); digitalWrite(TOUCH_SCLK, LOW);
  pinMode(TOUCH_MOSI, OUTPUT); pinMode(TOUCH_MISO, INPUT);
#endif
  Preferences storage;
  bool opened = storage.begin("brico-touch-v2", false);
  bool valid = opened && storage.getUInt("panel") == BRICO_PANEL &&
      storage.getBytesLength("cal") == sizeof(calibration) &&
      storage.getBytes("cal", &calibration, sizeof(calibration)) == sizeof(calibration) &&
      abs(calibration.xEnd - calibration.xStart) > 500 && abs(calibration.yEnd - calibration.yStart) > 500;
  if (!valid || digitalRead(0) == LOW) {
    calibrate();
    if (!opened || storage.putBytes("cal", &calibration, sizeof(calibration)) != sizeof(calibration) ||
        storage.putUInt("panel", BRICO_PANEL) != sizeof(uint32_t))
      Serial.println("Touch: calibration could not be saved");
  }
  if (opened) storage.end();
}

Touch readTouch() {
  if (millis() - polledAt < 5) { Touch result = lastTouch; result.tapped = false; return result; }
  polledAt = millis();
  uint16_t rawX, rawY;
  Touch result;
  result.down = rawTouch(rawX, rawY);
  if (result.down) {
    result.x = brico::clamp(brico::scaleAxis(calibration.swap ? rawY : rawX,
        calibration.xStart, calibration.xEnd, 24, DISPLAY_W - 25), 0, DISPLAY_W - 1);
    result.y = brico::clamp(brico::scaleAxis(calibration.swap ? rawX : rawY,
        calibration.yStart, calibration.yEnd, 24, DISPLAY_H - 25), 0, DISPLAY_H - 1);
  }
  result.tapped = result.down && !wasDown;
  wasDown = result.down; lastTouch = result;
  return result;
}
void setBrightness(int percent) { ledcWrite(BRICO_BACKLIGHT_CHANNEL, constrain(percent, 10, 100) * 255 / 100); }
void tone(bool enabled, int frequency) {
  if (enabled == previousTone && (!enabled || frequency == previousPitch)) return;
  previousTone = enabled; previousPitch = frequency;
#if BRICO_PANEL == 778932
  if (!audioTimer) return;
  esp_err_t stopped = esp_timer_stop(audioTimer);
  if (stopped != ESP_OK && stopped != ESP_ERR_INVALID_STATE) Serial.println("Audio: cannot stop DAC timer");
  if (enabled) {
    if (esp_timer_start_periodic(audioTimer, 500000 / frequency) != ESP_OK)
      Serial.println("Audio: cannot start DAC tone");
  } else { dacWrite(25, 128); dacWrite(26, 128); }
#else
  ledcWriteTone(BRICO_AUDIO_CHANNEL, enabled ? frequency : 0);
#endif
}
bool dit() { return BRICO_DIT_PIN >= 0 && digitalRead(BRICO_DIT_PIN) == LOW; }
bool dah() { return BRICO_DAH_PIN >= 0 && digitalRead(BRICO_DAH_PIN) == LOW; }
bool mountCard() {
  cardBus.begin(BRICO_SD_SCLK, BRICO_SD_MISO, BRICO_SD_MOSI, BRICO_SD_CS);
  bool mounted = SD.begin(BRICO_SD_CS, cardBus, 10000000);
  if (!mounted) Serial.println("SD: no card; card exercises unavailable");
  return mounted;
}
}
