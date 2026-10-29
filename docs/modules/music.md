# Music player

Files: `src/music_player.h`, `src/music_player.cpp` (uses the `ESP8266Audio` library), the Music page in `src/dashboard_display.cpp`.

An optional page that plays MP3 files from the board's microSD slot through its speaker. It is a BricoHams addition, inspired by the *LVGL Music* chapter of the Freenove ESP32 Display tutorial (see [References](#references)); the code is our own and does not use LVGL.

Project: <https://github.com/Brico-Hams/Ham-Clock>

## Using it

1. Format a microSD card as FAT32 and copy MP3 files into a folder called `Music` at the card root. If `/Music` has no MP3 files, the card root is used instead.
2. Insert the card **before** powering the board. The card is read once at start-up, so restart the board after changing its contents.
3. Connect a small speaker to the board's speaker connector (2-pin JST on the CYD, speaker header on the Freenove board).
4. Move to the last page, **Music**. It is hidden when there is no card or no MP3 file on it.

| Button | Action |
| --- | --- |
| `\|<` | Previous track |
| `>` / `\|\|` | Play / pause (resuming continues where it stopped; after `\|<` or `>\|` while paused, the new track starts from the beginning) |
| `>\|` | Next track |
| `VOL-` / `VOL+` | Volume in 10 % steps (start 60 %) |

Tapping outside the button row still moves to the previous or next page. Playback carries on while you look at other pages, and continues with the next track at the end of each one (the list wraps round).

## Behaviour and limits

- Up to 64 tracks (`kMusicMaxTracks`), sorted by file name. Only `.mp3` files are listed; files and folders starting with `.` are skipped.
- MP3 tags (ID3) are skipped; the title shown is the file name without the extension.
- The Music page is never selected by **Automatic Page Change**, even if ticked, so the rotation does not pull you away from the controls.
- Decoding runs in its own FreeRTOS task (core 0, priority 2), so the main loop's network fetches do not stop the music. Expect the odd short drop-out while large pages (maps) are being drawn.
- Volume is not saved: it starts at 60 % after every restart.

## Hardware notes

| Board | SD card | Audio output |
| --- | --- | --- |
| ESP32-2432S028R 2.8" (ILI9341 or ST7789) | HSPI on 18/19/23, CS 5 | 1-bit PDM (sigma-delta) on GPIO26 into the on-board amplifier |
| CYD 4.0" ST7796S | HSPI on 18/19/23, CS 5 | 1-bit PDM on GPIO26 |
| Freenove ESP32 Display 3.2" ST7789 (FNK0103L) | HSPI on 18/19/23, CS 5 | Internal DAC on GPIO25/26; amplifier enabled by GPIO4 low |

- On the CYD boards the display uses VSPI and the SD card needs HSPI, so on the 2.8" boards the XPT2046 touch controller (own pins 25/32/39/33/36) is now read by bit-banging instead of HSPI, as the BricoHams Morse Tutor already does on the same board.
- The CYD cannot use the ESP32's internal DAC for music: DAC channel 1 is GPIO25, which is the touch clock on the 2.8" board. PDM through the amplifier is noticeably lower fidelity than the Freenove board's DAC; it is fine for speech and simple music.
- The Freenove board has no touch interrupt line; touches are detected from the XPT2046 pressure reading instead.

**Status:** builds for all four environments; not yet tested on real hardware. Please report how it behaves on your board.

## Public API

```cpp
void musicBegin();               // mount the card, list tracks, start the audio task
bool musicAvailable();           // card mounted and at least one MP3 found
uint8_t musicTrackCount();
String musicTrackTitle(uint8_t index);
int16_t musicCurrentTrack();
bool musicIsPlaying();
uint8_t musicVolumePercent();
uint32_t musicStateVersion();    // bumped on every change the page must redraw
void musicTogglePlay();
void musicNext();
void musicPrevious();
void musicChangeVolume(int8_t steps);  // ±10 % per step
```

The display only posts commands; the audio task owns the decoder, the SD file and the output, so no locking is needed beyond a single pending command.

## References

- Freenove ESP32 Display tutorial, chapter 17 *LVGL Music*: <https://docs.freenove.com/projects/fnk0103/en/latest/fnk0103/codes/tutorial/17_LVGL_Music.html>. Used only as a reference for the board's SD and audio pins. Freenove's code is released under CC BY-NC-SA 3.0, which is not compatible with this project's GPL-3.0, so none of it is copied here.
- [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio) by Earle F. Philhower III (GPL-3.0): MP3 decoding and I2S/DAC/PDM output.
