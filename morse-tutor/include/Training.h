#pragma once
#include "NetworkProtocol.h"
#include "Languages.h"
#include <limits.h>

namespace brico {
inline bool readCodepoint(const char*& input, uint32_t& code) {
  uint8_t lead = uint8_t(*input++);
  if (lead < 128) { code = lead; return true; }
  unsigned count = lead >= 0xc2 && lead <= 0xdf ? 1 : lead >= 0xe0 && lead <= 0xef ? 2 : 0;
  if (!count) return false;
  code = lead & (count == 1 ? 0x1f : 0x0f);
  for (unsigned i = 0; i < count; ++i) {
    uint8_t next = uint8_t(*input);
    if ((next & 0xc0) != 0x80) return false;
    ++input; code = (code << 6) | (next & 0x3f);
  }
  return (count != 2 || code >= 0x800) && (code < 0xd800 || code > 0xdfff);
}
inline const char* latinReplacement(uint32_t code) {
  if ((code >= 0xc0 && code <= 0xc5) || (code >= 0xe0 && code <= 0xe5)) return "A";
  if (code == 0xc6 || code == 0xe6) return "AE";
  if (code == 0xc7 || code == 0xe7) return "C";
  if ((code >= 0xc8 && code <= 0xcb) || (code >= 0xe8 && code <= 0xeb)) return "E";
  if ((code >= 0xcc && code <= 0xcf) || (code >= 0xec && code <= 0xef)) return "I";
  if (code == 0xd1 || code == 0xf1) return "N";
  if ((code >= 0xd2 && code <= 0xd6) || (code >= 0xf2 && code <= 0xf6) || code == 0xd8 || code == 0xf8) return "O";
  if ((code >= 0xd9 && code <= 0xdc) || (code >= 0xf9 && code <= 0xfc)) return "U";
  if (code == 0xdd || code == 0xfd || code == 0xff) return "Y";
  if (code == 0xdf || code == 0x1e9e) return "SS";
  if (code == 0x152 || code == 0x153) return "OE";
  return nullptr;
}
inline bool normalizeLatin(const char* input, char* output, size_t capacity) {
  if (!input || !output || !capacity) return false;
  size_t size = 0;
  bool space = false;
  const auto append = [&](char ch) -> bool {
    if (ch >= 'a' && ch <= 'z') ch -= 32;
    if (ch == '\r' || ch == '\n' || ch == '\t') ch = ' ';
    if (!supported(ch)) return false;
    if (ch == ' ') { space = size != 0; return true; }
    if (size + (space ? 1 : 0) + 1 >= capacity || size + (space ? 1 : 0) >= kMaxText) return false;
    if (space) output[size++] = ' ';
    output[size++] = ch; space = false;
    return true;
  };
  while (*input) {
    uint32_t code;
    if (!readCodepoint(input, code)) return false;
    if (code < 128) {
      if (!append(char(code))) return false;
    } else {
      const char* replacement = latinReplacement(code);
      if (!replacement) return false;
      while (*replacement) {
        if (!append(*replacement++)) return false;
      }
    }
  }
  output[size] = '\0';
  return size != 0;
}
inline uint8_t cp437(uint32_t code) {
  if (code < 128) return uint8_t(code);
  struct Glyph { uint16_t code; uint8_t glyph; };
  constexpr Glyph glyphs[] = {{0xe9,130},{0xe8,138},{0xea,136},{0xeb,137},{0xe0,133},
    {0xe1,160},{0xe2,131},{0xe4,132},{0xe7,135},{0xc7,128},{0xc9,144},{0xc4,142},
    {0xee,140},{0xef,139},{0xed,161},{0xec,141},{0xf1,164},{0xd1,165},{0xf3,162},
    {0xf2,149},{0xf4,147},{0xf6,148},{0xd6,153},{0xf9,151},{0xfa,163},{0xfb,150},
    {0xfc,129},{0xdc,154},{0xdf,225},{0xbf,168},{0xa1,173}};
  for (const auto& glyph : glyphs) if (code == glyph.code) return glyph.glyph;
  return '?';
}
inline size_t displayText(const char* input, char* output, size_t capacity) {
  size_t count = 0;
  if (!capacity) return 0;
  while (*input && count + 1 < capacity) {
    uint32_t code;
    if (!readCodepoint(input, code)) { output[count++] = '?'; break; }
    output[count++] = cp437(code);
  }
  output[count] = '\0'; return count;
}
constexpr const char* kSequences[] = {
  "KMURESNAPTLWI.JZFOY,VG5/Q9H38B?427C1D6X0",
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/",
  "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ.,?/"
};
inline bool validSet(const char* text) {
  size_t count = strlen(text);
  if (count < 2 || count > 40) return false;
  for (size_t i = 0; i < count; ++i) {
    if (!elementsFor(text[i]) || (text[i] >= 'a' && text[i] <= 'z')) return false;
    for (size_t j = 0; j < i; ++j) if (text[j] == text[i]) return false;
  }
  return true;
}
constexpr const char* kWords[kLanguageCount][20] = {
  {"RADIO","ANTENA","SENAL","AMIGO","CASA","HOLA","MAR","SOL","LUNA","MONTE","CABLE","ONDA","NORTE","SUR","ESTE","OESTE","CAMPO","PUERTO","BUENOS","DIAS"},
  {"RADIO","ANTENNA","SIGNAL","FRIEND","HOUSE","HELLO","SEA","SUN","MOON","HILL","CABLE","WAVE","NORTH","SOUTH","EAST","WEST","FIELD","PORT","GOOD","MORNING"},
  {"FUNK","ANTENNE","SIGNAL","FREUND","HAUS","HALLO","MEER","SONNE","MOND","BERG","KABEL","WELLE","NORDEN","SUEDEN","OSTEN","WESTEN","FELD","HAFEN","GUTEN","MORGEN"},
  {"RADIO","ANTENNE","SIGNAL","AMI","MAISON","BONJOUR","MER","SOLEIL","LUNE","MONT","CABLE","ONDE","NORD","SUD","EST","OUEST","CHAMP","PORT","BON","MATIN"},
  {"RADIO","ANTENNA","SEGNALE","AMICO","CASA","CIAO","MARE","SOLE","LUNA","MONTE","CAVO","ONDA","NORD","SUD","EST","OVEST","CAMPO","PORTO","BUON","GIORNO"}
};
struct CharacterScore { uint16_t attempts = 0, errors = 0; };
class TrainingStats {
  static size_t index(char ch) {
    for (size_t i = 0; i < sizeof(kAlphabet) / sizeof(*kAlphabet); ++i)
      if (kAlphabet[i].letter == ch) return i;
    return sizeof(kAlphabet) / sizeof(*kAlphabet);
  }
public:
  CharacterScore scores[sizeof(kAlphabet) / sizeof(*kAlphabet)] = {};
  uint32_t groups = 0, correct = 0;
  void record(const char* expected, const char* answer) {
    if (groups == UINT32_MAX) { groups /= 2; correct /= 2; }
    ++groups;
    if (!strcmp(expected, answer)) ++correct;
    size_t answerLength = strlen(answer);
    for (size_t i = 0; expected[i]; ++i) {
      size_t symbol = index(expected[i]);
      if (symbol >= sizeof(scores) / sizeof(*scores)) continue;
      auto& score = scores[symbol];
      if (score.attempts == UINT16_MAX) { score.attempts /= 2; score.errors /= 2; }
      ++score.attempts;
      if (i >= answerLength || answer[i] != expected[i]) ++score.errors;
    }
  }
  unsigned weight(char ch) const {
    size_t symbol = index(ch);
    if (symbol >= sizeof(scores) / sizeof(*scores)) return 1;
    const auto& score = scores[symbol];
    return 1 + (score.attempts ? 4 * unsigned(score.errors) / score.attempts : 1);
  }
  char choose(const char* alphabet, uint32_t random) const {
    unsigned total = 0;
    for (const char* ch = alphabet; *ch; ++ch) total += weight(*ch);
    if (!total) return '\0';
    unsigned slot = random % total;
    for (const char* ch = alphabet; *ch; ++ch) {
      unsigned value = weight(*ch);
      if (slot < value) return *ch;
      slot -= value;
    }
    return '\0';
  }
};
}
