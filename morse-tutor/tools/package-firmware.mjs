import { mkdirSync, readFileSync, copyFileSync, writeFileSync } from 'node:fs';
import { resolve, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { createHash } from 'node:crypto';

const root = resolve(fileURLToPath(new URL('..', import.meta.url)));
const environments = ['esp32-2432s028r', 'esp32-2432s028r-st7789', 'esp32-4in-st7796', 'freenove-32-st7789'];
const checksums = [];
for (const environment of environments) {
  const build = join(root, '.pio', 'build', environment);
  const target = join(root, 'webflasher', 'firmware', environment);
  mkdirSync(target, { recursive: true });
  const parts = [
    { file: 'bootloader.bin', offset: 0x1000 },
    { file: 'partitions.bin', offset: 0x8000 },
    { file: 'boot_app0.bin', offset: 0xe000 },
    { file: 'firmware.bin', offset: 0x10000 },
  ];
  for (const part of parts) {
    copyFileSync(join(build, part.file), join(target, part.file));
    const hash = createHash('sha256').update(readFileSync(join(target, part.file))).digest('hex');
    checksums.push(`${hash}  ${environment}/${part.file}`);
  }
  writeFileSync(join(root, 'webflasher', `manifest-${environment}.json`), JSON.stringify({
    name: `Bricohams Morse Tutor (${environment})`,
    version: '2.1.0',
    new_install_prompt_erase: false,
    builds: [{ chipFamily: 'ESP32', parts: parts.map(part => ({
      path: `firmware/${environment}/${part.file}`, offset: part.offset,
    })) }],
  }, null, 2) + '\n');
}
writeFileSync(join(root, 'webflasher', 'firmware', 'SHA256SUMS'), checksums.join('\n') + '\n');
const notices = join(root, 'webflasher', 'licenses');
mkdirSync(notices, { recursive: true });
copyFileSync(join(root, 'LICENSE'), join(notices, 'Bricohams-MIT.txt'));
copyFileSync(join(root, 'THIRD_PARTY_NOTICES.md'), join(notices, 'THIRD_PARTY_NOTICES.md'));
copyFileSync(join(root, '.pio', 'libdeps', environments[0], 'TFT_eSPI', 'license.txt'),
  join(notices, 'TFT_eSPI.txt'));
const license = await fetch('https://raw.githubusercontent.com/espressif/arduino-esp32/2.0.17/LICENSE.md');
if (!license.ok) throw new Error(`Cannot package Arduino LGPL notice: HTTP ${license.status}`);
writeFileSync(join(notices, 'Arduino-ESP32-LGPL-2.1.md'), await license.text());
console.log(`Packaged ${environments.length} independent Morse Tutor builds.`);
