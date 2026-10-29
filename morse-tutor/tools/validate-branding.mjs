// Static regression checks for boot branding and user-configurable callsign.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
const source = readFileSync(new URL('../src/main.cpp', import.meta.url), 'utf8');
const config = readFileSync(new URL('../include/NetworkProtocol.h', import.meta.url), 'utf8');
assert.match(source, /char callsign\[brico::kCallSize\] = "NOCALL";/);
assert.match(source, /storage\.getString\("call", "NOCALL"\)/);
assert.match(source, /storage\.putString\("call", config\.callsign\)/);
assert.match(source, /text\(config\.callsign, DISPLAY_W - callWidth - 8, 12, 2, kAccent\)/);
assert.match(source, /drawHomeBrand\(\)/);
assert.match(source, /void drawBootLogo\(\)/);
assert.match(source, /drawBootLogo\(\);[\s\S]*while \(millis\(\) - logoShownAt < 1900\)/);
assert.doesNotMatch(source, /text\("MORSE TUTOR /);
assert.doesNotMatch(source, /"AO5BH"/);
assert.match(config, /constexpr size_t kCallSize = 10;/);
console.log('PASS: startup branding and persisted configurable callsign');
