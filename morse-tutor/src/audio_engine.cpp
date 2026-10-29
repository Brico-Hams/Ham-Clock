#include "audio_engine.h"
#include "board_io.h"
#include "TonePlayer.h"
#include "ControlLogic.h"
#include "NetworkProtocol.h"
#include <Arduino.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

namespace audio {
namespace {
static_assert(configTICK_RATE_HZ == 1000, "Morse timing requires a 1 ms FreeRTOS tick");
enum class Kind { Configure, Play };
struct Command {
  Kind kind = Kind::Configure;
  uint32_t generation = 0;
  char text[brico::kMaxText + 1] = {};
  int speed = 18, overall = 12, pitch = 700, keyMode = 0;
  unsigned wordGap = 1;
  bool enabled = false;
};
QueueHandle_t commands = nullptr, events = nullptr;
constexpr uint32_t kDebounceMs = 5;
std::atomic<uint32_t> generation{0};
// Generation still playing, 0 when idle; the worker clears only its own playback
// so a finish racing with a newer play() cannot report the new one as idle.
std::atomic<uint32_t> playing{0};
std::atomic<bool> virtualDit{false}, virtualDah{false};
std::atomic<bool> keyActive{false};
std::atomic<bool> virtualStraight{false};
bool ready = false;
void finished(uint32_t id) { playing.compare_exchange_strong(id, 0); }
void worker(void*) {
  brico::TonePlayer player;
  brico::KeyDecoder decoder;
  brico::IambicKeyer keyer;
  brico::Debouncer ditContact, dahContact;
  Command config;
  uint32_t currentGeneration = 0;
  uint32_t lastKeyedAt = 0;
  TickType_t wake = xTaskGetTickCount();
  decoder.reset(millis());
  while (true) {
    uint32_t now = millis(), requested = generation.load();
    if (requested != currentGeneration) {
      currentGeneration = requested;
      player.stop(); keyer.reset(); decoder.reset(now);
      board::tone(false, config.pitch);
      keyActive.store(false);
    }
    Command command;
    while (xQueueReceive(commands, &command, 0) == pdTRUE) {
      if (command.kind == Kind::Configure) {
        config = command;
        keyer.reset(); decoder.reset(now);
      } else if (command.generation == generation.load()) {
        currentGeneration = command.generation;
        player.stop(); keyer.reset(); decoder.reset(now);
        if (!player.start(command.text, command.speed, command.overall, now, command.wordGap)) {
          Serial.println("Audio: rejected invalid playback command");
          finished(currentGeneration);
        }
      }
    }
    Event event; event.generation = currentGeneration;
    if (player.busy()) {
      player.tick(now);
      event.letter = player.takeLetter();
      event.finished = !player.busy();
      if (event.finished) {
        finished(currentGeneration); keyer.reset(); decoder.reset(now);
      }
      board::tone(player.sounding(), config.pitch);
    } else {
      bool dit = virtualDit.load(), dah = virtualDah.load();
      bool contactDit = ditContact.sample(board::dit(), now, kDebounceMs);
      bool contactDah = dahContact.sample(board::dah(), now, kDebounceMs);
      bool keyed = false;
      if (config.enabled) {
        bool straight = config.keyMode == 0 && (contactDit || virtualStraight.load());
        if (config.keyMode != 0) { dit |= contactDit; dah |= contactDah; }
        keyed = keyer.sample(dit, dah, config.keyMode == 2, now, 1200 / config.speed) || straight;
        if (keyed) lastKeyedAt = now;
        keyActive.store(keyed || now - lastKeyedAt < uint32_t(7 * 1200 / config.speed));
        event.letter = decoder.sample(keyed, now, 1200 / config.speed);
        event.decoded = event.letter != '\0';
      } else keyActive.store(false);
      board::tone(keyed, config.pitch);
    }
    if ((event.letter || event.finished) && xQueueSend(events, &event, 0) != pdTRUE)
      Serial.println("Audio: UI event queue full; event dropped");
    vTaskDelayUntil(&wake, 1);
  }
}
}
bool begin() {
  commands = xQueueCreate(8, sizeof(Command));
  events = xQueueCreate(64, sizeof(Event));
  if (!commands || !events) {
    if (commands) { vQueueDelete(commands); commands = nullptr; }
    if (events) { vQueueDelete(events); events = nullptr; }
    Serial.println("Audio: cannot allocate queues"); return false;
  }
  ready = xTaskCreatePinnedToCore(worker, "brico-audio", 4096, nullptr, 3, nullptr, 1) == pdPASS;
  if (!ready) {
    vQueueDelete(commands); vQueueDelete(events); commands = events = nullptr;
    Serial.println("Audio: cannot start timing task");
  }
  return ready;
}
bool configure(int speed, int overall, int pitch, int keyMode, bool inputEnabled) {
  if (!ready) return false;
  Command command;
  command.speed = speed; command.overall = overall; command.pitch = pitch;
  command.keyMode = keyMode; command.enabled = inputEnabled;
  if (xQueueSend(commands, &command, 0) != pdTRUE) { Serial.println("Audio: configuration queue full"); return false; }
  return true;
}
bool play(const char* text, int speed, int overall, unsigned wordGap) {
  brico::TonePlayer validator;
  if (!ready || !validator.start(text, speed, overall, 0, wordGap)) return false;
  Command command;
  command.kind = Kind::Play; command.speed = speed; command.overall = overall;
  command.wordGap = wordGap;
  memcpy(command.text, text, strlen(text) + 1);
  command.generation = generation.fetch_add(1) + 1;
  playing.store(command.generation);
  if (xQueueSend(commands, &command, 0) != pdTRUE) {
    playing.store(0); Serial.println("Audio: playback queue full"); return false;
  }
  return true;
}
void stop() {
  playing.store(0); generation.fetch_add(1);
  virtualDit.store(false); virtualDah.store(false);
  virtualStraight.store(false);
}
bool busy() { return playing.load() != 0; }
bool inputBusy() { return keyActive.load(); }
bool poll(Event& event) {
  if (!ready) return false;
  while (xQueueReceive(events, &event, 0) == pdTRUE)
    if (event.generation == generation.load()) return true;
  return false;
}
void paddles(bool dit, bool dah) { virtualDit.store(dit); virtualDah.store(dah); }
void straightKey(bool down) { virtualStraight.store(down); }
}
