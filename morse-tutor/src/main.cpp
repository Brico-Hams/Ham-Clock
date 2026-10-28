// Copyright (c) 2026 Bricohams. New implementation, MIT licensed.
#include <Arduino.h>
#include <Preferences.h>
#include <SD.h>
#include "board_io.h"
#include "tutor_network.h"
#include "audio_engine.h"
#include "ControlLogic.h"

namespace {
using board::display;
constexpr uint16_t kBackground = 0x10c3, kCard = 0x2125, kAccent = 0xfb23, kMuted = 0xad55;
constexpr int kFooter = DISPLAY_H - BRICO_CONTROL_HEIGHT;
enum class Screen { Home, Menu, Exercise, Setting, Callsign, Reset, Files };
enum class Activity { Koch, Letters, Words, Numbers, Mixed, Card, Qso, Calls,
  Practice, CopyOne, CopyTwo, CopyWord, CopyCall, Flash, HeadCopy, Network };
struct MenuEntry { const char* label; Activity action; };
constexpr MenuEntry kReceive[] = {{"Koch", Activity::Koch}, {"Letras", Activity::Letters},
  {"Palabras", Activity::Words}, {"Numeros", Activity::Numbers}, {"Mixto", Activity::Mixed},
  {"Tarjeta SD", Activity::Card}, {"QSO", Activity::Qso}, {"Indicativos", Activity::Calls}};
constexpr MenuEntry kSend[] = {{"Practica", Activity::Practice}, {"Copiar 1", Activity::CopyOne},
  {"Copiar 2", Activity::CopyTwo}, {"Copiar palabra", Activity::CopyWord}, {"Copiar indic.", Activity::CopyCall},
  {"Tarjetas", Activity::Flash}, {"Copia mental", Activity::HeadCopy}, {"Red CW", Activity::Network}};
constexpr const char* kSettings[] = {"Velocidad", "Farnsworth", "Tono", "Manipulador", "Indicativo",
  "Brillo", "Koch nivel", "Grupo largo", "Restaurar"};
constexpr const char* kTrainingWords[] = {"RADIO", "ANTENA", "SENAL", "AMIGO", "CASA", "HOLA",
  "MAR", "SOL", "LUNA", "MONTE", "CABLE", "ONDA", "NORTE", "SUR", "ESTE", "OESTE", "CAMPO", "PUERTO"};
constexpr char kLessonOrder[] = "KMURESNAPTLWI.JZFOY,VG5/Q9H38B?427C1D6X0";
struct Config {
  int speed = 18, overall = 12, pitch = 700, brightness = 80, keyMode = 0, lesson = 2, groupSize = 5;
  char callsign[brico::kCallSize] = "NOCALL";
};
Config config;
Preferences storage;
bool storageReady = false, cardReady = false, paused = false, waitingAnswer = false, reveal = false;
bool playbackVisible = true, playingNetwork = false, redraw = true;
Screen screen = Screen::Home;
Activity activity = Activity::Letters;
int menuGroup = 0, menuPage = 0, focus = 0, setting = 0, hits = 0, attempts = 0, fileCount = 0, filePage = 0;
char target[brico::kMaxText + 1] = {}, answer[brico::kMaxText + 1] = {}, outgoingWord[brico::kMaxText + 1] = {};
char editingCall[brico::kCallSize] = {};
String transcript, notice;
String files[20];
File lessonFile;
uint32_t nextExercise = 0, lastStatusAt = 0;

bool receiveActivity() { return int(activity) <= int(Activity::Calls); }
bool challengeActivity() { return activity >= Activity::CopyOne && activity <= Activity::HeadCopy; }
bool save() {
  if (!storageReady) { notice = "Error: no se pueden guardar ajustes"; Serial.println(notice); redraw = true; return false; }
  bool ok = storage.putInt("speed", config.speed) == sizeof(int);
  ok &= storage.putInt("overall", config.overall) == sizeof(int);
  ok &= storage.putInt("pitch", config.pitch) == sizeof(int);
  ok &= storage.putInt("brightness", config.brightness) == sizeof(int);
  ok &= storage.putInt("key", config.keyMode) == sizeof(int);
  ok &= storage.putInt("lesson", config.lesson) == sizeof(int);
  ok &= storage.putInt("group", config.groupSize) == sizeof(int);
  ok &= storage.putString("call", config.callsign) == strlen(config.callsign);
  if (!ok) { notice = "Error guardando ajustes"; Serial.println(notice); }
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
  config.lesson = constrain(storage.getInt("lesson", 2), 2, int(strlen(kLessonOrder)));
  config.groupSize = constrain(storage.getInt("group", 5), 1, 10);
  String call = storage.getString("call", "NOCALL");
  if (brico::validCall(call.c_str())) memcpy(config.callsign, call.c_str(), call.length() + 1);
}
void text(const char* label, int x, int y, int size = 2, uint16_t color = TFT_WHITE) {
  display.setTextSize(size); display.setTextColor(color, kBackground);
  display.setCursor(x, y); display.print(label);
}
void center(const char* label, int x, int y, int width, int size = 2, uint16_t color = TFT_WHITE) {
  text(label, x + max(0, (width - int(strlen(label)) * size * 6) / 2), y, size, color);
}
void button(int x, int y, int width, int height, const char* label, bool selected = false) {
  display.fillRoundRect(x, y, width, height, 6, selected ? kAccent : kCard);
  display.drawRoundRect(x, y, width, height, 6, kAccent);
  int size = int(strlen(label)) * 12 > width - 8 ? 1 : 2;
  display.setTextSize(size); display.setTextColor(selected ? kBackground : TFT_WHITE, selected ? kAccent : kCard);
  display.setCursor(x + max(2, (width - int(strlen(label)) * size * 6) / 2), y + (height - size * 8) / 2);
  display.print(label);
}
bool inside(const board::Touch& touch, int x, int y, int width, int height) {
  return touch.x >= x && touch.x < x + width && touch.y >= y && touch.y < y + height;
}
void footer(const char* first, const char* second, const char* third, const char* fourth, const char* fifth) {
  const char* labels[] = {first, second, third, fourth, fifth};
  for (int i = 0; i < 5; ++i) button(i * DISPLAY_W / 5 + 2, kFooter, DISPLAY_W / 5 - 4, 33, labels[i]);
}
void header(const char* title) {
  button(6, 5, 76, 30, "< ATR");
  text(title, 94, 13, 2, kAccent);
  display.drawFastHLine(6, 41, DISPLAY_W - 12, kAccent);
}
void append(const char* value) {
  transcript += value;
  if (transcript.length() > 320) transcript.remove(0, transcript.length() - 320);
  redraw = true;
}
void append(char value) { char valueString[] = {value, '\0'}; append(valueString); }
void renderTranscript(int y = 93) {
  int columns = (DISPLAY_W - 16) / 12;
  int rows = (kFooter - y - 18) / 18;
  if (rows < 1) rows = 1;
  size_t available = size_t(columns * rows);
  String visible = transcript.length() > available ? transcript.substring(transcript.length() - available) : transcript;
  for (int row = 0; row < rows; ++row) {
    String line = visible.substring(row * columns, (row + 1) * columns);
    text(line.c_str(), 8, y + row * 18);
  }
}
const char* activityName() {
  for (const auto& entry : kReceive) if (entry.action == activity) return entry.label;
  for (const auto& entry : kSend) if (entry.action == activity) return entry.label;
  return "Ejercicio";
}
void render() {
  redraw = false;
  display.fillScreen(kBackground);
  if (screen == Screen::Home) {
    text("brico", 10, 12); text("hams", 70, 12, 2, kAccent);
    text("MORSE TUTOR 2", DISPLAY_W - 92, 18, 1, kMuted);
    display.drawFastHLine(8, 39, DISPLAY_W - 16, kAccent);
    const char* names[] = {"RECIBIR", "ENVIAR", "CONFIG"};
    const char* icons[] = {"RX", "TX", "*"};
    for (int i = 0; i < 3; ++i) {
      int width = (DISPLAY_W - 24) / 3, x = 8 + i * (DISPLAY_W - 8) / 3;
      button(x, 57, width, kFooter - 70, "", i == focus);
      center(icons[i], x, 75, width, 3, kAccent);
      center(names[i], x, 133, width, 2);
    }
    String status = networkReady() ? String("Red CW: ") + networkPending() + " pendientes" : "ERROR de red: ver Serial";
    text(status.c_str(), 8, kFooter - 15, 1, kMuted);
    footer("-", "+", "OK", "", "");
  } else if (screen == Screen::Menu) {
    const char* names[] = {"RECIBIR", "ENVIAR", "CONFIG"};
    header(names[menuGroup]);
    int count = menuGroup == 2 ? int(sizeof(kSettings) / sizeof(*kSettings)) : 8;
    for (int i = 0; i < 4; ++i) {
      int index = menuPage * 4 + i;
      if (index >= count) continue;
      const char* label = menuGroup == 0 ? kReceive[index].label : menuGroup == 1 ? kSend[index].label : kSettings[index];
      button(8 + (i % 2) * DISPLAY_W / 2, 52 + (i / 2) * 55,
          DISPLAY_W / 2 - 16, 47, label, focus == i);
    }
    String page = String(menuPage + 1) + "/" + ((count + 3) / 4);
    center(page.c_str(), 0, kFooter - 20, DISPLAY_W, 1, kMuted);
    footer("<", ">", "OK", "", "");
  } else if (screen == Screen::Exercise) {
    header(activityName());
    if (activity == Activity::Network) {
      String mode = String(networkDecodeEnabled() ? "Texto + audio" : "Solo Morse") + " | RX " + networkPending();
      text(mode.c_str(), 8, 49, 1, kAccent);
      text(networkSsid(), 8, 63, 1, kMuted);
      text("Clave: BricohamsCW | 192.168.4.1", 8, 76, 1, kMuted);
      renderTranscript(94);
      footer("CW", "TXT", "ENV", "DIT", "DAH");
    } else {
      String status = String(config.speed) + "/" + config.overall + " WPM  " + config.pitch + " Hz";
      if (challengeActivity()) status += "  " + String(hits) + "/" + attempts;
      text(status.c_str(), 8, 49, 1, kMuted);
      if (activity == Activity::Koch) text((String("Nivel ") + config.lesson).c_str(), 8, 64, 1, kAccent);
      if (waitingAnswer) text(activity == Activity::Flash ? "OK: revelar respuesta" : "Responde con llave; pausa: evaluar", 8, 76, 1, kAccent);
      renderTranscript();
      footer("-", "+", paused ? "SIG" : "OK", "DIT", "DAH");
    }
    if (notice.length()) text(notice.substring(0, (DISPLAY_W - 16) / 6).c_str(), 8, kFooter - 14, 1, kAccent);
  } else if (screen == Screen::Setting) {
    header(kSettings[setting]);
    String value;
    if (setting == 0) value = String(config.speed) + " WPM";
    if (setting == 1) value = String(config.overall) + " WPM";
    if (setting == 2) value = String(config.pitch) + " Hz";
    if (setting == 3) value = config.keyMode == 0 ? "Llave recta" : config.keyMode == 1 ? "Iambic A" : "Iambic B";
    if (setting == 5) value = String(config.brightness) + " %";
    if (setting == 6) value = String(config.lesson) + " simbolos";
    if (setting == 7) value = String(config.groupSize) + " caracteres";
    center(value.c_str(), 0, 100, DISPLAY_W, 2, kAccent);
    if (setting == 2) center("DIT: probar tono", 0, 146, DISPLAY_W, 1, kMuted);
    if (notice.length()) center(notice.c_str(), 0, kFooter - 22, DISPLAY_W, 1, kAccent);
    footer("-", "+", "GUAR", "DIT", "");
  } else if (screen == Screen::Callsign) {
    header("Indicativo");
    center(editingCall, 0, 49, DISPLAY_W, 2, kAccent);
    const char* keys = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789/";
    int cellWidth = DISPLAY_W / 10;
    for (int i = 0; i < 37; ++i) {
      char key[] = {keys[i], '\0'};
      button((i % 10) * cellWidth + 1, 75 + (i / 10) * 28, cellWidth - 2, 26, key);
    }
    if (notice.length()) text(notice.substring(0, (DISPLAY_W - 16) / 6).c_str(), 8, kFooter - 14, 1, kAccent);
    footer("BOR", "", "GUAR", "", "");
  } else if (screen == Screen::Reset) {
    header("Restaurar");
    center("Restaurar SOLO el tutor?", 0, 80, DISPLAY_W, 2, kAccent);
    center("No borra calibracion ni historial Git.", 0, 114, DISPLAY_W, 1);
    footer("NO", "", "SI", "", "");
  } else if (screen == Screen::Files) {
    header("Tarjeta SD");
    for (int i = 0; i < 4; ++i) {
      int index = filePage * 4 + i;
      if (index < fileCount) button(8, 49 + i * 36, DISPLAY_W - 16, 32, files[index].substring(0, 24).c_str());
    }
    if (!fileCount) text(cardReady ? "Sin TXT en la raiz" : "SD no disponible", 12, 85);
    footer("<", ">", "", "", "");
  }
}
void stopPlayback() { audio::stop(); }
void back() {
  stopPlayback();
  if (lessonFile) lessonFile.close();
  if (outgoingWord[0]) {
    if (!networkPublish(outgoingWord, config.callsign)) Serial.println("Network: final word not sent");
    outgoingWord[0] = '\0';
  }
  screen = screen == Screen::Menu ? Screen::Home : Screen::Menu;
  if (screen == Screen::Menu) { focus = 0; menuPage = 0; }
  notice = ""; redraw = true;
}
bool play(const char* message, bool visible) {
  if (!audio::play(message, config.speed, config.overall)) {
    notice = "Error: mensaje/velocidad no valido"; Serial.println(notice); redraw = true; return false;
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
    case Activity::Koch: {
      char lesson[sizeof(kLessonOrder)];
      memcpy(lesson, kLessonOrder, config.lesson); lesson[config.lesson] = '\0';
      for (int i = 0; i < count; ++i) target[i] = randomFrom(lesson);
      target[count] = '\0'; break;
    }
    case Activity::Numbers: source = "0123456789"; break;
    case Activity::Mixed: source = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"; break;
    case Activity::CopyOne: case Activity::Flash: count = 1; break;
    case Activity::CopyTwo: count = 2; break;
    case Activity::Words: case Activity::CopyWord: case Activity::HeadCopy:
      snprintf(target, sizeof(target), "%s", kTrainingWords[esp_random() % (sizeof(kTrainingWords) / sizeof(*kTrainingWords))]); break;
    case Activity::Calls: case Activity::CopyCall:
      snprintf(target, sizeof(target), "EA%u%c%c%c", unsigned(esp_random() % 9 + 1),
          randomFrom("ABCDEFGHIJKLMNOPQRSTUVWXYZ"), randomFrom("ABCDEFGHIJKLMNOPQRSTUVWXYZ"), randomFrom("ABCDEFGHIJKLMNOPQRSTUVWXYZ")); break;
    case Activity::Qso:
      snprintf(target, sizeof(target), "CQ CQ DE %s K TNX QSO UR RST 599 NAME BRICO QTH ESPANA 73 SK", config.callsign); break;
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
  if (!root || !root.isDirectory()) { notice = "Error abriendo SD"; Serial.println(notice); return; }
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
  transcript = ""; notice = ""; paused = waitingAnswer = reveal = playingNetwork = false;
  answer[0] = outgoingWord[0] = '\0'; hits = attempts = 0;
  nextExercise = millis() + 300;
  if (activity == Activity::Card) { loadFiles(); screen = Screen::Files; }
  if (activity == Activity::Network && !networkReady()) notice = "Error de red; revisar Serial";
  if (activity == Activity::Practice) append("Manipula DIT/DAH o llave. ");
  redraw = true;
}
void chooseMenu(int index) {
  if (menuGroup < 2) { startActivity(menuGroup == 0 ? kReceive[index].action : kSend[index].action); return; }
  setting = index;
  if (index == 4) { screen = Screen::Callsign; strcpy(editingCall, config.callsign); }
  else if (index == 8) screen = Screen::Reset;
  else screen = Screen::Setting;
  notice = ""; redraw = true;
}
void adjust(int direction) {
  if (screen == Screen::Setting) {
    if (setting == 0) { config.speed = constrain(config.speed + direction, 3, 50); config.overall = min(config.speed, config.overall); }
    if (setting == 1) config.overall = constrain(config.overall + direction, 3, config.speed);
    if (setting == 2) config.pitch = constrain(config.pitch + 25 * direction, 300, 1800);
    if (setting == 3) config.keyMode = (config.keyMode + direction + 3) % 3;
    if (setting == 5) { config.brightness = constrain(config.brightness + 5 * direction, 10, 100); board::setBrightness(config.brightness); }
    if (setting == 6) config.lesson = constrain(config.lesson + direction, 2, int(strlen(kLessonOrder)));
    if (setting == 7) config.groupSize = constrain(config.groupSize + direction, 1, 10);
  } else if (screen == Screen::Exercise) {
    if (activity == Activity::Network) {
      if (!networkSetDecode(direction > 0)) notice = "Error guardando modo RX";
    } else {
      config.speed = constrain(config.speed + direction, 3, 50); config.overall = min(config.overall, config.speed);
      notice = "Nueva velocidad: siguiente transmision";
    }
  }
  redraw = true;
}
void flushWord() {
  if (!outgoingWord[0]) return;
  bool sent = networkPublish(outgoingWord, config.callsign);
  notice = sent ? "TX en cola; entrega no confirmada" : "ERROR TX: red/cola/texto";
  outgoingWord[0] = '\0'; redraw = true;
}
void confirm() {
  if (screen == Screen::Home) { menuGroup = focus; menuPage = focus = 0; screen = Screen::Menu; }
  else if (screen == Screen::Menu) {
    int index = menuPage * 4 + focus;
    if (index < (menuGroup == 2 ? 9 : 8)) chooseMenu(index);
  } else if (screen == Screen::Setting) { if (save()) back(); }
  else if (screen == Screen::Callsign) {
    if (!brico::validCall(editingCall)) { notice = "Indicativo invalido"; Serial.println(notice); return; }
    strcpy(config.callsign, editingCall); if (save()) back();
  } else if (screen == Screen::Reset) {
    config = Config{}; board::setBrightness(config.brightness); if (save()) back();
  } else if (screen == Screen::Exercise) {
    if (activity == Activity::Network) flushWord();
    else if (activity == Activity::Flash && waitingAnswer) {
      append(target); append(' '); waitingAnswer = false; nextExercise = millis() + 2000;
    } else if (receiveActivity()) {
      paused = !paused; if (paused) audio::stop(); else nextExercise = millis();
    } else if (challengeActivity()) {
      waitingAnswer = false; nextExercise = millis(); answer[0] = '\0';
    } else { transcript = ""; notice = ""; }
  }
  redraw = true;
}
void tap(const board::Touch& touch) {
  if (!touch.tapped) return;
  if (screen != Screen::Home && inside(touch, 6, 5, 76, 30)) { back(); return; }
  if (touch.y >= kFooter) {
    int key = brico::controlAt(touch.x, touch.y, DISPLAY_W, DISPLAY_H, BRICO_CONTROL_HEIGHT);
    if (screen == Screen::Home && key < 2) focus = (focus + (key == 0 ? 2 : 1)) % 3;
    else if (screen == Screen::Menu && key < 2) {
      int pages = menuGroup == 2 ? 3 : 2;
      menuPage = (menuPage + (key == 0 ? pages - 1 : 1)) % pages; focus = 0;
    } else if (screen == Screen::Files && key < 2) {
      int pages = max(1, (fileCount + 3) / 4);
      filePage = (filePage + (key == 0 ? pages - 1 : 1)) % pages;
    } else if (screen == Screen::Callsign && key == 0) {
      size_t length = strlen(editingCall); if (length) editingCall[length - 1] = '\0';
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
      if (index < (menuGroup == 2 ? 9 : 8) && inside(touch, 8 + (i % 2) * DISPLAY_W / 2,
          52 + (i / 2) * 55, DISPLAY_W / 2 - 16, 47)) { focus = i; chooseMenu(index); return; }
    }
  } else if (screen == Screen::Callsign) {
    const char* keys = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789/";
    int cell = DISPLAY_W / 10;
    if (touch.y >= 75 && touch.y < 187) {
      int index = ((touch.y - 75) / 28) * 10 + touch.x / cell;
      size_t length = strlen(editingCall);
      if (index >= 0 && index < 37 && length < brico::kCallSize - 1) {
        editingCall[length] = keys[index]; editingCall[length + 1] = '\0'; redraw = true;
      }
    }
  } else if (screen == Screen::Files) {
    for (int i = 0; i < 4; ++i) if (inside(touch, 8, 49 + i * 36, DISPLAY_W - 16, 32) && filePage * 4 + i < fileCount) {
      lessonFile = SD.open(files[filePage * 4 + i]);
      if (!lessonFile) { notice = "Error abriendo archivo"; Serial.println(notice); redraw = true; return; }
      screen = Screen::Exercise; transcript = ""; notice = ""; nextExercise = millis(); redraw = true; return;
    }
  }
}
void scoreAnswer() {
  ++attempts;
  bool correct = strcmp(answer, target) == 0;
  if (correct) ++hits;
  notice = correct ? "Correcto!" : String("Era: ") + target;
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
      else { notice = "Respuesta demasiado larga"; Serial.println(notice); redraw = true; }
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
        else if (challengeActivity()) { waitingAnswer = true; redraw = true; }
        else { nextExercise = now + 800; append(' '); }
      }
    }
  }
  if (audio::busy()) return;
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
        notice = "Fin de archivo"; paused = true; redraw = true;
        if (lessonFile) lessonFile.close();
        return;
      }
      char raw[121]; size_t count = 0;
      while (lessonFile.available() && count < 110) {
        char ch = lessonFile.read();
        if (ch >= 'a' && ch <= 'z') ch -= 32;
        if (ch == '\r' || ch == '\n' || ch == '\t') ch = ' ';
        if (brico::supported(ch)) raw[count++] = ch;
        else { Serial.println("SD: unsupported character skipped"); }
      }
      raw[count] = '\0';
      if (!brico::normalize(raw, target, sizeof(target))) { nextExercise = now + 10; return; }
    } else newTarget();
    if (activity == Activity::CopyOne || activity == Activity::CopyTwo ||
        activity == Activity::CopyWord || activity == Activity::CopyCall) {
      append("Copia "); append(target); append(": ");
    }
    bool visible = receiveActivity();
    if (!play(target, visible)) { paused = true; }
  }
}
}

void setup() {
  Serial.begin(115200);
  board::begin();
  load();
  board::setBrightness(config.brightness);
  cardReady = board::mountCard();
  if (!networkBegin()) {
    notice = "Error de red: tutor local disponible";
    Serial.println(notice);
  }
  if (!audio::begin()) {
    notice = "ERROR de audio; reinicia y revisa Serial";
    Serial.println(notice);
  }
  redraw = true;
}

void loop() {
  networkService();
  board::Touch touch = board::readTouch();
  tap(touch);
  static bool lastDecode = networkDecodeEnabled();
  if (lastDecode != networkDecodeEnabled()) {
    lastDecode = networkDecodeEnabled();
    if (screen == Screen::Exercise && activity == Activity::Network) {
      transcript = ""; notice = "Modo RX actualizado"; redraw = true;
    }
  }
  bool input = (screen == Screen::Exercise && (activity == Activity::Network ||
      activity == Activity::Practice || waitingAnswer)) || (screen == Screen::Setting && setting == 2);
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
  audio::paddles(input && touch.down && control == 3, input && touch.down && control == 4);
  if (screen == Screen::Exercise) exerciseTick(touch);
  else { audio::Event discarded; while (audio::poll(discarded)) {} }
  if (screen == Screen::Home && millis() - lastStatusAt > 1500) { lastStatusAt = millis(); redraw = true; }
  if (redraw) render();
  delay(1);
}
