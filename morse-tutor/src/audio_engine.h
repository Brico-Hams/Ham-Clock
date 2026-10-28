#pragma once
#include <stdint.h>

namespace audio {
struct Event { char letter = '\0'; bool finished = false, decoded = false; uint32_t generation = 0; };
bool begin();
bool configure(int speed, int overall, int pitch, int keyMode, bool inputEnabled);
bool play(const char* text, int speed, int overall);
void stop();
bool busy();
bool inputBusy();
bool poll(Event& event);
void paddles(bool dit, bool dah);
}
