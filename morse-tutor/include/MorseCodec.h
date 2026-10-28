#pragma once
#include <stddef.h>
#include <stdint.h>

namespace brico {
struct Symbol { char letter; const char* elements; };
constexpr Symbol kAlphabet[] = {
  {'A', ".-"}, {'B', "-..."}, {'C', "-.-."}, {'D', "-.."}, {'E', "."},
  {'F', "..-."}, {'G', "--."}, {'H', "...."}, {'I', ".."}, {'J', ".---"},
  {'K', "-.-"}, {'L', ".-.."}, {'M', "--"}, {'N', "-."}, {'O', "---"},
  {'P', ".--."}, {'Q', "--.-"}, {'R', ".-."}, {'S', "..."}, {'T', "-"},
  {'U', "..-"}, {'V', "...-"}, {'W', ".--"}, {'X', "-..-"}, {'Y', "-.--"},
  {'Z', "--.."}, {'0', "-----"}, {'1', ".----"}, {'2', "..---"}, {'3', "...--"},
  {'4', "....-"}, {'5', "....."}, {'6', "-...."}, {'7', "--..."}, {'8', "---.."},
  {'9', "----."}, {'.', ".-.-.-"}, {',', "--..--"}, {'?', "..--.."}, {'/', "-..-."},
  {'=', "-...-"}, {'+', ".-.-."}, {'-', "-....-"}, {'@', ".--.-."}, {':', "---..."},
  {';', "-.-.-."}, {'(', "-.--."}, {')', "-.--.-"}, {'!', "-.-.--"},
  {'\'', ".----."}, {'"', ".-..-."}, {'&', ".-..."}, {'$', "...-..-"}
};

inline const char* elementsFor(char letter) {
  if (letter >= 'a' && letter <= 'z') letter -= 32;
  for (const auto& entry : kAlphabet) if (entry.letter == letter) return entry.elements;
  return nullptr;
}

inline char letterFor(const char* elements) {
  for (const auto& entry : kAlphabet) {
    size_t i = 0;
    while (elements[i] && entry.elements[i] == elements[i]) ++i;
    if (!elements[i] && !entry.elements[i]) return entry.letter;
  }
  return '?';
}

inline bool due(uint32_t now, uint32_t deadline) { return int32_t(now - deadline) >= 0; }

class KeyDecoder {
  char elements_[9] = {};
  uint8_t count_ = 0;
  bool down_ = false, wordSent_ = true;
  uint32_t pressedAt_ = 0, releasedAt_ = 0;
public:
  void reset(uint32_t now) {
    count_ = 0; elements_[0] = '\0'; down_ = false; wordSent_ = true; releasedAt_ = now;
  }
  char sample(bool down, uint32_t now, uint32_t dit) {
    char completed = '\0';
    if (!down_ && count_ && now - releasedAt_ >= dit * 3) {
      completed = letterFor(elements_);
      count_ = 0; elements_[0] = '\0';
    }
    if (down && !down_) { down_ = true; pressedAt_ = now; }
    if (!down && down_) {
      down_ = false;
      uint32_t duration = now - pressedAt_;
      if (duration >= 8) {
        if (count_ < sizeof(elements_) - 1) {
          elements_[count_++] = duration >= dit * 2 ? '-' : '.';
          elements_[count_] = '\0';
        } else { count_ = 1; elements_[0] = '!'; elements_[1] = '\0'; }
        releasedAt_ = now;
        wordSent_ = false;
      }
    }
    if (completed) return completed;
    if (!down_ && !wordSent_ && !count_ && now - releasedAt_ >= dit * 7) {
      wordSent_ = true;
      return ' ';
    }
    return '\0';
  }
};

class IambicKeyer {
  bool sounding_ = false, lastDash_ = true, rememberedDit_ = false, rememberedDah_ = false;
  bool active_ = false;
  uint32_t deadline_ = 0;
public:
  void reset() { sounding_ = false; active_ = false; rememberedDit_ = rememberedDah_ = false; lastDash_ = true; }
  bool sample(bool dit, bool dah, bool modeB, uint32_t now, uint32_t unit) {
    if (active_) {
      if (lastDash_ && dit && (modeB || !dah)) rememberedDit_ = true;
      if (!lastDash_ && dah && (modeB || !dit)) rememberedDah_ = true;
      if (!due(now, deadline_)) return sounding_;
      if (sounding_) { sounding_ = false; deadline_ = now + unit; return false; }
      active_ = false;
    }
    bool wantDit = dit || rememberedDit_, wantDah = dah || rememberedDah_;
    if (!wantDit && !wantDah) return false;
    bool dash = wantDah && (!wantDit || !lastDash_);
    if (dash) rememberedDah_ = false; else rememberedDit_ = false;
    lastDash_ = dash; active_ = sounding_ = true;
    deadline_ = now + unit * (dash ? 3 : 1);
    return true;
  }
};
}
