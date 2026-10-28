import { readFileSync, existsSync } from 'node:fs';
import { join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { createHash } from 'node:crypto';
import assert from 'node:assert/strict';

const root = resolve(fileURLToPath(new URL('..', import.meta.url)));
const site = join(root, '..', 'webflasher', 'morse-tutor');
const environments = ['esp32-2432s028r', 'esp32-2432s028r-st7789', 'esp32-4in-st7796', 'freenove-32-st7789'];
const checksums = new Map(readFileSync(join(site, 'firmware', 'SHA256SUMS'), 'utf8').trim()
  .split('\n').map(line => { const [hash, name] = line.split('  '); return [name, hash]; }));
let checked = 0;
for (const environment of environments) {
  const manifest = JSON.parse(readFileSync(join(site, `manifest-${environment}.json`), 'utf8'));
  assert(manifest.name.includes('Bricohams Morse Tutor'));
  assert.equal(manifest.version, '2.1.0');
  assert.equal(manifest.builds.length, 1);
  assert.equal(manifest.builds[0].chipFamily, 'ESP32');
  const parts = manifest.builds[0].parts;
  assert.deepEqual(parts.map(p => p.offset), [0x1000, 0x8000, 0xe000, 0x10000]);
  assert.equal(parts.length, 4);
  for (let i = 0; i < parts.length; i++) {
    const part = parts[i], data = readFileSync(join(site, part.path));
    assert(data.length > 0);
    if (i < parts.length - 1) assert(part.offset + data.length <= parts[i + 1].offset);
    const relative = `${environment}/${part.path.split('/').at(-1)}`;
    assert.equal(createHash('sha256').update(data).digest('hex'), checksums.get(relative));
    if (part.path.endsWith('firmware.bin') || part.path.endsWith('bootloader.bin'))
      assert.equal(data[0], 0xe9, 'ESP32 image magic');
    if (part.path.endsWith('partitions.bin')) {
      assert.equal(data.readUInt16LE(0), 0x50aa, 'ESP32 partition magic');
      const apps = [];
      for (let offset = 0; offset + 32 <= data.length && data.readUInt16LE(offset) === 0x50aa; offset += 32) {
        if (data[offset + 2] === 0) apps.push({ offset: data.readUInt32LE(offset + 4), size: data.readUInt32LE(offset + 8) });
      }
      assert(apps.some(p => p.offset === 0x10000 && p.size >= readFileSync(join(site, parts[3].path)).length),
        'Firmware must fit real partition table');
    }
    checked++;
  }
}
assert.equal(checksums.size, environments.length * 4);
for (const name of ['Bricohams-MIT.txt', 'THIRD_PARTY_NOTICES.md', 'TFT_eSPI.txt', 'Arduino-ESP32-LGPL-2.1.md'])
  assert(readFileSync(join(site, 'licenses', name), 'utf8').length > 100, 'Distributed copyright notices');
const clockPage = readFileSync(join(root, '..', 'webflasher', 'index.html'), 'utf8');
const tutorPage = readFileSync(join(site, 'index.html'), 'utf8');
assert(clockPage.includes('href="morse-tutor/index.html"'));
assert(tutorPage.includes('href="../index.html"'));
for (const environment of environments) assert(tutorPage.includes(`value="${environment}"`));
assert(existsSync(join(root, 'hardware', 'adapter', 'gerber', 'Bricohams-PTH.drl')));
assert.equal(JSON.parse(readFileSync(join(root, 'mechanical', 'mesh-validation.json'), 'utf8')).length, 8);
console.log(`PASS: ${checked} firmware files, SHA-256, flash offsets, partition fit, ${environments.length} manifests, reciprocal installer links and 8 mesh reports.`);
