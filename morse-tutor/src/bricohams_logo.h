#pragma once
// BricoHams wordmark from the user's proven Freenove 3.2-inch sketch (logo.h).
// Reduced to a 4-colour RGB565 palette and RLE to avoid a 67 KiB uncompressed
// bitmap in firmware. Original aspect ratio 300x112; no filesystem or LVGL.
#include <Arduino.h>
#include <TFT_eSPI.h>

namespace brico_logo {
constexpr int kWidth = 300, kHeight = 112;
constexpr uint16_t kPalette[] = {0xffff, 0x2125, 0xf343, 0xb596};
// One byte encodes a non-zero run length in its top six bits and a colour
// index in its lowest two bits; stream is Base64-encoded to keep the header small.
const char kPacked[] PROGMEM = R"BH(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)BH";

inline int b64(char ch) {
  if (ch >= 'A' && ch <= 'Z') return ch - 'A';
  if (ch >= 'a' && ch <= 'z') return ch - 'a' + 26;
  if (ch >= '0' && ch <= '9') return ch - '0' + 52;
  if (ch == '+') return 62;
  if (ch == '/') return 63;
  return -1;
}

// Cropping permits using the same real logo as the startup image and the BH
// pictogram in the main menu. Rendering is scanline-based (<2 KiB static RAM).
inline void draw(TFT_eSPI& tft, int x, int y, int outputWidth, int outputHeight,
                 int cropLeft = 0, int cropWidth = kWidth) {
  if (outputWidth <= 0 || outputWidth > 480 || outputHeight <= 0 ||
      cropLeft < 0 || cropWidth <= 0 || cropLeft + cropWidth > kWidth) return;
  static uint16_t sourceLine[kWidth];
  static uint16_t outputLine[480];
  uint32_t bits = 0;
  int bitCount = 0, sourceX = 0, sourceY = 0, outputY = 0;
  tft.setSwapBytes(true);  // Matches the working Freenove sketch.
  tft.startWrite();
  for (const char* p = kPacked; *p && sourceY < kHeight; ++p) {
    const int code = b64(*p);
    if (code < 0) continue;
    bits = (bits << 6) | uint32_t(code);
    bitCount += 6;
    if (bitCount < 8) continue;
    bitCount -= 8;
    const uint8_t packed = uint8_t(bits >> bitCount);
    int count = packed >> 2;
    const uint16_t colour = kPalette[packed & 3];
    while (count && sourceY < kHeight) {
      const int n = min(count, kWidth - sourceX);
      for (int i = 0; i < n; ++i) sourceLine[sourceX + i] = colour;
      sourceX += n; count -= n;
      if (sourceX == kWidth) {
        while (outputY < outputHeight &&
               outputY * kHeight / outputHeight == sourceY) {
          for (int i = 0; i < outputWidth; ++i)
            outputLine[i] = sourceLine[cropLeft + i * cropWidth / outputWidth];
          tft.pushImage(x, y + outputY, outputWidth, 1, outputLine);
          ++outputY;
        }
        sourceX = 0; ++sourceY;
      }
    }
  }
  tft.endWrite();
}
} // namespace brico_logo
