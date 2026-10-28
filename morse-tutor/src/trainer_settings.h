#pragma once
#include "Training.h"

namespace trainer {
struct Settings {
  uint8_t language = 0, vocabulary = 0, sequence = 0, wordGap = 1;
  char custom[41] = "KMURESNAPTLWI";
  char memories[8][brico::kMaxText + 1] = {};
};
bool begin();
const Settings& settings();
bool save(const Settings& candidate);
uint32_t revision();
brico::Language language();
const char* tr(brico::Text id);
const char* sequence();
brico::TrainingStats& stats();
void record(const char* expected, const char* answer);
bool flushStats();
}
