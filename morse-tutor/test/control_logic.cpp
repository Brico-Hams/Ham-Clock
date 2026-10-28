#include "ControlLogic.h"
#include "BoardConfig.h"

using brico::controlAt;
using brico::scaleAxis;
static_assert(controlAt(0, DISPLAY_H - BRICO_CONTROL_HEIGHT, DISPLAY_W, DISPLAY_H, BRICO_CONTROL_HEIGHT) == 0,
              "Actual footer first pixel");
static_assert(controlAt(DISPLAY_W - 1, DISPLAY_H - 1, DISPLAY_W, DISPLAY_H, BRICO_CONTROL_HEIGHT) == 4,
              "Actual footer last pixel");
static_assert(controlAt(0, DISPLAY_H - BRICO_CONTROL_HEIGHT - 1, DISPLAY_W, DISPLAY_H, BRICO_CONTROL_HEIGHT) == -1,
              "Actual body does not activate footer");
#if BRICO_PANEL == 778932
static_assert(BRICO_DIT_PIN == -1 && BRICO_DAH_PIN == -1, "Freenove connector is not assumed");
static_assert(TFT_BL == 27 && TOUCH_IRQ == -1 && BRICO_AUDIO_ENABLE == 4, "Verified Freenove profile");
#endif
static_assert(controlAt(0, 208, 320, 240, 32) == 0, "Left button");
static_assert(controlAt(63, 239, 320, 240, 32) == 0, "Left boundary");
static_assert(controlAt(64, 208, 320, 240, 32) == 1, "Right button");
static_assert(controlAt(128, 208, 320, 240, 32) == 2, "OK button");
static_assert(controlAt(192, 208, 320, 240, 32) == 3, "DIT button");
static_assert(controlAt(319, 239, 320, 240, 32) == 4, "DAH button");
static_assert(controlAt(320, 239, 320, 240, 32) == -1, "Reject right edge");
static_assert(controlAt(-1, 239, 320, 240, 32) == -1, "Reject negative X");
static_assert(controlAt(120, 207, 320, 240, 32) == -1, "Body is not a button");
static_assert(controlAt(120, 240, 320, 240, 32) == -1, "Reject bottom edge");
static_assert(controlAt(479, 319, 480, 320, 32) == 4, "Four-inch DAH");
static_assert(controlAt(192, 288, 480, 320, 32) == 2, "Four-inch OK");
static_assert(scaleAxis(400, 400, 3600, 30, 289) == 30, "Calibration start");
static_assert(scaleAxis(3600, 400, 3600, 30, 289) == 289, "Calibration end");
static_assert(scaleAxis(2000, 400, 3600, 30, 290) == 160, "Calibration midpoint");
static_assert(scaleAxis(400, 3600, 400, 30, 289) == 289, "Reversed axis");
static_assert(scaleAxis(3600, 3600, 400, 30, 289) == 30, "Reversed start");
static_assert(brico::clamp(-12, 0, 319) == 0, "Clamp lower");
static_assert(brico::clamp(340, 0, 319) == 319, "Clamp upper");
static_assert(BRICO_AUDIO_CHANNEL / 2 != BRICO_BACKLIGHT_CHANNEL / 2,
              "Audio and backlight must not share an LEDC timer");
static_assert(DISPLAY_H - BRICO_CONTROL_HEIGHT > 180, "Usable body height");
