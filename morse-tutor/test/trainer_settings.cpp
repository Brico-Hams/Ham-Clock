#include "trainer_settings.h"
#include <Arduino.h>
#include <Preferences.h>
#include <assert.h>
#include <stdio.h>

TestSerial Serial;
int main() {
  assert(trainer::begin());
  assert(trainer::settings().language == 0 && trainer::settings().wordGap == 1);
  auto options = trainer::settings();
  options.language = 2; options.vocabulary = 4; options.sequence = 3; options.wordGap = 5;
  strcpy(options.custom, "KM");
  for (int i = 0; i < 8; ++i) snprintf(options.memories[i], sizeof(options.memories[i]), "CQ MEMORY %d", i + 1);
  uint32_t revision = trainer::revision();
  assert(trainer::save(options) && trainer::revision() == revision + 1);
  assert(trainer::language() == brico::Language::German && !strcmp(trainer::sequence(), "KM"));
  auto invalid = options; invalid.language = 5; assert(!trainer::save(invalid));
  invalid = options; invalid.vocabulary = 5; assert(!trainer::save(invalid));
  invalid = options; invalid.wordGap = 0; assert(!trainer::save(invalid));
  invalid = options; invalid.sequence = 4; assert(!trainer::save(invalid));
  invalid = options; strcpy(invalid.custom, "KK"); assert(!trainer::save(invalid));
  invalid = options; strcpy(invalid.memories[0], "lower"); assert(!trainer::save(invalid));
  invalid = options; memset(invalid.memories[0], 'E', sizeof(invalid.memories[0])); assert(!trainer::save(invalid));
  testStorage::failWrite() = true;
  invalid = options; invalid.language = 1;
  assert(!trainer::save(invalid));
  assert(trainer::language() == brico::Language::German);
  trainer::record("KM", "K");
  assert(!trainer::flushStats());
  testStorage::failWrite() = false;
  assert(trainer::flushStats());
  assert(trainer::begin());
  assert(trainer::language() == brico::Language::German && trainer::settings().vocabulary == 4);
  for (int i = 0; i < 8; ++i) assert(!strcmp(trainer::settings().memories[i], options.memories[i]));
  assert(trainer::stats().groups == 1 && trainer::stats().correct == 0);
  testStorage::values()["options"] = {1, 2, 3};
  testStorage::values()["stats"] = {1};
  unsigned messages = Serial.messages;
  assert(trainer::begin());
  assert(Serial.messages == messages + 2);
  assert(trainer::settings().language == 0 && trainer::stats().groups == 0);
  puts("PASS: language/vocabulary independence, eight persistent memories, custom Koch, rejected settings, failed writes and corrupt-storage diagnostics");
}
