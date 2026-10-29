#include "tutor_network.h"

#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "network_page.h"
#include "trainer_settings.h"

namespace {
constexpr uint8_t kBroadcast[] = {255, 255, 255, 255, 255, 255};
constexpr size_t kHistorySize = 12;
struct Datagram { uint8_t source[6]; uint16_t length; uint8_t data[brico::kMaxPacket]; };
struct History { brico::Message message; bool outgoing = false; };
struct Outgoing { brico::Message message; uint8_t attempts = 0; uint32_t nextSend = 0; };
WebServer server(80);
Preferences settings;
QueueHandle_t incoming = nullptr, playback = nullptr, outgoing = nullptr;
brico::Deduplicator<64> seen;
History history[kHistorySize];
size_t historyCount = 0, historyNext = 0;
uint8_t stationMac[6];
uint32_t bootSession = 0, nextSequence = 0, lastAnnouncement = 0;
bool ready = false, autoDecode = true, settingsOpen = false, espNowOpen = false;
char ssid[33] = {};
String token;
std::atomic<uint32_t> callbackDrops{0}, sendFailures{0};
uint32_t invalidPackets = 0, playbackDrops = 0;
bool activeSend = false;
Outgoing sending;

String jsonString(const char* value) {
  String result = "\"";
  for (; *value; ++value) {
    if (*value == '"' || *value == '\\') result += '\\';
    if (uint8_t(*value) >= 32) result += *value;
  }
  return result + '"';
}

void record(const brico::Message& message, bool tx) {
  history[historyNext].message = message;
  history[historyNext].outgoing = tx;
  historyNext = (historyNext + 1) % kHistorySize;
  if (historyCount < kHistorySize) ++historyCount;
}

void receiveCallback(const uint8_t* source, const uint8_t* bytes, int length) {
  if (length <= 0 || size_t(length) > brico::kMaxPacket) { ++callbackDrops; return; }
  Datagram packet;
  memcpy(packet.source, source, 6);
  packet.length = uint16_t(length);
  memcpy(packet.data, bytes, size_t(length));
  if (xQueueSend(incoming, &packet, 0) != pdTRUE) ++callbackDrops;
}

void sentCallback(const uint8_t*, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) ++sendFailures;
}

bool authorize() {
  if (server.header("X-Bricohams-Token") == token) return true;
  server.send(403, "text/plain; charset=utf-8", trainer::tr(brico::Text::Unauthorized));
  return false;
}

String catalog() {
  String result = "{";
  for (size_t i = 0; i < size_t(brico::Text::Count); ++i) {
    if (i) result += ',';
    result += jsonString(brico::kTranslations[i].key) + ":[";
    for (size_t language = 0; language < brico::kLanguageCount; ++language) {
      if (language) result += ',';
      result += jsonString(brico::kTranslations[i].values[language]);
    }
    result += ']';
  }
  return result + "}";
}
bool number(const String& input, unsigned maximum, uint8_t& value) {
  if (!input.length() || input.length() > 2) return false;
  unsigned parsed = 0;
  for (size_t i = 0; i < input.length(); ++i) {
    if (input[i] < '0' || input[i] > '9') return false;
    parsed = parsed * 10 + unsigned(input[i] - '0');
  }
  if (parsed > maximum) return false;
  value = uint8_t(parsed); return true;
}
void reply(int code, brico::Text message) {
  server.send(code, "text/plain; charset=utf-8", trainer::tr(message));
}

void serveState() {
  const auto& options = trainer::settings();
  String result;
  result.reserve(4096); // One allocation for the common case instead of dozens of regrowths.
  result += "{\"ssid\":" + jsonString(ssid) + ",\"decode\":" +
      (autoDecode ? "true" : "false") + ",\"pending\":" + String(uxQueueMessagesWaiting(playback)) +
      ",\"drops\":" + String(callbackDrops.load() + playbackDrops) + ",\"invalid\":" + String(invalidPackets) +
      ",\"sendFailures\":" + String(sendFailures.load()) +
      ",\"language\":" + options.language + ",\"vocabulary\":" + options.vocabulary +
      ",\"sequence\":" + options.sequence + ",\"wordGap\":" + options.wordGap +
      ",\"custom\":" + jsonString(options.custom) + ",\"memories\":[";
  for (int i = 0; i < 8; ++i) {
    if (i) result += ',';
    result += jsonString(options.memories[i]);
  }
  const auto& stats = trainer::stats();
  result += "],\"groups\":" + String(stats.groups) + ",\"correct\":" + String(stats.correct) + ",\"scores\":[";
  for (size_t i = 0; i < sizeof(stats.scores) / sizeof(*stats.scores); ++i) {
    if (i) result += ',';
    char symbol[] = {brico::kAlphabet[i].letter, '\0'};
    result += "[" + jsonString(symbol) + "," + stats.scores[i].attempts + "," + stats.scores[i].errors + "]";
  }
  result += "],\"messages\":[";
  for (size_t i = 0; i < historyCount; ++i) {
    const auto& entry = history[(historyNext + kHistorySize - historyCount + i) % kHistorySize];
    char morse[brico::kMaxText * 9 + 1];
    if (!brico::toMorse(entry.message.text, morse, sizeof(morse))) {
      Serial.println("Network: failed to encode stored Morse");
      server.send(500, "text/plain; charset=utf-8", trainer::tr(brico::Text::PlayError));
      return;
    }
    if (i) result += ',';
    result += "{\"from\":" + jsonString(entry.message.callsign) +
        ",\"tx\":" + (entry.outgoing ? "true" : "false") +
        ",\"morse\":" + jsonString(morse);
    if (autoDecode || entry.outgoing) result += ",\"text\":" + jsonString(entry.message.text);
    result += '}';
  }
  result += "]}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", result);
}
bool failedStartup(const char* reason) {
  Serial.println(reason);
  if (espNowOpen) { esp_now_deinit(); espNowOpen = false; }
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  if (settingsOpen) { settings.end(); settingsOpen = false; }
  if (incoming) { vQueueDelete(incoming); incoming = nullptr; }
  if (playback) { vQueueDelete(playback); playback = nullptr; }
  if (outgoing) { vQueueDelete(outgoing); outgoing = nullptr; }
  return false;
}
}

bool networkBegin() {
  if (ready) return true;
  incoming = xQueueCreate(12, sizeof(Datagram));
  playback = xQueueCreate(12, sizeof(brico::Message));
  outgoing = xQueueCreate(8, sizeof(Outgoing));
  if (!incoming || !playback || !outgoing) {
    return failedStartup("Network: could not allocate bounded message queues");
  }
  settingsOpen = settings.begin("brico-network", false);
  if (!settingsOpen) Serial.println("Network: preferences unavailable; settings cannot be saved");
  if (settingsOpen) autoDecode = settings.getBool("decode", true);
  if (!WiFi.mode(WIFI_AP_STA)) return failedStartup("Network: WiFi mode failed");
  WiFi.setSleep(false);
  WiFi.macAddress(stationMac);
  snprintf(ssid, sizeof(ssid), "Bricohams-CW-%02X%02X%02X", stationMac[3], stationMac[4], stationMac[5]);
  if (!WiFi.softAP(ssid, kNetworkPassword, brico::kNetworkChannel, false, 4)) {
    return failedStartup("Network: default WiFi access point failed");
  }
  if (esp_wifi_set_channel(brico::kNetworkChannel, WIFI_SECOND_CHAN_NONE) != ESP_OK ||
      esp_now_init() != ESP_OK) {
    return failedStartup("Network: ESP-NOW/channel initialization failed");
  }
  espNowOpen = true;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, kBroadcast, 6);
  peer.channel = brico::kNetworkChannel;
  peer.ifidx = WIFI_IF_STA;
  if (esp_now_add_peer(&peer) != ESP_OK ||
      esp_now_register_recv_cb(receiveCallback) != ESP_OK ||
      esp_now_register_send_cb(sentCallback) != ESP_OK) {
    return failedStartup("Network: broadcast peer/callback registration failed");
  }
  bootSession = esp_random();
  token = String(esp_random(), HEX) + String(esp_random(), HEX);
  const char* headers[] = {"X-Bricohams-Token"};
  server.collectHeaders(headers, 1);
  server.on("/", HTTP_GET, [] {
    String page = FPSTR(kNetworkPage);
    page.replace("@TOKEN@", token);
    page.replace("@CATALOG@", catalog());
    page.replace("@LANGUAGE@", String(trainer::settings().language));
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "text/html; charset=utf-8", page);
  });
  server.on("/api/state", HTTP_GET, serveState);
  server.on("/api/decode", HTTP_POST, [] {
    if (!authorize()) return;
    String body = server.arg("plain");
    if (body != "0" && body != "1") { reply(400, brico::Text::InvalidInput); return; }
    if (!networkSetDecode(body == "1")) { reply(500, brico::Text::SaveError); return; }
    reply(200, brico::Text::DecodeSaved);
  });
  server.on("/api/language", HTTP_POST, [] {
    if (!authorize()) return;
    auto options = trainer::settings();
    if (!number(server.arg("plain"), 4, options.language)) { reply(400, brico::Text::InvalidInput); return; }
    if (!trainer::save(options)) { reply(500, brico::Text::SaveError); return; }
    reply(200, brico::Text::Saved);
  });
  server.on("/api/trainer", HTTP_POST, [] {
    if (!authorize()) return;
    auto options = trainer::settings();
    String custom = server.arg("custom");
    if (strlen(custom.c_str()) != custom.length() || !number(server.arg("vocabulary"), 4, options.vocabulary) ||
        !number(server.arg("sequence"), 3, options.sequence) ||
        !number(server.arg("wordGap"), 5, options.wordGap) || !options.wordGap ||
        !brico::validSet(custom.c_str())) { reply(400, brico::Text::InvalidInput); return; }
    strcpy(options.custom, custom.c_str());
    if (!trainer::save(options)) { reply(500, brico::Text::SaveError); return; }
    reply(200, brico::Text::Saved);
  });
  server.on("/api/memory", HTTP_POST, [] {
    if (!authorize()) return;
    String body = server.arg("plain");
    int separator = body.indexOf('\n'); uint8_t index;
    if (strlen(body.c_str()) != body.length() || separator != 1 || !number(body.substring(0, separator), 7, index) ||
        body.length() > 2 + brico::kMaxText * 3) { reply(400, brico::Text::InvalidInput); return; }
    auto options = trainer::settings();
    String value = body.substring(separator + 1);
    if (!value.length()) options.memories[index][0] = '\0';
    else if (!brico::normalizeLatin(value.c_str(), options.memories[index], sizeof(options.memories[index]))) {
      reply(400, brico::Text::InvalidInput); return;
    }
    if (!trainer::save(options)) { reply(500, brico::Text::SaveError); return; }
    reply(200, brico::Text::Saved);
  });
  server.on("/api/send", HTTP_POST, [] {
    if (!authorize()) return;
    String body = server.arg("plain");
    int separator = body.indexOf('\n');
    if (strlen(body.c_str()) != body.length() || separator <= 0 || separator >= int(brico::kCallSize) ||
        body.length() > brico::kMaxText * 3 + brico::kCallSize + 1) {
      reply(400, brico::Text::InvalidInput); return;
    }
    String call = body.substring(0, separator);
    call.toUpperCase();
    if (!brico::validCall(call.c_str())) { reply(400, brico::Text::InvalidCall); return; }
    char normalized[brico::kMaxText + 1];
    if (!brico::normalizeLatin(body.substring(separator + 1).c_str(), normalized, sizeof(normalized))) {
      reply(400, brico::Text::InvalidInput); return;
    }
    if (!networkPublish(normalized, call.c_str())) { reply(409, brico::Text::TxError); return; }
    reply(202, brico::Text::Queued);
  });
  server.onNotFound([] { server.send(404, "text/plain", "No encontrado"); });
  server.begin();
  ready = true;
  Serial.printf("Bricohams network: SSID %s, password %s, http://%s, channel %u\n",
      ssid, kNetworkPassword, WiFi.softAPIP().toString().c_str(), brico::kNetworkChannel);
  return true;
}

void networkService() {
  if (!ready) return;
  server.handleClient();
  Datagram packet;
  for (int i = 0; i < 3 && xQueueReceive(incoming, &packet, 0) == pdTRUE; ++i) {
    brico::Message message;
    if (!brico::decode(packet.data, packet.length, packet.source, message)) {
      ++invalidPackets;
      Serial.println("Network: invalid or incompatible packet rejected");
      continue;
    }
    if (!memcmp(message.sender, stationMac, 6) || !seen.accept(message)) continue;
    record(message, false);
    if (xQueueSend(playback, &message, 0) != pdTRUE) {
      ++playbackDrops;
      Serial.println("Network: playback inbox full; message retained only in recent web history");
    }
    Serial.printf("Network: RX %s, %u bytes\n", message.callsign, unsigned(strlen(message.text)));
  }
  if (!activeSend && xQueueReceive(outgoing, &sending, 0) == pdTRUE) activeSend = true;
  if (activeSend && int32_t(millis() - sending.nextSend) >= 0) {
    uint8_t bytes[brico::kMaxPacket];
    size_t length = brico::encode(sending.message, bytes, sizeof(bytes));
    if (!length) { Serial.println("Network: internal packet encoding failed"); activeSend = false; return; }
    esp_err_t result = esp_now_send(kBroadcast, bytes, length);
    if (result != ESP_OK) {
      ++sendFailures;
      Serial.printf("Network: broadcast failed (%d)\n", int(result));
    }
    ++sending.attempts;
    sending.nextSend = millis() + 100 + esp_random() % 80;
    if (sending.attempts >= 3) activeSend = false;
  }
  if (millis() - lastAnnouncement > 10000) {
    lastAnnouncement = millis();
    if (callbackDrops || sendFailures)
      Serial.printf("Network diagnostics: callback drops=%lu, send failures=%lu\n",
          static_cast<unsigned long>(callbackDrops.load()), static_cast<unsigned long>(sendFailures.load()));
  }
}

bool networkPublish(const char* text, const char* callsign) {
  if (!ready || !brico::validCall(callsign)) { Serial.println("Network: not ready or invalid callsign"); return false; }
  Outgoing item;
  if (!brico::normalize(text, item.message.text, sizeof(item.message.text))) {
    Serial.println("Network: unsupported/empty/oversized Morse message");
    return false;
  }
  memcpy(item.message.sender, stationMac, 6);
  item.message.session = bootSession;
  item.message.sequence = ++nextSequence;
  item.nextSend = millis();
  memcpy(item.message.callsign, callsign, strlen(callsign) + 1);
  if (xQueueSend(outgoing, &item, 0) != pdTRUE) {
    Serial.println("Network: transmit queue full");
    return false;
  }
  record(item.message, true);
  return true;
}

bool networkReceive(brico::Message& message) { return ready && xQueueReceive(playback, &message, 0) == pdTRUE; }
bool networkDecodeEnabled() { return autoDecode; }
bool networkSetDecode(bool enabled) {
  if (!settingsOpen || settings.putBool("decode", enabled) != 1) {
    Serial.println("Network: could not persist decode preference");
    return false;
  }
  autoDecode = enabled;
  return true;
}
const char* networkSsid() { return ssid; }
bool networkReady() { return ready; }
unsigned networkPending() { return ready ? unsigned(uxQueueMessagesWaiting(playback)) : 0; }
