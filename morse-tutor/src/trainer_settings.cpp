#include "trainer_settings.h"
#include <Arduino.h>
#include <Preferences.h>

namespace trainer {
namespace {
Settings options;
brico::TrainingStats results;
Preferences storage;
bool available = false, dirty = false;
uint32_t changed = 0;
bool valid(const Settings& candidate) {
  if (!brico::validLanguage(candidate.language) || !brico::validLanguage(candidate.vocabulary) ||
      candidate.sequence > 3 || candidate.wordGap < 1 || candidate.wordGap > 5 ||
      !memchr(candidate.custom, 0, sizeof(candidate.custom)) || !brico::validSet(candidate.custom)) return false;
  for (const auto& memory : candidate.memories) {
    if (!memchr(memory, 0, sizeof(memory))) return false;
    if (!memory[0]) continue;
    char normalized[brico::kMaxText + 1];
    if (!brico::normalize(memory, normalized, sizeof(normalized)) || strcmp(memory, normalized)) return false;
  }
  return true;
}
}
bool begin() {
  if (available) storage.end();
  options = Settings{}; results = brico::TrainingStats{}; dirty = false;
  available = storage.begin("brico-learn-v1", false);
  if (!available) { Serial.println("Trainer: persistence unavailable"); return false; }
  if (storage.isKey("options")) {
    Settings candidate;
    if (storage.getBytesLength("options") == sizeof(candidate) &&
        storage.getBytes("options", &candidate, sizeof(candidate)) == sizeof(candidate) && valid(candidate)) options = candidate;
    else Serial.println("Trainer: invalid saved options; factory values in RAM");
  }
  if (storage.isKey("stats")) {
    brico::TrainingStats candidate;
    bool ok = storage.getBytesLength("stats") == sizeof(candidate) &&
        storage.getBytes("stats", &candidate, sizeof(candidate)) == sizeof(candidate) && candidate.correct <= candidate.groups;
    for (const auto& score : candidate.scores) ok &= score.errors <= score.attempts;
    if (ok) results = candidate;
    else Serial.println("Trainer: invalid statistics; empty statistics in RAM");
  }
  return true;
}
const Settings& settings() { return options; }
bool save(const Settings& candidate) {
  if (!valid(candidate)) { Serial.println("Trainer: invalid settings rejected"); return false; }
  if (!available || storage.putBytes("options", &candidate, sizeof(candidate)) != sizeof(candidate)) {
    Serial.println("Trainer: could not save settings"); return false;
  }
  options = candidate; ++changed; return true;
}
uint32_t revision() { return changed; }
brico::Language language() { return brico::Language(options.language); }
const char* tr(brico::Text id) { return brico::translate(id, language()); }
const char* sequence() { return options.sequence == 3 ? options.custom : brico::kSequences[options.sequence]; }
brico::TrainingStats& stats() { return results; }
void record(const char* expected, const char* answer) { results.record(expected, answer); dirty = true; }
bool flushStats() {
  if (!dirty) return true;
  if (!available || storage.putBytes("stats", &results, sizeof(results)) != sizeof(results)) {
    Serial.println("Trainer: could not save statistics"); return false;
  }
  dirty = false; return true;
}
}
