#pragma once
#include <TFT_eSPI.h>

namespace board {
extern TFT_eSPI display;
struct Touch { bool down = false, tapped = false; int x = 0, y = 0; };
void begin();
Touch readTouch();
void setBrightness(int percent);
void tone(bool enabled, int frequency);
bool dit();
bool dah();
bool mountCard();
}
