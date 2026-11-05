// Regression checks for persisted themes and the true-black TFT diagnostic.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
const source = readFileSync(new URL('../src/main.cpp', import.meta.url), 'utf8');
const language = readFileSync(new URL('../include/Languages.h', import.meta.url), 'utf8');

assert.match(language, /X\(Theme,"Tema","Theme","Design","Thème","Tema"\)/);
assert.match(language, /X\(BlackScreen,"Negro","Black","Schwarz","Noir","Nero"\)/);
assert.match(source, /Text::Brightness, Text::Theme,/);
assert.match(source, /theme = 0;/);
assert.match(source, /storage\.putInt\("theme", config\.theme\)/);
assert.match(source, /storage\.getInt\("theme", 0\)/);
assert.match(source, /void applyTheme\(\)/);
assert.match(source, /if \(editing\(Text::Theme\)\) \{ config\.theme = 1 - config\.theme; applyTheme\(\); \}/);
assert.match(source, /if \(editing\(Text::Theme\)\) value = tr\(config\.theme \? Text::Light : Text::Dark\);/);
assert.match(source, /config = Config\{\}; applyTheme\(\);/);
assert.match(source, /void text\([^\n]+color = kText\)/);
assert.match(source, /void center\([^\n]+color = kText\)/);
assert.match(source, /if \(screen == Screen::Blackout\) \{[\s\S]*?display\.fillScreen\(TFT_BLACK\);/);
assert.match(source, /screen = Screen::Blackout;\s*redraw = true;/);
assert.match(source, /screen = blackoutReturn;/);
assert.match(source, /blackoutWasPaused = paused;/);
assert.match(source, /audio::stop\(\);\s*paused = true;/);
assert.match(source, /button\(DISPLAY_W - 82, 44, 74, 28, tr\(Text::BlackScreen\)\);/);
assert.match(source, /button\(DISPLAY_W - 82, 5, 74, 30, tr\(Text::BlackScreen\)\);/);
assert.match(source, /void setup\(\) \{[\s\S]*?load\(\);\s*applyTheme\(\);/);
console.log('PASS: persistent light/dark themes and screen-clear diagnostic');
