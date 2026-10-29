#pragma once

#include <stdint.h>

namespace brico {
class TouchStraightKey {
 public:
  bool sample(bool down, bool inside, bool enabled) {
    if (!down) engaged_ = false;
    else if (!previousDown_) engaged_ = enabled && inside;
    else if (!enabled || !inside) engaged_ = false;
    previousDown_ = down;
    return engaged_;
  }
 private:
  bool previousDown_ = false, engaged_ = false;
};

// Accepts a new contact level only after it has stayed unchanged for `hold` ms,
// so mechanical key bounce neither clicks the tone nor splits marks.
class Debouncer {
 public:
  bool sample(bool raw, uint32_t now, uint32_t hold) {
    if (raw != candidate_) { candidate_ = raw; since_ = now; }
    if (raw != stable_ && now - since_ >= hold) stable_ = raw;
    return stable_;
  }
 private:
  bool stable_ = false, candidate_ = false;
  uint32_t since_ = 0;
};

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
