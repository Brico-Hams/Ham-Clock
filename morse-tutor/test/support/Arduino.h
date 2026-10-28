#pragma once
struct TestSerial {
  unsigned messages = 0;
  void println(const char*) { ++messages; }
};
extern TestSerial Serial;
