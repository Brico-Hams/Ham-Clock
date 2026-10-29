#pragma once

#include <Arduino.h>

// MP3 player for the Music page: tracks from /Music (or the card root) on the
// microSD slot, played through the board's speaker. Decoding runs in its own
// FreeRTOS task so slow network fetches in the main loop do not stutter it.

// Maximum number of tracks listed from the card.
constexpr uint8_t kMusicMaxTracks = 64;

// Mounts the card and lists the tracks. Safe to call once at start-up; the
// page stays hidden when no card or no MP3 file is found.
void musicBegin();

bool musicAvailable();
uint8_t musicTrackCount();
// File name without folder and extension, for display.
String musicTrackTitle(uint8_t index);
int16_t musicCurrentTrack();
bool musicIsPlaying();
uint8_t musicVolumePercent();
// Bumped on every change the page has to redraw.
uint32_t musicStateVersion();

void musicTogglePlay();
void musicNext();
void musicPrevious();
void musicChangeVolume(int8_t steps);
