#include "Training.h"
#include "TonePlayer.h"
#include <assert.h>
#include <stdio.h>
#include <string>

namespace {
void languages() {
  assert(brico::kLanguageCount == 5);
  assert(brico::validLanguage(0) && brico::validLanguage(4));
  assert(!brico::validLanguage(-1) && !brico::validLanguage(5));
  for (const auto& entry : brico::kTranslations) {
    for (size_t language = 0; language < brico::kLanguageCount; ++language) {
      assert(entry.values[language] && *entry.values[language]);
      char glyphs[500];
      size_t count = brico::displayText(entry.values[language], glyphs, sizeof(glyphs));
      assert(count > 0);
      // UI strings must render without missing GLCD glyphs (literal '?' is allowed).
      const char* input = entry.values[language];
      while (*input) {
        uint32_t code;
        assert(brico::readCodepoint(input, code));
        if (code != '?' && brico::cp437(code) == '?') {
          fprintf(stderr, "Missing glyph U+%04x in %s, language %u\n", unsigned(code), entry.key, unsigned(language));
          assert(false);
        }
      }
    }
  }
  char glyphs[20];
  assert(brico::displayText("Español", glyphs, sizeof(glyphs)) == 7);
  assert(uint8_t(glyphs[4]) == 164);
  assert(brico::displayText("Français", glyphs, 4) == 3 && glyphs[3] == '\0');
  for (size_t language = 0; language < brico::kLanguageCount; ++language) {
    for (auto id : {brico::Text::Save, brico::Text::Delete, brico::Text::Edit,
        brico::Text::Play, brico::Text::Next, brico::Text::Send}) {
      char label[80];
      size_t count = brico::displayText(brico::translate(id, brico::Language(language)), label, sizeof(label));
      assert(count * 6 <= 58); // Smallest 320px panel has 60px footer buttons.
    }
  }
  for (size_t language = 0; language < brico::kLanguageCount; ++language)
    for (const char* word : brico::kWords[language]) {
      char normalized[121];
      assert(brico::normalizeLatin(word, normalized, sizeof(normalized)));
      assert(!strcmp(word, normalized));
    }
}
void normalization() {
  struct Case { const char* input; const char* output; };
  const Case cases[] = {{"mañana, señal", "MANANA, SENAL"}, {"Grüße aus Köln", "GRUSSE AUS KOLN"},
    {"français été cœur", "FRANCAIS ETE COEUR"}, {"città più perché", "CITTA PIU PERCHE"},
    {"ÄÖÜ äöü ß ẞ Ææ Œœ", "AOU AOU SS SS AEAE OEOE"}, {" hello\t world \n", "HELLO WORLD"}};
  char output[121];
  for (const auto& test : cases) {
    assert(brico::normalizeLatin(test.input, output, sizeof(output)));
    assert(!strcmp(output, test.output));
  }
  assert(!brico::normalizeLatin("日本", output, sizeof(output)));
  assert(!brico::normalizeLatin("\xc0\xaf", output, sizeof(output)));
  assert(!brico::normalizeLatin("\xed\xa0\x80", output, sizeof(output)));
  assert(!brico::normalizeLatin("\xe0\x80\x80", output, sizeof(output)));
  assert(!brico::normalizeLatin("\xc3", output, sizeof(output)));
  assert(!brico::normalizeLatin("BAD#", output, sizeof(output)));
  assert(!brico::normalizeLatin(" ", output, sizeof(output)));
  assert(!brico::normalizeLatin(nullptr, output, sizeof(output)));
  assert(!brico::normalizeLatin("X", output, 0));
  assert(!brico::normalizeLatin(std::string(61, char(0xdf)).c_str(), output, sizeof(output)));
  std::string maximum(120, 'E');
  assert(brico::normalizeLatin((maximum + " \t ").c_str(), output, sizeof(output)));
  assert(strlen(output) == 120);
  assert(!brico::normalizeLatin((maximum + "E").c_str(), output, sizeof(output)));
  std::string expanded; for (int i = 0; i < 60; ++i) expanded += "ß";
  assert(brico::normalizeLatin(expanded.c_str(), output, sizeof(output)) && strlen(output) == 120);
  assert(!brico::normalizeLatin((expanded + "ß").c_str(), output, sizeof(output)));
}
void learning() {
  for (const char* sequence : brico::kSequences) assert(brico::validSet(sequence));
  assert(brico::validSet("KM") && brico::validSet("0123456789"));
  for (const char* invalid : {"K", "", "KK", "Km", "K M", "KM#", "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/+"})
    assert(!brico::validSet(invalid));
  brico::TrainingStats stats;
  stats.record("KM", "KM");
  stats.record("KM", "K");
  stats.record("KM", "");
  stats.record("KM", "KMEXTRA");
  assert(stats.groups == 4 && stats.correct == 1);
  assert(stats.weight('M') > stats.weight('K'));
  int countK = 0, countM = 0;
  const unsigned total = stats.weight('K') + stats.weight('M');
  for (unsigned i = 0; i < total; ++i) {
    char ch = stats.choose("KM", i);
    assert(ch == 'K' || ch == 'M');
    if (ch == 'K') ++countK; else ++countM;
  }
  assert(unsigned(countK) == stats.weight('K') && unsigned(countM) == stats.weight('M'));
  assert(stats.choose("", 10) == '\0');
  stats.groups = UINT32_MAX; stats.correct = 1;
  stats.record("E", "E");
  assert(stats.groups == UINT32_MAX / 2 + 1 && stats.correct == 1);
  for (auto& score : stats.scores) { score.attempts = UINT16_MAX; score.errors = UINT16_MAX; }
  stats.record("E", "E");
  for (const auto& score : stats.scores) assert(score.errors <= score.attempts);
}
void wordsworth() {
  brico::TonePlayer player;
  assert(!player.start("E E", 12, 12, 0, 0));
  assert(!player.start("E E", 12, 12, 0, 6));
  for (unsigned multiplier = 1; multiplier <= 5; ++multiplier) {
    assert(player.start("E E", 12, 12, 0, multiplier));
    unsigned duration = 0, tone = 0;
    for (unsigned now = 0; now < 5000; ++now) {
      player.tick(now);
      if (player.sounding()) ++tone;
      if (!player.busy()) { duration = now; break; }
    }
    assert(duration == 500 + 700 * multiplier);
    assert(tone == 200);
  }
}
void cardBlocks() {
  assert(brico::wholeWords("CQ CQ DE EA", 11, false) == 11);
  assert(brico::wholeWords("CQ CQ DE EA", 11, true) == 9);
  assert(brico::wholeWords("CQ CQ\nDE", 8, true) == 6);
  assert(brico::wholeWords("CQ DE ", 6, true) == 6);
  assert(brico::wholeWords("SUPERCALIFRAGILISTIC", 20, true) == 20);
  assert(brico::wholeWords("", 0, true) == 0);
}
}
int main() {
  languages(); normalization(); learning(); wordsworth(); cardBlocks();
  puts("PASS: five complete renderable catalogs/vocabularies, UTF-8 normalization, Koch sets, adaptive scoring, exact Wordsworth timings and whole-word SD blocks");
}
