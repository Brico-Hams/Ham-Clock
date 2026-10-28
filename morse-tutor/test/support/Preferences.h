#pragma once
#include <map>
#include <string>
#include <vector>
#include <string.h>

namespace testStorage {
inline std::map<std::string, std::vector<unsigned char>>& values() {
  static std::map<std::string, std::vector<unsigned char>> data;
  return data;
}
inline bool& failWrite() { static bool fail = false; return fail; }
}
class Preferences {
public:
  bool begin(const char*, bool) { return true; }
  void end() {}
  bool isKey(const char* key) { return testStorage::values().count(key) != 0; }
  size_t getBytesLength(const char* key) { return testStorage::values()[key].size(); }
  size_t getBytes(const char* key, void* output, size_t capacity) {
    const auto& value = testStorage::values()[key];
    if (value.size() > capacity) return 0;
    memcpy(output, value.data(), value.size()); return value.size();
  }
  size_t putBytes(const char* key, const void* input, size_t size) {
    if (testStorage::failWrite()) return 0;
    const auto* bytes = static_cast<const unsigned char*>(input);
    testStorage::values()[key] = std::vector<unsigned char>(bytes, bytes + size);
    return size;
  }
};
