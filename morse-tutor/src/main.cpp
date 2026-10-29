// Copyright (c) 2026 Bricohams. New implementation, MIT licensed.
#include <Arduino.h>
#include <Preferences.h>
#include <SD.h>
#include "board_io.h"
#include "tutor_network.h"
#include "audio_engine.h"
#include "ControlLogic.h"
#include "trainer_settings.h"

namespace {
using board::display;
using brico::Text;
using trainer::tr;
constexpr uint16_t kBackground = 0x10c3, kCard = 0x2125, kAccent = 0xfb23, kMuted = 0xad55;
constexpr int kFooter = DISPLAY_H - BRICO_CONTROL_HEIGHT;
enum class Screen { Home, Menu, Exercise, Setting, Callsign, Reset, Files, Memories, Statistics };
enum class Activity { Koch, Letters, Words, Numbers, Mixed, Card, Qso, Calls,
  Practice, CopyOne, CopyTwo, CopyWord, CopyCall, Flash, HeadCopy, Network, Echo, Adaptive, Memories, Statistics };
struct MenuEntry { Text label; Activity action; };
constexpr MenuEntry kReceive[] = {{Text::Koch, Activity::Koch}, {Text::Letters, Activity::Letters},
  {Text::Words, Activity::Words}, {Text::Numbers, Activity::Numbers}, {Text::Mixed, Activity::Mixed},
  {Text::Card, Activity::Card}, {Text::Qso, Activity::Qso}, {Text::Calls, Activity::Calls}};
constexpr MenuEntry kSend[] = {{Text::Practice, Activity::Practice}, {Text::CopyOne, Activity::CopyOne},
  {Text::CopyTwo, Activity::CopyTwo}, {Text::CopyWord, Activity::CopyWord}, {Text::CopyCall, Activity::CopyCall},
  {Text::Flash, Activity::Flash}, {Text::HeadCopy, Activity::HeadCopy}, {Text::Network, Activity::Network},
  {Text::Echo, Activity::Echo}, {Text::Adaptive, Activity::Adaptive}, {Text::Memories, Activity::Memories},
  {Text::Statistics, Activity::Statistics}};
constexpr Text kSettings[] = {Text::Speed, Text::Farnsworth, Text::Pitch, Text::KeyMode, Text::Callsign,
  Text::Brightness, Text::KochLevel, Text::GroupSize, Text::Reset, Text::UiLanguage,
  Text::WordLanguage, Text::Sequence, Text::CustomSet, Text::WordGap};
struct Config {
  int speed = 18, overall = 12, pitch = 700, brightness = 80, keyMode = 0, lesson = 2, groupSize = 5;
  char callsign[brico::kCallSize] = "NOCALL";
};
Config config;
Preferences storage;
bool storageReady = false, cardReady = false, paused = false, waitingAnswer = false;
bool playbackVisible = true, playingNetwork = false, redraw = true;
bool touchKeyDown = false;
brico::TouchStraightKey touchKey;
Screen screen = Screen::Home;
Activity activity = Activity::Letters;
int menuGroup = 0, menuPage = 0, focus = 0, setting = 0, hits = 0, attempts = 0, fileCount = 0, filePage = 0;
char target[brico::kMaxText + 1] = {}, answer[brico::kMaxText + 1] = {}, outgoingWord[brico::kMaxText + 1] = {};
char editingCall[brico::kMaxText + 1] = {};
int editor = 0, keyboardPage = 0, memoryIndex = 0, memoryPage = 0;
constexpr const char* kKeyboard = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789/ .?,=+-@:;()!'\"&$";
trainer::Settings draft;
uint32_t answerDeadline = 0, settingsRevision = 0;
String transcript, notice;
String files[20];
File lessonFile;
uint32_t nextExercise = 0;

bool editing(Text id) { return kSettings[setting] == id; }
bool receiveActivity() { return int(activity) <= int(Activity::Calls); }
bool challengeActivity() { return (activity >= Activity::CopyOne && activity <= Activity::HeadCopy) ||
    activity == Activity::Echo || activity == Activity::Adaptive; }
int menuCount() { return menuGroup == 0 ? sizeof(kReceive) / sizeof(*kReceive) :
    menuGroup == 1 ? sizeof(kSend) / sizeof(*kSend) : sizeof(kSettings) / sizeof(*kSettings); }
void scoreAnswer();
bool save() {
  if (!storageReady) { notice = tr(Text::SaveError); Serial.println(notice); redraw = true; return false; }
  bool ok = storage.putInt("speed", config.speed) == sizeof(int);
  ok &= storage.putInt("overall", config.overall) == sizeof(int);
  ok &= storage.putInt("pitch", config.pitch) == sizeof(int);
  ok &= storage.putInt("brightness", config.brightness) == sizeof(int);
  ok &= storage.putInt("key", config.keyMode) == sizeof(int);
  ok &= storage.putInt("lesson", config.lesson) == sizeof(int);
  ok &= storage.putInt("group", config.groupSize) == sizeof(int);
  ok &= storage.putString("call", config.callsign) == strlen(config.callsign);
  if (!ok) { notice = tr(Text::SaveError); Serial.println(notice); }
  redraw = true;
  return ok;
}
void load() {
  storageReady = storage.begin("bricohams-v2", false);
  if (!storageReady) { Serial.println("Config: persistence unavailable"); return; }
  config.speed = constrain(storage.getInt("speed", 18), 3, 50);
  config.overall = constrain(storage.getInt("overall", 12), 3, config.speed);
  config.pitch = constrain(storage.getInt("pitch", 700), 300, 1800);
  config.brightness = constrain(storage.getInt("brightness", 80), 10, 100);
  config.keyMode = constrain(storage.getInt("key", 0), 0, 2);
  config.lesson = constrain(storage.getInt("lesson", 2), 2, int(strlen(trainer::sequence())));
  config.groupSize = constrain(storage.getInt("group", 5), 1, 10);
  String call = storage.getString("call", "NOCALL");
  if (brico::validCall(call.c_str())) memcpy(config.callsign, call.c_str(), call.length() + 1);
}
void text(const char* label, int x, int y, int size = 2, uint16_t color = TFT_WHITE) {
  char glyphs[256]; brico::displayText(label, glyphs, sizeof(glyphs));
  display.setTextSize(size); display.setTextColor(color, kBackground);
  display.setCursor(x, y); display.print(glyphs);
}
void center(const char* label, int x, int y, int width, int size = 2, uint16_t color = TFT_WHITE) {
  char glyphs[256]; size_t length = brico::displayText(label, glyphs, sizeof(glyphs));
  if (int(length) * size * 6 > width) size = 1;
  text(label, x + max(0, (width - int(length) * size * 6) / 2), y, size, color);
}
// Draw the same vector BricoHams mark on every supported display size.
// Keeping it in firmware means no SD/PSRAM/extra image library is required.
void brandEmblem(int centerX, int centerY, int radius) {
  display.drawCircle(centerX, centerY, radius, kAccent);
  display.drawCircle(centerX, centerY, radius - max(2, radius / 10), kMuted);
  // Radio antenna mast and angled arms.
  display.drawLine(centerX, centerY - radius * 2 / 3, centerX, centerY + radius / 2, TFT_WHITE);
  display.drawLine(centerX, centerY - radius / 3, centerX - radius / 2,
                   centerY + radius / 3, TFT_WHITE);
  display.drawLine(centerX, centerY - radius / 3, centerX + radius / 2,
                   centerY + radius / 3, TFT_WHITE);
  display.fillCircle(centerX, centerY - radius * 2 / 3, max(2, radius / 9), kAccent);
  display.drawFastHLine(centerX - radius / 2, centerY + radius / 2, radius, kAccent);
}
void brandWordmark(int x, int y, int scale) {
  display.setTextSize(scale);
  display.setTextColor(TFT_WHITE, kBackground);
  display.setCursor(x, y);
  display.print("brico");
  display.setTextColor(kAccent, kBackground);
  display.print("hams");
}
void drawBootLogo() {
  display.fillScreen(kBackground);
  const int centerX = DISPLAY_W / 2;
  const int centerY = DISPLAY_H / 2 - DISPLAY_H / 10;
  const int radius = min(DISPLAY_H / 4, 68);
  brandEmblem(centerX, centerY, radius);
  const int scale = DISPLAY_W >= 400 ? 5 : 4;
  const int textWidth = 9 * 6 * scale;
  const int textY = centerY + radius + 12;
  brandWordmark((DISPLAY_W - textWidth) / 2, textY, scale);
}
void drawHomeBrand() {
  brandEmblem(20, 20, 11);
  brandWordmark(40, 12, 2);
}

void button(int x, int y, int width, int height, const char* label, bool selected = false) {
  display.fillRoundRect(x, y, width, height, 6, selected ? kAccent : kCard);
  display.drawRoundRect(x, y, width, height, 6, kAccent);
  char glyphs[128]; size_t length = brico::displayText(label, glyphs, sizeof(glyphs));
  int size = int(length) * 12 > width - 8 ? 1 : 2;
  display.setTextSize(size); display.setTextColor(selected ? kBackground : TFT_WHITE, selected ? kAccent : kCard);
  display.setCursor(x + max(2, (width - int(length) * size * 6) / 2), y + (height - size * 8) / 2);
  display.print(glyphs);
}
bool inside(const board::Touch& touch, int x, int y, int width, int height) {
  return touch.x >= x && touch.x < x + width && touch.y >= y && touch.y < y + height;
}
void footer(const char* first, const char* second, const char* third, const char* fourth, const char* fifth) {
  const char* labels[] = {first, second, third, fourth, fifth};
  for (int i = 0; i < 5; ++i) button(i * DISPLAY_W / 5 + 2, kFooter, DISPLAY_W / 5 - 4, 33, labels[i]);
}
bool touchKeyScreen() {
  return screen == Screen::Exercise && !receiveActivity() && config.keyMode == 0;
}
void drawTouchKey() {
  button(3 * DISPLAY_W / 5 + 2, kFooter, 2 * DISPLAY_W / 5 - 4, 33,
      tr(Text::TouchKey), touchKeyDown);
}
void exerciseFooter(const char* first, const char* second, const char* third) {
  footer(first, second, third, touchKeyScreen() ? "" : "DIT", touchKeyScreen() ? "" : "DAH");
  if (touchKeyScreen()) drawTouchKey();
}
void header(const char* title) {
  button(6, 5, 76, 30, tr(Text::Back));
  center(title, 87, 13, DISPLAY_W - 94, 2, kAccent);
  display.drawFastHLine(6, 41, DISPLAY_W - 12, kAccent);
}
void append(const char* value) {
  transcript += value;
  if (transcript.length() > 320) transcript.remove(0, transcript.length() - 320);
  redraw = true;
}
void append(char value) { char valueString[] = {value, '\0'}; append(valueString); }
// Only update the transcript rows that changed, and draw appended characters
// without clearing their row. This prevents flicker during Morse playback.
void renderTranscript(int y = 93, bool full = true) {
  static String shown[16];
  static int shownY = -1, shownRows = -1;
  const int columns = (DISPLAY_W - 16) / 12;
  const int rows = constrain((kFooter - y - 18) / 18, 1, 16);
  const bool reset = full || shownY != y || shownRows != rows;
  shownY = y; shownRows = rows;
  const size_t available = size_t(columns * rows);
  const String visible = transcript.length() > available ?
      transcript.substring(transcript.length() - available) : transcript;
  for (int row = 0; row < rows; ++row) {
    const String line = visible.substring(row * columns, (row + 1) * columns);
    if (!reset && line == shown[row]) continue;
    if (!reset && line.startsWith(shown[row]) && line.length() > shown[row].length()) {
      char glyphs[128];
      const size_t prefixWidth = brico::displayText(shown[row].c_str(), glyphs, sizeof(glyphs));
      text(line.substring(shown[row].length()).c_str(), 8 + int(prefixWidth) * 12, y + row * 18);
    } else {
      display.fillRect(8, y + row * 18, DISPLAY_W - 16, 16, kBackground);
      if (line.length()) text(line.c_str(), 8, y + row * 18);
    }
    shown[row] = line;
  }
}
void drawExerciseLine(String& shown, const String& value, int y, uint16_t color, bool full) {
  if (!full && shown == value) return;
  if (!full) display.fillRect(8, y, DISPLAY_W - 16, 9, kBackground);
  if (value.length()) text(value.c_str(), 8, y, 1, color);
  shown = value;
}
const char* activityName() {
  for (const auto& entry : kReceive) if (entry.action == activity) return tr(entry.label);
  for (const auto& entry : kSend) if (entry.action == activity) return tr(entry.label);
  return "CW";
}
void render() {
  static Screen paintedScreen = Screen::Home;
  static bool painted = false;
  // Wipe only when changing views; keep the TFT intact between letters.
  const bool updatingExercise = painted && paintedScreen == Screen::Exercise && screen == Screen::Exercise;
  redraw = false;
  if (!updatingExercise) display.fillScreen(kBackground);
  if (screen == Screen::Home) {
    drawHomeBrand();
    // Use the persisted callsign from Settings, never a hardcoded example.
    const int callWidth = int(strlen(config.callsign)) * 12;
    text(config.callsign, DISPLAY_W - callWidth - 8, 12, 2, kAccent);
    display.drawFastHLine(8, 39, DISPLAY_W - 16, kAccent);
    const char* names[] = {tr(Text::Receive), tr(Text::Send), tr(Text::Config)};
    const char* icons[] = {"RX", "TX", "*"};
    for (int i = 0; i < 3; ++i) {
      int width = (DISPLAY_W - 24) / 3, x = 8 + i * (DISPLAY_W - 8) / 3;
      button(x, 57, width, kFooter - 70, "", i == focus);
      center(icons[i], x, 75, width, 3, kAccent);
      center(names[i], x, 133, width, 2);
    }
    String status = networkReady() ? String(tr(Text::Network)) + ": " + networkPending() + " " + tr(Text::Pending) : tr(Text::NetworkError);
    text(status.c_str(), 8, kFooter - 15, 1, kMuted);
    footer("-", "+", "OK", "", "");
  } else if (screen == Screen::Menu) {
    const char* names[] = {tr(Text::Receive), tr(Text::Send), tr(Text::Config)};
    header(names[menuGroup]);
    int count = menuCount();
    for (int i = 0; i < 4; ++i) {
      int index = menuPage * 4 + i;
      if (index >= count) continue;
      const char* label = tr(menuGroup == 0 ? kReceive[index].label : menuGroup == 1 ? kSend[index].label : kSettings[index]);
      button(8 + (i % 2) * DISPLAY_W / 2, 52 + (i / 2) * 55,
          DISPLAY_W / 2 - 16, 47, label, focus == i);
    }
    String page = String(menuPage + 1) + "/" + ((count + 3) / 4);
    center(page.c_str(), 0, kFooter - 20, DISPLAY_W, 1, kMuted);
    footer("<", ">", "OK", "", "");
  } else if (screen == Screen::Exercise) {
    static String firstLine, secondLine, thirdLine, shownNotice;
    static bool shownPaused = false, shownKeyDown = false, shownStraight = false;
    const bool full = !updatingExercise;
    if (full) {
      firstLine = secondLine = thirdLine = shownNotice = "";
      header(activityName());
    }
    if (activity == Activity::Network) {
      const String mode = String(tr(networkDecodeEnabled() ? Text::TextAudio : Text::MorseOnly)) +
          " | RX " + networkPending();
      drawExerciseLine(firstLine, mode, 49, kAccent, full);
      drawExerciseLine(secondLine, networkSsid(), 63, kMuted, full);
      drawExerciseLine(thirdLine,
          String(tr(Text::Password)) + ": " + kNetworkPassword + " | 192.168.4.1", 76, kMuted, full);
      renderTranscript(94, full);
    } else {
      String status = String(config.speed) + "/" + config.overall + " WPM  " + config.pitch + " Hz";
      if (challengeActivity()) status += "  " + String(hits) + "/" + attempts;
      drawExerciseLine(firstLine, status, 49, kMuted, full);
      const String level = (activity == Activity::Koch || activity == Activity::Adaptive)
          ? String(tr(Text::KochLevel)) + " " + config.lesson : String();
      drawExerciseLine(secondLine, level, 64, kAccent, full);
      const String prompt = waitingAnswer
          ? String(tr(activity == Activity::Flash ? Text::Reveal : Text::AnswerPrompt)) : String();
      drawExerciseLine(thirdLine, prompt, 76, kAccent, full);
      renderTranscript(93, full);
    }
    const bool straight = touchKeyScreen();
    if (full || straight != shownStraight) {
      exerciseFooter(activity == Activity::Network ? "CW" : "-",
          activity == Activity::Network ? "TXT" : "+",
          activity == Activity::Network ? tr(Text::Send) : (paused ? tr(Text::Next) : "OK"));
    } else {
      if (paused != shownPaused && activity != Activity::Network)
        button(2 * DISPLAY_W / 5 + 2, kFooter, DISPLAY_W / 5 - 4, 33, paused ? tr(Text::Next) : "OK");
      if (straight && touchKeyDown != shownKeyDown) drawTouchKey();
    }
    shownPaused = paused; shownKeyDown = touchKeyDown; shownStraight = straight;
    drawExerciseLine(shownNotice,
        notice.length() ? notice.substring(0, (DISPLAY_W - 16) / 6) : String(),
        kFooter - 14, kAccent, full);
  } else if (screen == Screen::Setting) {
    header(tr(kSettings[setting]));
    String value;
    if (editing(Text::Speed)) value = String(config.speed) + " WPM";
    if (editing(Text::Farnsworth)) value = String(config.overall) + " WPM";
    if (editing(Text::Pitch)) value = String(config.pitch) + " Hz";
    if (editing(Text::KeyMode)) value = config.keyMode == 0 ? tr(Text::Straight) : config.keyMode == 1 ? "Iambic A" : "Iambic B";
    if (editing(Text::Brightness)) value = String(config.brightness) + " %";
    if (editing(Text::KochLevel)) value = String(config.lesson);
    if (editing(Text::GroupSize)) value = String(config.groupSize);
    if (editing(Text::UiLanguage)) value = brico::kLanguageNames[draft.language];
    if (editing(Text::WordLanguage)) value = brico::kLanguageNames[draft.vocabulary];
    if (editing(Text::Sequence)) {
      const Text names[] = {Text::SequenceBrico, Text::SequenceLetters, Text::SequenceDigits, Text::CustomSet};
      value = tr(names[draft.sequence]);
    }
    if (editing(Text::WordGap)) value = String("x") + draft.wordGap;
    center(value.c_str(), 0, 100, DISPLAY_W, 2, kAccent);
    if (editing(Text::Pitch)) center(tr(Text::TestTone), 0, 146, DISPLAY_W, 1, kMuted);
    if (notice.length()) center(notice.c_str(), 0, kFooter - 22, DISPLAY_W, 1, kAccent);
    footer("-", "+", tr(Text::Save), "DIT", "");
  } else if (screen == Screen::Callsign) {
    header(tr(editor == 0 ? Text::Callsign : editor == 1 ? Text::CustomSet : Text::Memories));
    size_t length = strlen(editingCall);
    center(length > 25 ? editingCall + length - 25 : editingCall, 0, 49, DISPLAY_W, 2, kAccent);
    const char* keys = kKeyboard;
    int cellWidth = DISPLAY_W / 10;
    for (int i = 0; i < 40 && keyboardPage * 40 + i < int(strlen(keys)); ++i) {
      char key[] = {keys[keyboardPage * 40 + i], '\0'};
      if (key[0] == ' ') key[0] = '_';
      button((i % 10) * cellWidth + 1, 75 + (i / 10) * 28, cellWidth - 2, 26, key);
    }
    if (notice.length()) text(notice.substring(0, (DISPLAY_W - 16) / 6).c_str(), 8, kFooter - 14, 1, kAccent);
    footer(tr(Text::Delete), "...", tr(Text::Save), "", "");
  } else if (screen == Screen::Reset) {
    header(tr(Text::Reset));
    center(tr(Text::ResetQuestion), 0, 80, DISPLAY_W, 2, kAccent);
    center(tr(Text::ResetNote), 0, 114, DISPLAY_W, 1);
    footer(tr(Text::No), "", tr(Text::Yes), "", "");
  } else if (screen == Screen::Files) {
    header(tr(Text::Card));
    for (int i = 0; i < 4; ++i) {
      int index = filePage * 4 + i;
      if (index < fileCount) button(8, 49 + i * 36, DISPLAY_W - 16, 32, files[index].substring(0, 24).c_str());
    }
    if (!fileCount) text(tr(cardReady ? Text::NoFiles : Text::NoCard), 12, 85, 1);
    footer("<", ">", "", "", "");
  } else if (screen == Screen::Memories) {
    header(tr(Text::Memories));
    for (int i = 0; i < 4; ++i) {
      int index = memoryPage * 4 + i;
      String label = String(index + 1) + ": " + trainer::settings().memories[index];
      button(8, 49 + i * 36, DISPLAY_W - 16, 32, label.substring(0, 24).c_str(), memoryIndex == index);
    }
    footer("<", ">", tr(Text::Play), tr(Text::Edit), "");
    if (notice.length()) text(notice.c_str(), 8, kFooter - 14, 1, kAccent);
  } else if (screen == Screen::Statistics) {
    header(tr(Text::Statistics));
    const auto& stats = trainer::stats();
    String summary = String(tr(Text::Attempts)) + ": " + stats.groups + "  " + tr(Text::Accuracy) +
        ": " + (stats.groups ? uint32_t(uint64_t(stats.correct) * 100 / stats.groups) : 0) + "%";
    text(summary.c_str(), 8, 50, 1, kAccent);
    int row = 0;
    for (size_t i = 0; i < sizeof(stats.scores) / sizeof(*stats.scores); ++i) {
      const auto& score = stats.scores[i];
      if (!score.attempts) continue;
      if (row / 4 == filePage) {
        String line = String(brico::kAlphabet[i].letter) + "  " + tr(Text::Attempts) + ": " + score.attempts +
            "  " + tr(Text::Errors) + ": " + score.errors;
        text(line.c_str(), 8, 75 + (row % 4) * 26, 1);
      }
      ++row;
    }
    if (!stats.groups) text(tr(Text::NoStats), 8, 85, 1);
    footer("<", ">", "", "", "");
  }
  paintedScreen = screen;
  painted = true;
}
void stopPlayback() { audio::stop(); }
void back() {
  stopPlayback();
  if (lessonFile) lessonFile.close();
  if (outgoingWord[0]) {
    if (!networkPublish(outgoingWord, config.callsign)) Serial.println("Network: final word not sent");
    outgoingWord[0] = '\0';
  }
  // Warn once on the current screen; a second Back leaves anyway (stats stay in RAM).
  if (!trainer::flushStats() && notice != tr(Text::SaveError)) { notice = tr(Text::SaveError); redraw = true; return; }
  if (screen == Screen::Callsign && editor == 2) screen = Screen::Memories;
  else screen = screen == Screen::Menu ? Screen::Home : Screen::Menu;
  if (screen == Screen::Menu) { focus = 0; menuPage = 0; }
  notice = ""; redraw = true;
}
bool play(const char* message, bool visible) {
  unsigned wordGap = activity == Activity::Network ? 1 : trainer::settings().wordGap;
  if (!audio::play(message, config.speed, config.overall, wordGap)) {
    notice = tr(Text::PlayError); Serial.println(notice); redraw = true; return false;
  }
  playbackVisible = visible;
  return true;
}
char randomFrom(const char* alphabet) { return alphabet[esp_random() % strlen(alphabet)]; }
void newTarget() {
  target[0] = '\0';
  int count = config.groupSize;
  const char* source = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  switch (activity) {
    case Activity::Koch: case Activity::Adaptive: case Activity::Echo: {
      char lesson[41];
      int level = min(config.lesson, int(strlen(trainer::sequence())));
      memcpy(lesson, trainer::sequence(), level); lesson[level] = '\0';
      for (int i = 0; i < count; ++i) target[i] = activity == Activity::Adaptive ?
          trainer::stats().choose(lesson, esp_random()) : randomFrom(lesson);
      target[count] = '\0'; break;
    }
    case Activity::Numbers: source = "0123456789"; break;
    case Activity::Mixed: source = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"; break;
    case Activity::CopyOne: case Activity::Flash: count = 1; break;
    case Activity::CopyTwo: count = 2; break;
    case Activity::Words: case Activity::CopyWord: case Activity::HeadCopy:
      snprintf(target, sizeof(target), "%s", brico::kWords[trainer::settings().vocabulary][esp_random() % 20]); break;
    case Activity::Calls: case Activity::CopyCall:
      snprintf(target, sizeof(target), "EA%u%c%c%c", unsigned(esp_random() % 9 + 1),
          randomFrom("ABCDEFGHIJKLMNOPQRSTUVWXYZ"), randomFrom("ABCDEFGHIJKLMNOPQRSTUVWXYZ"), randomFrom("ABCDEFGHIJKLMNOPQRSTUVWXYZ")); break;
    case Activity::Qso:
      snprintf(target, sizeof(target), "CQ CQ DE %s K TNX QSO UR RST 599 NAME BRICO 73 SK", config.callsign); break;
    default: break;
  }
  if (!target[0]) {
    for (int i = 0; i < count; ++i) target[i] = randomFrom(source);
    target[count] = '\0';
  }
}
void loadFiles() {
  fileCount = filePage = 0;
  if (!cardReady) return;
  File root = SD.open("/");
  if (!root || !root.isDirectory()) { notice = tr(Text::FileError); Serial.println(notice); return; }
  while (fileCount < 20) {
    File entry = root.openNextFile();
    if (!entry) break;
    String name = entry.name(), lower = name;
    lower.toLowerCase();
    if (!entry.isDirectory() && lower.endsWith(".txt") && name.length() < 64)
      files[fileCount++] = name.startsWith("/") ? name : "/" + name;
    entry.close();
  }
  root.close();
}
void startActivity(Activity chosen) {
  stopPlayback(); activity = chosen; screen = Screen::Exercise;
  transcript = ""; notice = ""; paused = waitingAnswer = playingNetwork = false;
  answer[0] = outgoingWord[0] = '\0'; hits = attempts = 0;
  nextExercise = millis() + 300;
  if (activity == Activity::Card) { loadFiles(); screen = Screen::Files; }
  if (activity == Activity::Network && !networkReady()) notice = tr(Text::NetworkError);
  if (activity == Activity::Practice) notice = tr(Text::PracticePrompt);
  if (activity == Activity::Memories) { screen = Screen::Memories; memoryPage = memoryIndex = 0; }
  if (activity == Activity::Statistics) { screen = Screen::Statistics; filePage = 0; }
  redraw = true;
}
void chooseMenu(int index) {
  if (menuGroup < 2) { startActivity(menuGroup == 0 ? kReceive[index].action : kSend[index].action); return; }
  setting = index;
  draft = trainer::settings(); keyboardPage = 0;
  if (editing(Text::Callsign)) { screen = Screen::Callsign; editor = 0; strcpy(editingCall, config.callsign); }
  else if (editing(Text::CustomSet)) { screen = Screen::Callsign; editor = 1; strcpy(editingCall, draft.custom); }
  else if (editing(Text::Reset)) screen = Screen::Reset;
  else screen = Screen::Setting;
  notice = ""; redraw = true;
}
void adjust(int direction) {
  if (screen == Screen::Setting) {
    constexpr int languages = int(brico::kLanguageCount);
    if (editing(Text::Speed)) { config.speed = constrain(config.speed + direction, 3, 50); config.overall = min(config.speed, config.overall); }
    if (editing(Text::Farnsworth)) config.overall = constrain(config.overall + direction, 3, config.speed);
    if (editing(Text::Pitch)) config.pitch = constrain(config.pitch + 25 * direction, 300, 1800);
    if (editing(Text::KeyMode)) config.keyMode = (config.keyMode + direction + 3) % 3;
    if (editing(Text::Brightness)) { config.brightness = constrain(config.brightness + 5 * direction, 10, 100); board::setBrightness(config.brightness); }
    if (editing(Text::KochLevel)) config.lesson = constrain(config.lesson + direction, 2, int(strlen(trainer::sequence())));
    if (editing(Text::GroupSize)) config.groupSize = constrain(config.groupSize + direction, 1, 10);
    if (editing(Text::UiLanguage)) draft.language = (draft.language + direction + languages) % languages;
    if (editing(Text::WordLanguage)) draft.vocabulary = (draft.vocabulary + direction + languages) % languages;
    if (editing(Text::Sequence)) draft.sequence = (draft.sequence + direction + 4) % 4;
    if (editing(Text::WordGap)) draft.wordGap = constrain(draft.wordGap + direction, 1, 5);
  } else if (screen == Screen::Exercise) {
    if (activity == Activity::Network) {
      if (!networkSetDecode(direction > 0)) notice = tr(Text::SaveError);
    } else {
      config.speed = constrain(config.speed + direction, 3, 50); config.overall = min(config.overall, config.speed);
      notice = tr(Text::NewSpeed);
    }
  }
  redraw = true;
}
void flushWord() {
  if (!outgoingWord[0]) return;
  bool sent = networkPublish(outgoingWord, config.callsign);
  notice = tr(sent ? Text::Queued : Text::TxError);
  outgoingWord[0] = '\0'; redraw = true;
}
void confirm() {
  if (screen == Screen::Home) { menuGroup = focus; menuPage = focus = 0; screen = Screen::Menu; }
  else if (screen == Screen::Menu) {
    int index = menuPage * 4 + focus;
    if (index < menuCount()) chooseMenu(index);
  } else if (screen == Screen::Setting) {
    if (!save()) return;
    if (editing(Text::UiLanguage) || editing(Text::WordLanguage) || editing(Text::Sequence) || editing(Text::WordGap)) {
      auto candidate = trainer::settings();
      if (editing(Text::UiLanguage)) candidate.language = draft.language;
      if (editing(Text::WordLanguage)) candidate.vocabulary = draft.vocabulary;
      if (editing(Text::Sequence)) candidate.sequence = draft.sequence;
      if (editing(Text::WordGap)) candidate.wordGap = draft.wordGap;
      if (!trainer::save(candidate)) { notice = tr(Text::SaveError); redraw = true; return; }
    }
    back();
  }
  else if (screen == Screen::Callsign) {
    if (editor == 0) {
      if (!brico::validCall(editingCall)) { notice = tr(Text::InvalidCall); redraw = true; return; }
      strcpy(config.callsign, editingCall); if (save()) back();
    } else {
      auto candidate = trainer::settings();
      if (editor == 1) {
        if (!brico::validSet(editingCall)) { notice = tr(Text::InvalidSet); redraw = true; return; }
        strcpy(candidate.custom, editingCall); candidate.sequence = 3;
      } else if (!editingCall[0]) candidate.memories[memoryIndex][0] = '\0';
      else if (!brico::normalizeLatin(editingCall, candidate.memories[memoryIndex], sizeof(candidate.memories[memoryIndex]))) {
        notice = tr(Text::InvalidInput); redraw = true; return;
      }
      if (!trainer::save(candidate)) { notice = tr(Text::SaveError); redraw = true; return; }
      back();
    }
  } else if (screen == Screen::Reset) {
    config = Config{}; board::setBrightness(config.brightness);
    auto defaults = trainer::Settings{};
    memcpy(defaults.memories, trainer::settings().memories, sizeof(defaults.memories));
    if (save() && trainer::save(defaults)) back();
    else { notice = tr(Text::SaveError); redraw = true; }
  } else if (screen == Screen::Memories) {
    const char* message = trainer::settings().memories[memoryIndex];
    if (!*message) notice = tr(Text::EmptyMemory);
    else play(message, false);
  } else if (screen == Screen::Exercise) {
    if (activity == Activity::Network) flushWord();
    else if (activity == Activity::Flash && waitingAnswer) {
      append(target); append(' '); waitingAnswer = false; nextExercise = millis() + 2000;
    } else if (receiveActivity()) {
      paused = !paused; if (paused) audio::stop(); else nextExercise = millis();
    } else if (challengeActivity()) {
      if (waitingAnswer && (activity == Activity::Echo || activity == Activity::Adaptive)) scoreAnswer();
      else { audio::stop(); waitingAnswer = false; nextExercise = millis(); answer[0] = '\0'; }
    } else { transcript = ""; notice = ""; }
  }
  redraw = true;
}
void tap(const board::Touch& touch) {
  if (!touch.tapped) return;
  if (screen != Screen::Home && inside(touch, 6, 5, 76, 30)) { back(); return; }
  if (touch.y >= kFooter) {
    int key = brico::controlAt(touch.x, touch.y, DISPLAY_W, DISPLAY_H, BRICO_CONTROL_HEIGHT);
    if (screen == Screen::Exercise && key >= 3) return;
    if (screen == Screen::Home && key < 2) focus = (focus + (key == 0 ? 2 : 1)) % 3;
    else if (screen == Screen::Menu && key < 2) {
      int pages = (menuCount() + 3) / 4;
      menuPage = (menuPage + (key == 0 ? pages - 1 : 1)) % pages; focus = 0;
    } else if (screen == Screen::Files && key < 2) {
      int pages = max(1, (fileCount + 3) / 4);
      filePage = (filePage + (key == 0 ? pages - 1 : 1)) % pages;
    } else if (screen == Screen::Callsign && key == 0) {
      size_t length = strlen(editingCall); if (length) editingCall[length - 1] = '\0';
    } else if (screen == Screen::Callsign && key == 1) {
      keyboardPage = 1 - keyboardPage;
    } else if (screen == Screen::Memories && key < 2) {
      memoryPage = 1 - memoryPage; memoryIndex = memoryPage * 4;
    } else if (screen == Screen::Memories && key == 3) {
      audio::stop(); editor = 2; keyboardPage = 0;
      strcpy(editingCall, trainer::settings().memories[memoryIndex]); screen = Screen::Callsign;
    } else if (screen == Screen::Statistics && key < 2) {
      int count = 0; for (const auto& score : trainer::stats().scores) if (score.attempts) ++count;
      int pages = max(1, (count + 3) / 4);
      filePage = (filePage + (key == 0 ? pages - 1 : 1)) % pages;
    } else if (screen == Screen::Reset && key == 0) back();
    else if (key == 2) confirm();
    else if (key < 2) adjust(key == 0 ? -1 : 1);
    redraw = true; return;
  }
  if (screen == Screen::Home && touch.y >= 57 && touch.y < kFooter - 13) {
    focus = min(2, touch.x * 3 / DISPLAY_W); confirm();
  } else if (screen == Screen::Menu) {
    for (int i = 0; i < 4; ++i) {
      int index = menuPage * 4 + i;
      if (index < menuCount() && inside(touch, 8 + (i % 2) * DISPLAY_W / 2,
          52 + (i / 2) * 55, DISPLAY_W / 2 - 16, 47)) { focus = i; chooseMenu(index); return; }
    }
  } else if (screen == Screen::Callsign) {
    const char* keys = kKeyboard;
    int cell = DISPLAY_W / 10;
    if (touch.y >= 75 && touch.y < 187) {
      int index = keyboardPage * 40 + ((touch.y - 75) / 28) * 10 + touch.x / cell;
      size_t length = strlen(editingCall);
      size_t limit = editor == 0 ? brico::kCallSize - 1 : editor == 1 ? 40 : brico::kMaxText;
      if (index >= 0 && index < int(strlen(keys)) && length < limit) {
        editingCall[length] = keys[index]; editingCall[length + 1] = '\0'; redraw = true;
      }
    }
  } else if (screen == Screen::Memories) {
    for (int i = 0; i < 4; ++i) if (inside(touch, 8, 49 + i * 36, DISPLAY_W - 16, 32)) {
      memoryIndex = memoryPage * 4 + i; confirm(); return;
    }
  } else if (screen == Screen::Files) {
    for (int i = 0; i < 4; ++i) if (inside(touch, 8, 49 + i * 36, DISPLAY_W - 16, 32) && filePage * 4 + i < fileCount) {
      lessonFile = SD.open(files[filePage * 4 + i]);
      if (!lessonFile) { notice = tr(Text::FileError); Serial.println(notice); redraw = true; return; }
      screen = Screen::Exercise; transcript = ""; notice = ""; nextExercise = millis(); redraw = true; return;
    }
  }
}
void scoreAnswer() {
  ++attempts;
  bool correct = strcmp(answer, target) == 0;
  if (correct) ++hits;
  trainer::record(target, answer);
  notice = correct ? tr(Text::Correct) : String(tr(Text::Expected)) + ": " + target;
  answer[0] = '\0'; waitingAnswer = false; nextExercise = millis() + 2000; redraw = true;
}
void decodedLetter(char letter) {
  if (activity == Activity::Network) {
    if (letter == ' ') { flushWord(); append(' '); }
    else {
      size_t length = strlen(outgoingWord);
      if (length >= brico::kMaxText) { flushWord(); length = 0; }
      outgoingWord[length] = letter; outgoingWord[length + 1] = '\0'; append(letter);
    }
  } else if (activity == Activity::Practice) append(letter);
  else if (waitingAnswer && activity != Activity::Flash) {
    if (letter == ' ') { if (answer[0]) scoreAnswer(); }
    else {
      size_t length = strlen(answer);
      if (length < sizeof(answer) - 1) { answer[length] = letter; answer[length + 1] = '\0'; append(letter); }
      else { notice = tr(Text::TooLong); Serial.println(notice); redraw = true; }
    }
  }
}
void exerciseTick(const board::Touch& touch) {
  uint32_t now = millis();
  audio::Event event;
  while (audio::poll(event)) {
    if (event.decoded) decodedLetter(event.letter);
    else {
      if (event.letter && playbackVisible) append(event.letter);
      if (event.finished) {
        if (playingNetwork) { playingNetwork = false; append(' '); }
        else if (challengeActivity()) {
          waitingAnswer = true; answerDeadline = now + 60000; redraw = true;
        }
        else { nextExercise = now + 800; append(' '); }
      }
    }
  }
  if (audio::busy()) return;
  if (waitingAnswer && (activity == Activity::Echo || activity == Activity::Adaptive) && brico::due(now, answerDeadline)) {
    scoreAnswer(); return;
  }
  int control = brico::controlAt(touch.x, touch.y, DISPLAY_W, DISPLAY_H, BRICO_CONTROL_HEIGHT);
  bool virtualDit = touch.down && control == 3;
  bool virtualDah = touch.down && control == 4;
  bool dit = board::dit() || virtualDit, dah = board::dah() || virtualDah;
  if (activity == Activity::Network) {
    if (!dit && !dah && !audio::inputBusy() && !outgoingWord[0]) {
      brico::Message message;
      if (networkReceive(message)) {
        append(message.callsign); append(": ");
        playingNetwork = true;
        if (networkDecodeEnabled()) append(message.text);
        else append("[CW] ");
        if (!play(message.text, false)) playingNetwork = false;
        return;
      }
    }
  } else if (activity != Activity::Practice && !waitingAnswer && !paused && brico::due(now, nextExercise)) {
    if (activity == Activity::Card) {
      if (!lessonFile || !lessonFile.available()) {
        notice = tr(Text::EndFile); paused = true; redraw = true;
        if (lessonFile) lessonFile.close();
        return;
      }
      char raw[121]; size_t count = 0;
      while (lessonFile.available() && count < 110) {
        uint8_t ch = lessonFile.read();
        raw[count++] = char(ch);
        unsigned extra = ch >= 0xc2 && ch <= 0xdf ? 1 : ch >= 0xe0 && ch <= 0xef ? 2 : 0;
        while (extra-- && lessonFile.available()) raw[count++] = char(lessonFile.read());
      }
      size_t kept = brico::wholeWords(raw, count, lessonFile.available());
      if (kept < count && !lessonFile.seek(lessonFile.position() - (count - kept))) kept = count;
      count = kept;
      raw[count] = '\0';
      bool whitespace = true;
      for (size_t i = 0; i < count; ++i)
        if (raw[i] != ' ' && raw[i] != '\t' && raw[i] != '\r' && raw[i] != '\n') whitespace = false;
      if (whitespace) { nextExercise = now + 10; return; }
      if (strlen(raw) != count || !brico::normalizeLatin(raw, target, sizeof(target))) {
        notice = tr(Text::InvalidInput); Serial.println("SD: invalid UTF-8/Morse text or expanded block too long");
        paused = true; redraw = true; return;
      }
    } else newTarget();
    if (activity == Activity::CopyOne || activity == Activity::CopyTwo ||
        activity == Activity::CopyWord || activity == Activity::CopyCall) {
      append(tr(Text::Expected)); append(" "); append(target); append(": ");
    }
    bool visible = receiveActivity();
    if (!play(target, visible)) { paused = true; }
  }
}
}

void setup() {
  Serial.begin(115200);
  trainer::begin();
  board::begin();
  load();
  board::setBrightness(config.brightness);
  const uint32_t logoShownAt = millis();
  drawBootLogo();
  cardReady = board::mountCard();
  if (!networkBegin()) {
    notice = tr(Text::NetworkError);
    Serial.println(notice);
  }
  if (!audio::begin()) {
    notice = tr(Text::AudioError);
    Serial.println(notice);
  }
  // Allow the startup logo to remain readable even on a fast boot.
  while (millis() - logoShownAt < 1900) delay(10);
  redraw = true;
}

void loop() {
  networkService();
  if (settingsRevision != trainer::revision()) {
    settingsRevision = trainer::revision();
    config.lesson = min(config.lesson, int(strlen(trainer::sequence())));
    if (screen != Screen::Setting && screen != Screen::Callsign) draft = trainer::settings();
    notice = ""; redraw = true;
  }
  board::Touch touch = board::readTouch();
  tap(touch);
  static bool lastDecode = networkDecodeEnabled();
  if (lastDecode != networkDecodeEnabled()) {
    lastDecode = networkDecodeEnabled();
    if (screen == Screen::Exercise && activity == Activity::Network) {
      transcript = ""; notice = tr(Text::DecodeSaved); redraw = true;
    }
  }
  bool input = (screen == Screen::Exercise && (activity == Activity::Network ||
      activity == Activity::Practice || challengeActivity())) || (screen == Screen::Setting && editing(Text::Pitch));
  static int oldSpeed = -1, oldOverall = -1, oldPitch = -1, oldKeyMode = -1;
  static bool oldInput = false;
  if (config.speed != oldSpeed || config.overall != oldOverall || config.pitch != oldPitch ||
      config.keyMode != oldKeyMode || input != oldInput) {
    if (audio::configure(config.speed, config.overall, config.pitch, config.keyMode, input)) {
      oldSpeed = config.speed; oldOverall = config.overall; oldPitch = config.pitch;
      oldKeyMode = config.keyMode; oldInput = input;
    }
  }
  int control = brico::controlAt(touch.x, touch.y, DISPLAY_W, DISPLAY_H, BRICO_CONTROL_HEIGHT);
  bool straightTouch = touchKey.sample(touch.down,
      inside(touch, 3 * DISPLAY_W / 5 + 2, kFooter, 2 * DISPLAY_W / 5 - 4, 33),
      input && touchKeyScreen() && !audio::busy());
  audio::straightKey(straightTouch);
  bool virtualPaddles = input && !touchKeyScreen();
  audio::paddles(virtualPaddles && touch.down && control == 3, virtualPaddles && touch.down && control == 4);
  if (touchKeyDown != straightTouch) {
    touchKeyDown = straightTouch;
    if (touchKeyScreen() && !redraw) drawTouchKey();
  }
  if (screen == Screen::Exercise) exerciseTick(touch);
  else { audio::Event discarded; while (audio::poll(discarded)) {} }
  // Repaint Home only when its network status line changes, not on a timer.
  static unsigned shownPending = 0;
  if (screen == Screen::Home && networkPending() != shownPending) { shownPending = networkPending(); redraw = true; }
  if (redraw) render();
  delay(1);
}
