#pragma once

#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST -1
#define USE_HSPI_PORT
#define LOAD_GLCD
#define SPI_READ_FREQUENCY 16000000
#define SPI_TOUCH_FREQUENCY 2500000

#if BRICO_PANEL == 778932
#define ST7789_DRIVER
#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_ON
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define DISPLAY_W 320
#define DISPLAY_H 240
#define TFT_BL 27
#define TOUCH_SCLK 14
#define TOUCH_MOSI 13
#define TOUCH_MISO 12
// Conservative ST7789 bus clock: 80 MHz can leave corrupt/stale pixels.
#define SPI_FREQUENCY 40000000
#elif BRICO_PANEL == 7796
#define ST7796_DRIVER
#define TFT_WIDTH 320
#define TFT_HEIGHT 480
#define DISPLAY_W 480
#define DISPLAY_H 320
#define TFT_BL 27
#define TOUCH_SCLK 14
#define TOUCH_MOSI 13
#define TOUCH_MISO 12
#define TOUCH_RAW_MIN 200
#define TOUCH_RAW_MAX 3980
#define TOUCH_SWAP_XY 1
#define TOUCH_INVERT_X 1
#define TOUCH_INVERT_Y 1
#define SPI_FREQUENCY 80000000
#elif BRICO_PANEL == 9341 || BRICO_PANEL == 7789
#if BRICO_PANEL == 9341
#define ILI9341_DRIVER
#else
#define ST7789_DRIVER
#define TFT_INVERSION_ON
#endif
#define TFT_RGB_ORDER TFT_RGB
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define DISPLAY_W 320
#define DISPLAY_H 240
#define TFT_BL 21
#define TOUCH_SCLK 25
#define TOUCH_MOSI 32
#define TOUCH_MISO 39
#define TOUCH_RAW_MIN 120
#define TOUCH_RAW_MAX 3975
#define TOUCH_SWAP_XY 1
#define TOUCH_INVERT_X 0
#define TOUCH_INVERT_Y 1
#define SPI_FREQUENCY 40000000
#else
#error Select a supported BRICO_PANEL in platformio.ini
#endif

#define TOUCH_CS 33
#if BRICO_PANEL == 778932
#define TOUCH_IRQ -1
#define BRICO_AUDIO_ENABLE 4
#else
#define TOUCH_IRQ 36
#endif
#define BRICO_CONTROL_HEIGHT 36
#define BRICO_AUDIO_PIN 26
#define BRICO_AUDIO_CHANNEL 0
#define BRICO_BACKLIGHT_CHANNEL 2
#define BRICO_SD_CS 5
#define BRICO_SD_SCLK 18
#define BRICO_SD_MISO 19
#define BRICO_SD_MOSI 23

#ifndef BRICO_DIT_PIN
#if BRICO_PANEL == 7796 || BRICO_PANEL == 778932
#define BRICO_DIT_PIN -1
#else
#define BRICO_DIT_PIN 22
#endif
#endif
#ifndef BRICO_DAH_PIN
#if BRICO_PANEL == 7796 || BRICO_PANEL == 778932
#define BRICO_DAH_PIN -1
#else
#define BRICO_DAH_PIN 27
#endif
#endif

#ifdef __cplusplus
constexpr bool bricoReservedPin(int pin) {
  return pin == TFT_MISO || pin == TFT_MOSI || pin == TFT_SCLK ||
      pin == TFT_CS || pin == TFT_DC || pin == TFT_BL || pin == TOUCH_CS ||
      pin == TOUCH_IRQ || pin == TOUCH_MOSI || pin == TOUCH_MISO ||
      pin == TOUCH_SCLK || pin == BRICO_AUDIO_PIN || pin == BRICO_SD_CS ||
#if BRICO_PANEL == 778932
      pin == 25 || pin == BRICO_AUDIO_ENABLE ||
#endif
      pin == BRICO_SD_SCLK || pin == BRICO_SD_MISO || pin == BRICO_SD_MOSI ||
      pin == 0 || pin == 1 || pin == 3 || (pin >= 6 && pin <= 11);
}
static_assert(BRICO_DIT_PIN < 0 || !bricoReservedPin(BRICO_DIT_PIN), "DIT conflicts with a reserved GPIO");
static_assert(BRICO_DAH_PIN < 0 || !bricoReservedPin(BRICO_DAH_PIN), "DAH conflicts with a reserved GPIO");
static_assert(BRICO_DIT_PIN < 0 || BRICO_DAH_PIN < 0 || BRICO_DIT_PIN != BRICO_DAH_PIN,
              "DIT and DAH must use different pins");
#endif
