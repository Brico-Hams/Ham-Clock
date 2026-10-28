#pragma once

#include <stdint.h>

namespace brico {
constexpr int controlAt(int x, int y, int width, int height, int barHeight) {
  return x < 0 || x >= width || y < height - barHeight || y >= height
      ? -1 : x * 5 / width;
}

constexpr int scaleAxis(int raw, int first, int last, int low, int high) {
  return first == last ? low :
      low + (raw - first) * (high - low) / (last - first);
}

constexpr int clamp(int value, int low, int high) {
  return value < low ? low : (value > high ? high : value);
}
}
