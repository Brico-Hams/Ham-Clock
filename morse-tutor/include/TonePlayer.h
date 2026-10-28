#pragma once
#include "MorseCodec.h"
#include <string.h>

namespace brico {
class TonePlayer {
  char text_[121] = {};
  const char* elements_ = nullptr;
  size_t position_ = 0, element_ = 0;
  uint32_t deadline_ = 0, unit_ = 100, spacing_ = 100, wordGap_ = 1;
  bool active_ = false, sounding_ = false, startCharacter_ = true;
  char letter_ = '\0';
public:
  bool start(const char* text, int characterWpm, int overallWpm, uint32_t now, unsigned wordGap = 1) {
    size_t length = strlen(text);
    if (!length || length >= sizeof(text_) || characterWpm < 3 || characterWpm > 50 ||
        overallWpm < 3 || overallWpm > characterWpm || wordGap < 1 || wordGap > 5) return false;
    bool hasSymbol = false;
    for (size_t i = 0; i < length; ++i) if (text[i] != ' ') {
      if (!elementsFor(text[i])) return false;
      hasSymbol = true;
    }
    if (!hasSymbol) return false;
    memcpy(text_, text, length + 1);
    unit_ = 1200 / characterWpm;
    spacing_ = (60000 / overallWpm - 31 * unit_) / 19;
    if (spacing_ < unit_) spacing_ = unit_;
    wordGap_ = wordGap;
    position_ = element_ = 0; deadline_ = now;
    active_ = true; sounding_ = false; startCharacter_ = true; letter_ = '\0';
    return true;
  }
  void stop() { active_ = sounding_ = false; letter_ = '\0'; }
  bool busy() const { return active_; }
  bool sounding() const { return sounding_; }
  char takeLetter() { char value = letter_; letter_ = '\0'; return value; }
  void tick(uint32_t now) {
    if (!active_ || !due(now, deadline_)) return;
    if (sounding_) {
      sounding_ = false;
      ++element_;
      if (elements_[element_]) deadline_ = now + unit_;
      else {
        ++position_;
        startCharacter_ = true;
        if (text_[position_] == ' ') {
          while (text_[position_] == ' ') ++position_;
          letter_ = ' ';
          deadline_ = now + 7 * spacing_ * wordGap_;
        } else deadline_ = now + 3 * spacing_;
      }
      return;
    }
    if (startCharacter_) {
      while (text_[position_] == ' ') ++position_;
      if (!text_[position_]) { active_ = false; return; }
      elements_ = elementsFor(text_[position_]);
      element_ = 0; startCharacter_ = false; letter_ = text_[position_];
    }
    sounding_ = true;
    deadline_ = now + unit_ * (elements_[element_] == '-' ? 3 : 1);
  }
};
}
