#pragma once

// TFT_eSPI setup for the Freenove ESP32 Display 3.2" ST7789 (FNK0103L / FNK0114L,
// 320x240 IPS). Pin map taken from the Bricohams Morse Tutor's freenove-32-st7789
// profile in this repository; untested with the dashboard on real hardware.
//
// PlatformIO uses this file via the freenove-32-st7789 env in platformio.ini:
//   build_flags = -D USER_SETUP_LOADED=1 -include include/User_Setup_Freenove32.h

#define ST7789_DRIVER
#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_ON

#define TFT_WIDTH  240
#define TFT_HEIGHT 320
#define DISPLAY_W 320
#define DISPLAY_H 240

#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1

#define TFT_BL   27
#define TFT_BACKLIGHT_ON HIGH

// The XPT2046 shares the display bus and has no interrupt line: touch is
// detected from the pressure reading instead (see readRawTouch).
#define TOUCH_SCLK 14
#define TOUCH_MOSI 13
#define TOUCH_MISO 12
#define TOUCH_CS  33
#define TOUCH_IRQ -1

// The dashboard rewrites MADCTL at runtime, so the panel's BGR order has to be
// the settings default rather than only TFT_RGB_ORDER.
#define SWAP_RED_BLUE_DEFAULT true

// Music page: internal DAC on GPIO25/26 into the on-board amplifier, which is
// enabled by driving GPIO4 low. The SD slot is on VSPI 18/19/23, CS 5.
#define MUSIC_AUDIO_DAC 1
#define MUSIC_AUDIO_ENABLE_PIN 4

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  16000000
#define SPI_TOUCH_FREQUENCY 2500000
