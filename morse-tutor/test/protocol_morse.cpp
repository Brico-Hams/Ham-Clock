#include "NetworkProtocol.h"
#include "TonePlayer.h"
#include "ControlLogic.h"
#include <assert.h>
#include <stdio.h>
#include <string>
#include <vector>

namespace {
void protocol() {
  brico::Message message, decoded;
  const uint8_t mac[] = {2, 1, 2, 3, 4, 5};
  memcpy(message.sender, mac, 6);
  message.session = 0xff012345; message.sequence = 0xffffffff;
  strcpy(message.callsign, "EA1ABC/P");
  assert(brico::normalize("  cq  cq\tde ea1abc \r\n", message.text, sizeof(message.text)));
  assert(strcmp(message.text, "CQ CQ DE EA1ABC") == 0);
  uint8_t bytes[brico::kMaxPacket];
  size_t length = brico::encode(message, bytes, sizeof(bytes));
  assert(length == brico::kHeaderSize + strlen(message.text) + 4);
  assert(brico::decode(bytes, length, mac, decoded));
  assert(decoded.session == message.session && decoded.sequence == message.sequence);
  assert(!strcmp(decoded.callsign, message.callsign) && !strcmp(decoded.text, message.text));
  for (size_t size = 0; size < length; ++size) assert(!brico::decode(bytes, size, mac, decoded));
  for (size_t i = 0; i < length; ++i) {
    bytes[i] ^= 0x80;
    assert(!brico::decode(bytes, length, mac, decoded));
    bytes[i] ^= 0x80;
  }
  uint8_t otherMac[] = {2, 1, 2, 3, 4, 6};
  assert(!brico::decode(bytes, length, otherMac, decoded));
  assert(!brico::decode(nullptr, length, mac, decoded));
  assert(!brico::decode(bytes, length, nullptr, decoded));
  assert(!brico::validCall(nullptr));
  assert(!brico::validCall("") && brico::validCall("LOWER"));
  assert(!brico::validCall("lower") && !brico::validCall("1234567890") && !brico::validCall("EA 1"));
  assert(brico::validCall("123456789"));
  assert(!brico::normalize("", message.text, sizeof(message.text)));
  assert(!brico::normalize("   ", message.text, sizeof(message.text)));
  assert(!brico::normalize("RADIO#1", message.text, sizeof(message.text)));
  assert(!brico::normalize("PE\xC3\x91""A", message.text, sizeof(message.text)));
  std::string maximum(120, 'E');
  assert(brico::normalize((maximum + " \t  ").c_str(), message.text, sizeof(message.text)));
  assert(strlen(message.text) == 120);
  length = brico::encode(message, bytes, sizeof(bytes));
  assert(length == brico::kMaxPacket);
  assert(brico::decode(bytes, length, mac, decoded));
  assert(!brico::encode(message, bytes, sizeof(bytes) - 1));
  assert(!brico::normalize((maximum + "E").c_str(), message.text, sizeof(message.text)));
  assert(!brico::normalize((maximum + " E").c_str(), message.text, sizeof(message.text)));
  strcpy(message.text, "CQ");
  strcpy(message.callsign, "EA1");
  length = brico::encode(message, bytes, sizeof(bytes));
  bytes[21] = 'X';
  brico::write32(bytes + length - 4, brico::checksum(bytes, length - 4));
  assert(!brico::decode(bytes, length, mac, decoded));
  length = brico::encode(message, bytes, sizeof(bytes));
  bytes[brico::kHeaderSize] = 0;
  brico::write32(bytes + length - 4, brico::checksum(bytes, length - 4));
  assert(!brico::decode(bytes, length, mac, decoded));

  brico::Deduplicator<4> seen;
  assert(seen.accept(message) && !seen.accept(message));
  message.sequence = 0; assert(seen.accept(message));
  ++message.session; assert(seen.accept(message));
  ++message.sender[5]; assert(seen.accept(message));
  ++message.sequence; assert(seen.accept(message));
  assert(!seen.accept(message));
}
void broadcast() {
  brico::Deduplicator<64> receivers[3];
  int received[3] = {};
  for (int sender = 0; sender < 3; ++sender) {
    brico::Message message;
    message.sender[0] = 2; message.sender[5] = sender + 1;
    message.session = 99; message.sequence = 1;
    strcpy(message.callsign, "BRICO"); strcpy(message.text, "HOLA RED 73");
    uint8_t packet[brico::kMaxPacket];
    size_t size = brico::encode(message, packet, sizeof(packet));
    for (int retry = 0; retry < 3; ++retry) for (int receiver = 0; receiver < 3; ++receiver) {
      brico::Message parsed;
      assert(brico::decode(packet, size, message.sender, parsed));
      if (receiver != sender && receivers[receiver].accept(parsed)) ++received[receiver];
    }
  }
  for (int count : received) assert(count == 2);
}
void codec() {
  for (const auto& symbol : brico::kAlphabet) {
    assert(brico::letterFor(symbol.elements) == symbol.letter);
    assert(!strcmp(brico::elementsFor(symbol.letter), symbol.elements));
  }
  assert(brico::elementsFor('#') == nullptr);
  assert(brico::letterFor("........") == '?');
  char morse[100];
  assert(brico::toMorse("SOS 73", morse, sizeof(morse)));
  assert(!strcmp(morse, "... --- ... / --... ...-- "));
  assert(!brico::toMorse("SOS", morse, 3));
  assert(!brico::toMorse("#", morse, sizeof(morse)));
  assert(brico::due(10, 0xfffffff0));
  assert(!brico::due(0xfffffff0, 10));
}
void straightKey() {
  brico::KeyDecoder decoder;
  decoder.reset(0);
  assert(!decoder.sample(true, 0, 100));
  assert(!decoder.sample(false, 100, 100));
  assert(!decoder.sample(false, 399, 100));
  assert(decoder.sample(false, 400, 100) == 'E');
  assert(decoder.sample(false, 800, 100) == ' ');
  assert(!decoder.sample(false, 1000, 100));
  decoder.reset(0xfffffff0);
  assert(!decoder.sample(true, 0xfffffff0, 100));
  assert(!decoder.sample(false, 284, 100));
  assert(decoder.sample(false, 584, 100) == 'T');
  decoder.reset(0);
  decoder.sample(true, 0, 100); decoder.sample(false, 4, 100);
  assert(!decoder.sample(false, 1000, 100));
  decoder.reset(0);
  for (int n = 0; n < 3; ++n) {
    decoder.sample(true, n * 200, 100);
    decoder.sample(false, n * 200 + 100, 100);
  }
  assert(decoder.sample(false, 800, 100) == 'S');
  decoder.reset(0);
  decoder.sample(true, 0, 100); decoder.sample(false, 100, 100);
  assert(decoder.sample(true, 400, 100) == 'E');
  decoder.sample(false, 500, 100);
  assert(decoder.sample(false, 800, 100) == 'E');
}
void touchStraightKey() {
  brico::TouchStraightKey key;
  assert(key.sample(true, true, true));
  assert(key.sample(true, true, true));
  assert(!key.sample(true, false, true)); // Dragging off cancels until release.
  assert(!key.sample(true, true, true));
  assert(!key.sample(false, true, true));
  assert(!key.sample(true, false, true));
  assert(!key.sample(true, true, true)); // Dragging in must not start a mark.
  assert(!key.sample(false, false, true));
  assert(!key.sample(true, true, false)); // Playback or navigation disables input.
  assert(!key.sample(true, true, true));
  assert(!key.sample(false, true, true));
  assert(key.sample(true, true, true));
  assert(!key.sample(true, true, false));
  assert(!key.sample(true, true, true));
  assert(!key.sample(false, true, true));
  brico::KeyDecoder decoder;
  decoder.reset(0);
  std::string decoded;
  // Hold 100ms for a dot, 300ms for a dash, then seven units of word silence.
  for (uint32_t now = 0; now <= 2000; now++) {
    bool down = now < 100 || (now >= 900 && now < 1200);
    char ch = decoder.sample(key.sample(down, true, true), now, 100);
    if (ch) decoded += ch;
  }
  assert(decoded == "E T ");
  brico::TouchStraightKey messageKey;
  decoder.reset(0);
  std::vector<bool> presses;
  for (const char* letter = "SOS"; *letter; ++letter) {
    for (const char* mark = brico::elementsFor(*letter); *mark; ++mark) {
      presses.insert(presses.end(), *mark == '.' ? 100 : 300, true);
      presses.insert(presses.end(), 100, false);
    }
    presses.insert(presses.end(), 200, false);
  }
  presses.insert(presses.end(), 401, false);
  decoded.clear();
  for (size_t now = 0; now < presses.size(); ++now) {
    char ch = decoder.sample(messageKey.sample(presses[now], true, true), uint32_t(now), 100);
    if (ch) decoded += ch;
  }
  assert(decoded == "SOS ");
  brico::Message message, received;
  message.sender[0] = 2;
  strcpy(message.callsign, "NOCALL");
  assert(brico::normalize(decoded.c_str(), message.text, sizeof(message.text)));
  uint8_t bytes[brico::kMaxPacket];
  size_t size = brico::encode(message, bytes, sizeof(bytes));
  assert(brico::decode(bytes, size, message.sender, received));
  assert(!strcmp(received.text, "SOS"));
}
void debounce() {
  brico::Debouncer contact;
  assert(!contact.sample(true, 0, 5));
  assert(!contact.sample(false, 2, 5)); // Bounce restarts the hold.
  assert(!contact.sample(true, 3, 5));
  assert(!contact.sample(true, 7, 5));
  assert(contact.sample(true, 8, 5));
  assert(contact.sample(false, 100, 5) && contact.sample(true, 102, 5));
  assert(contact.sample(false, 103, 5) && !contact.sample(false, 108, 5));
  brico::Debouncer wrapped;
  assert(!wrapped.sample(true, 0xfffffffe, 5));
  assert(wrapped.sample(true, 3, 5));
  // A bouncy 300 ms press keyed through the debouncer still decodes as one dash.
  brico::Debouncer key;
  brico::KeyDecoder decoder;
  decoder.reset(0);
  std::string decoded;
  for (uint32_t now = 0; now <= 1200; ++now) {
    bool raw = (now < 300 && now % 7 != 3) || (now >= 300 && now < 304 && now % 2);
    char ch = decoder.sample(key.sample(raw, now, 5), now, 100);
    if (ch) decoded += ch;
  }
  assert(decoded == "T ");
}
std::vector<int> squeeze(bool modeB, int release) {
  brico::IambicKeyer keyer;
  std::vector<int> edges;
  bool previous = false;
  for (int now = 0; now <= 1100; ++now) {
    bool down = now < release;
    bool value = keyer.sample(down, down, modeB, now, 100);
    if (value != previous) edges.push_back(now);
    previous = value;
  }
  return edges;
}
void keyer() {
  assert((squeeze(false, 50) == std::vector<int>{0, 100}));
  assert((squeeze(false, 150) == std::vector<int>{0, 100}));
  assert((squeeze(true, 50) == std::vector<int>{0, 100, 200, 500}));
  assert((squeeze(false, 650) == std::vector<int>{0, 100, 200, 500, 600, 700}));
  assert((squeeze(true, 650) == std::vector<int>{0, 100, 200, 500, 600, 700, 800, 1100}));
  brico::IambicKeyer key;
  assert(key.sample(false, true, false, 0xfffffff0, 100));
  assert(key.sample(false, false, false, 283, 100));
  assert(!key.sample(false, false, false, 284, 100));
  key.reset();
  assert(key.sample(true, false, false, 0, 100));
  assert(key.sample(false, true, false, 50, 100));
  assert(!key.sample(false, false, false, 100, 100));
  assert(key.sample(false, false, false, 200, 100)); // A remembers a single opposite-paddle tap.
  assert(!key.sample(false, false, false, 500, 100));
}
void playback() {
  brico::TonePlayer player;
  assert(!player.start("CQ#", 12, 12, 0));
  assert(!player.start("   ", 12, 12, 0));
  assert(!player.start("CQ", 0, 12, 0));
  assert(!player.start("CQ", 12, 18, 0));
  assert(player.start("PARIS ", 12, 12, 0));
  std::string emitted;
  int onMilliseconds = 0, completedAt = -1;
  for (uint32_t now = 0; now <= 6000; ++now) {
    player.tick(now);
    char ch = player.takeLetter(); if (ch) emitted += ch;
    if (player.sounding()) ++onMilliseconds;
    if (!player.busy()) { completedAt = now; break; }
  }
  assert(emitted == "PARIS ");
  assert(completedAt == 5000 && onMilliseconds == 2200);
  assert(player.start("PARIS ", 20, 10, 0));
  for (uint32_t now = 0; now < 6500; ++now) {
    player.tick(now);
    if (!player.busy()) { completedAt = now; break; }
  }
  assert(completedAt >= 5980 && completedAt <= 6000);
  assert(player.start("E", 12, 12, 0xffffffc0));
  player.tick(0xffffffc0); assert(player.sounding());
  player.tick(35); assert(player.sounding());
  player.tick(36); assert(!player.sounding());
  player.tick(336); assert(!player.busy());
  assert(player.start("E", 12, 12, 0)); player.stop(); assert(!player.busy() && !player.sounding());
}
}
int main() {
  protocol(); broadcast(); codec(); straightKey(); touchStraightKey(); debounce(); keyer(); playback();
  puts("PASS: packet boundaries/corruption, three-node retry deduplication, Morse alphabet, touch straight-key holds/cancellation, key debounce, straight/iambic keys, PARIS/Farnsworth timing and rollover");
}
