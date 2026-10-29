#include "music_player.h"

#include <FS.h>
#include <SD.h>
#include <SPI.h>

#include "AudioFileSourceID3.h"
#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include "AudioOutputI2SNoDAC.h"
#include <driver/i2s.h>

// The microSD slot is on 18/19/23 with CS 5 on every supported board. The
// display owns VSPI, so the card takes HSPI (touch is bit-banged or shares the
// display bus, see dashboard_display.cpp).
#ifndef MUSIC_SD_SCK
#define MUSIC_SD_SCK 18
#endif
#ifndef MUSIC_SD_MISO
#define MUSIC_SD_MISO 19
#endif
#ifndef MUSIC_SD_MOSI
#define MUSIC_SD_MOSI 23
#endif
#ifndef MUSIC_SD_CS
#define MUSIC_SD_CS 5
#endif

namespace {

enum class Command : uint8_t { kNone, kToggle, kNext, kPrevious };

SPIClass g_sdSpi(HSPI);
String g_tracks[kMusicMaxTracks];
uint8_t g_trackCount = 0;
bool g_available = false;

volatile Command g_command = Command::kNone;
volatile int16_t g_current = -1;
volatile bool g_playing = false;
volatile uint8_t g_volume = 60;
volatile uint32_t g_version = 0;

AudioGeneratorMP3* g_mp3 = nullptr;
AudioFileSourceSD* g_file = nullptr;
AudioFileSourceID3* g_id3 = nullptr;
AudioOutputI2S* g_out = nullptr;

bool isMp3(const String& name) {
  String lower = name;
  lower.toLowerCase();
  return lower.endsWith(".mp3");
}

void scanFolder(const char* path) {
  File dir = SD.open(path);
  if (!dir || !dir.isDirectory()) {
    return;
  }
  for (File entry = dir.openNextFile(); entry && g_trackCount < kMusicMaxTracks;
       entry = dir.openNextFile()) {
    String name = entry.name();
    if (!entry.isDirectory() && isMp3(name) && !name.startsWith(".")) {
      // Arduino-ESP32 2.x returns the bare name; keep the full path for open().
      g_tracks[g_trackCount++] = String(path) + (String(path).endsWith("/") ? "" : "/") + name;
    }
    entry.close();
  }
  dir.close();
}

void sortTracks() {
  for (uint8_t i = 1; i < g_trackCount; ++i) {
    String key = g_tracks[i];
    int8_t j = i - 1;
    while (j >= 0 && g_tracks[j].compareTo(key) > 0) {
      g_tracks[j + 1] = g_tracks[j];
      --j;
    }
    g_tracks[j + 1] = key;
  }
}

void setAmplifier(bool on) {
#ifdef MUSIC_AUDIO_ENABLE_PIN
  // Freenove: the amplifier is enabled with the pin low.
  digitalWrite(MUSIC_AUDIO_ENABLE_PIN, on ? LOW : HIGH);
#else
  (void)on;
#endif
}

void applyGain() {
  if (g_out) {
    // 0..100 % mapped to 0..1.0: higher gains clip the small on-board amplifiers.
    g_out->SetGain(g_volume / 100.0f);
  }
}

void stopTrack() {
  if (g_mp3 && g_mp3->isRunning()) {
    g_mp3->stop();
  }
  delete g_mp3;
  g_mp3 = nullptr;
  delete g_id3;
  g_id3 = nullptr;
  delete g_file;
  g_file = nullptr;
}

bool startTrack(int16_t index) {
  stopTrack();
  if (g_trackCount == 0) {
    return false;
  }
  index = (index % g_trackCount + g_trackCount) % g_trackCount;
  g_file = new AudioFileSourceSD(g_tracks[index].c_str());
  g_id3 = new AudioFileSourceID3(g_file);
  g_mp3 = new AudioGeneratorMP3();
  g_current = index;
  if (!g_file->isOpen() || !g_mp3->begin(g_id3, g_out)) {
    Serial.printf("Music: cannot play %s\n", g_tracks[index].c_str());
    stopTrack();
    return false;
  }
  return true;
}

void audioTask(void*) {
  for (;;) {
    const Command command = g_command;
    if (command != Command::kNone) {
      g_command = Command::kNone;
      if (command == Command::kToggle) {
        g_playing = !g_playing;
        if (g_playing && !(g_mp3 && g_mp3->isRunning())) {
          g_playing = startTrack(g_current < 0 ? 0 : g_current);
        }
      } else {
        const int16_t step = command == Command::kNext ? 1 : -1;
        const int16_t from = g_current < 0 ? 0 : g_current + step;
        if (g_playing) {
          g_playing = startTrack(from);
        } else {
          stopTrack();
          g_current = (from % g_trackCount + g_trackCount) % g_trackCount;
        }
      }
      setAmplifier(g_playing);
      ++g_version;
    }

    if (g_playing && g_mp3 && g_mp3->isRunning()) {
      if (!g_mp3->loop()) {
        // End of track: carry on with the next one.
        g_playing = startTrack(g_current + 1);
        setAmplifier(g_playing);
        ++g_version;
      }
      // Yield without sleeping while the I2S buffers are being refilled.
      taskYIELD();
    } else {
      vTaskDelay(pdMS_TO_TICKS(20));
    }
  }
}

}  // namespace

void musicBegin() {
#ifdef MUSIC_AUDIO_ENABLE_PIN
  pinMode(MUSIC_AUDIO_ENABLE_PIN, OUTPUT);
  setAmplifier(false);
#endif
  g_sdSpi.begin(MUSIC_SD_SCK, MUSIC_SD_MISO, MUSIC_SD_MOSI, MUSIC_SD_CS);
  if (!SD.begin(MUSIC_SD_CS, g_sdSpi, 20000000)) {
    Serial.println("Music: no microSD card");
    return;
  }
  scanFolder("/Music");
  if (g_trackCount == 0) {
    scanFolder("/");
  }
  sortTracks();
  if (g_trackCount == 0) {
    Serial.println("Music: no MP3 files on the card");
    return;
  }

#if defined(MUSIC_AUDIO_DAC)
  g_out = new AudioOutputI2S(0, AudioOutputI2S::INTERNAL_DAC);
#elif defined(MUSIC_AUDIO_PDM_PIN)
  // Sigma-delta on one pin only; the library's INTERNAL_PDM mode would also
  // claim GPIO25 through the DAC, which is the touch clock on the 2.8" CYD.
  auto* noDac = new AudioOutputI2SNoDAC(0);
  noDac->SetPinout(I2S_PIN_NO_CHANGE, I2S_PIN_NO_CHANGE, MUSIC_AUDIO_PDM_PIN);
  g_out = noDac;
#else
  Serial.println("Music: no audio output defined for this board");
  return;
#endif
  g_out->SetOutputModeMono(true);
  applyGain();
  g_current = 0;
  g_available = true;
  Serial.printf("Music: %u tracks\n", g_trackCount);
  // Core 0 next to Wi-Fi: the main loop on core 1 blocks on HTTP fetches.
  xTaskCreatePinnedToCore(audioTask, "music", 10240, nullptr, 2, nullptr, 0);
}

bool musicAvailable() { return g_available; }
uint8_t musicTrackCount() { return g_trackCount; }
int16_t musicCurrentTrack() { return g_current; }
bool musicIsPlaying() { return g_playing; }
uint8_t musicVolumePercent() { return g_volume; }
uint32_t musicStateVersion() { return g_version; }

String musicTrackTitle(uint8_t index) {
  if (index >= g_trackCount) {
    return "";
  }
  String title = g_tracks[index].substring(g_tracks[index].lastIndexOf('/') + 1);
  const int dot = title.lastIndexOf('.');
  return dot > 0 ? title.substring(0, dot) : title;
}

void musicTogglePlay() {
  if (g_available) g_command = Command::kToggle;
}

void musicNext() {
  if (g_available) g_command = Command::kNext;
}

void musicPrevious() {
  if (g_available) g_command = Command::kPrevious;
}

void musicChangeVolume(int8_t steps) {
  if (!g_available) return;
  g_volume = constrain(static_cast<int>(g_volume) + steps * 10, 0, 100);
  applyGain();
  ++g_version;
}
