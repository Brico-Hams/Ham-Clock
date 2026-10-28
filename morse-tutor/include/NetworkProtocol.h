#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "MorseCodec.h"

namespace brico {
constexpr size_t kMaxText = 120;
constexpr size_t kCallSize = 10;
constexpr size_t kHeaderSize = 27;
constexpr size_t kMaxPacket = kHeaderSize + kMaxText + 4;
constexpr uint8_t kNetworkChannel = 1;
constexpr uint8_t kProtocolVersion = 2;

struct Message {
  uint8_t sender[6] = {};
  uint32_t session = 0;
  uint32_t sequence = 0;
  char callsign[kCallSize] = {};
  char text[kMaxText + 1] = {};
};

inline bool supported(char ch) {
  return ch == ' ' || elementsFor(ch) != nullptr;
}

inline bool normalize(const char* input, char* output, size_t capacity) {
  size_t count = 0;
  bool space = false;
  if (!input || !output || capacity == 0) return false;
  for (; *input; ++input) {
    char ch = *input;
    if (ch >= 'a' && ch <= 'z') ch -= 'a' - 'A';
    if (ch == '\r' || ch == '\n' || ch == '\t') ch = ' ';
    if (!supported(ch)) return false;
    if (ch == ' ') { space = count != 0; continue; }
    if (count + (space ? 1 : 0) + 1 >= capacity) return false;
    if (space) output[count++] = ' ';
    output[count++] = ch;
    space = false;
  }
  output[count] = '\0';
  return count != 0;
}

inline bool validCall(const char* call) {
  if (!call) return false;
  size_t length = 0;
  for (; length < kCallSize && call[length]; ++length)
    if (!((call[length] >= 'A' && call[length] <= 'Z') ||
          (call[length] >= '0' && call[length] <= '9') || call[length] == '/')) return false;
  return length > 0 && length < kCallSize;
}

inline uint32_t checksum(const uint8_t* data, size_t length) {
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < length; ++i) hash = (hash ^ data[i]) * 16777619u;
  return hash;
}

inline void write32(uint8_t* bytes, uint32_t value) {
  for (int i = 0; i < 4; ++i) bytes[i] = uint8_t(value >> (i * 8));
}
inline uint32_t read32(const uint8_t* bytes) {
  return uint32_t(bytes[0]) | uint32_t(bytes[1]) << 8 |
      uint32_t(bytes[2]) << 16 | uint32_t(bytes[3]) << 24;
}

inline size_t encode(const Message& message, uint8_t* data, size_t capacity) {
  size_t length = 0;
  while (length <= kMaxText && message.text[length]) ++length;
  if (!length || length > kMaxText || !validCall(message.callsign) ||
      capacity < kHeaderSize + length + 4) return 0;
  for (size_t i = 0; i < length; ++i) if (!supported(message.text[i])) return 0;
  memset(data, 0, kHeaderSize);
  data[0] = 'B'; data[1] = 'H'; data[2] = kProtocolVersion;
  memcpy(data + 3, message.sender, 6);
  write32(data + 9, message.session);
  write32(data + 13, message.sequence);
  memcpy(data + 17, message.callsign, strlen(message.callsign));
  data[26] = uint8_t(length);
  memcpy(data + kHeaderSize, message.text, length);
  write32(data + kHeaderSize + length, checksum(data, kHeaderSize + length));
  return kHeaderSize + length + 4;
}

inline bool decode(const uint8_t* data, size_t length, const uint8_t* source, Message& message) {
  if (!data || !source || length < kHeaderSize + 5 || length > kMaxPacket ||
      data[0] != 'B' || data[1] != 'H' || data[2] != kProtocolVersion ||
      data[26] == 0 || data[26] > kMaxText || length != kHeaderSize + data[26] + 4 ||
      memcmp(data + 3, source, 6) != 0 ||
      checksum(data, length - 4) != read32(data + length - 4)) return false;
  Message result;
  memcpy(result.sender, data + 3, 6);
  result.session = read32(data + 9);
  result.sequence = read32(data + 13);
  memcpy(result.callsign, data + 17, kCallSize - 1);
  if (!validCall(result.callsign)) return false;
  for (size_t i = strlen(result.callsign); i < kCallSize - 1; ++i)
    if (data[17 + i] != 0) return false;
  for (size_t i = 0; i < data[26]; ++i) {
    if (!supported(char(data[kHeaderSize + i]))) return false;
    result.text[i] = char(data[kHeaderSize + i]);
  }
  message = result;
  return true;
}

template<size_t Capacity>
class Deduplicator {
  struct Key { uint8_t mac[6] = {}; uint32_t session = 0, sequence = 0; bool used = false; };
  Key keys_[Capacity] = {};
  size_t next_ = 0;
public:
  bool accept(const Message& message) {
    for (const auto& key : keys_)
      if (key.used && !memcmp(key.mac, message.sender, 6) &&
          key.session == message.session && key.sequence == message.sequence) return false;
    auto& key = keys_[next_];
    memcpy(key.mac, message.sender, 6);
    key.session = message.session;
    key.sequence = message.sequence;
    key.used = true;
    next_ = (next_ + 1) % Capacity;
    return true;
  }
};

inline bool toMorse(const char* text, char* output, size_t capacity) {
  size_t count = 0;
  if (!capacity) return false;
  for (size_t i = 0; text[i]; ++i) {
    if (!supported(text[i])) return false;
    if (text[i] == ' ') {
      if (count + 2 >= capacity) return false;
      output[count++] = '/'; output[count++] = ' ';
    } else {
      for (const char* code = elementsFor(text[i]); *code; ++code) {
        if (count + 1 >= capacity) return false;
        output[count++] = *code;
      }
      if (count + 1 >= capacity) return false;
      output[count++] = ' ';
    }
  }
  output[count] = '\0';
  return true;
}
}
