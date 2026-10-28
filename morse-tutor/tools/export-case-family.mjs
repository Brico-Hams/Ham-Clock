import { readFileSync, mkdirSync, writeFileSync, existsSync } from 'node:fs';
import { resolve, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { execFileSync } from 'node:child_process';
import assert from 'node:assert/strict';

const root = resolve(fileURLToPath(new URL('..', import.meta.url)));
const mechanical = join(root, 'mechanical');
const catalog = JSON.parse(readFileSync(join(mechanical, 'display-profiles.json'), 'utf8'));
const openscad = process.env.OPENSCAD || 'openscad';
const selected = process.argv.slice(2);
for (const id of selected) assert(catalog.profiles.some(p => p.id === id), `Unknown profile: ${id}`);
for (const profile of catalog.profiles.filter(p => !selected.length || selected.includes(p.id))) {
  const { id, pcb, window, stack } = profile;
  assert(/^[a-z0-9-]+$/.test(id));
  assert([...pcb, ...window, stack].every(n => Number.isFinite(n) && n > 0));
  const folder = join(mechanical, 'family', id);
  mkdirSync(folder, { recursive: true });
  const definitions = { family: true, pcb_w: pcb[0], pcb_h: pcb[1],
    screen_w: window[0], screen_h: window[1], screen_stack: stack };
  for (const product of ['clock', 'tutor']) for (const part of ['base', 'lid']) {
    const target = join(folder, `${product}-${part}.stl`);
    const args = ['-o', target];
    for (const [key, value] of Object.entries({ ...definitions, product, part }))
      args.push('-D', `${key}=${JSON.stringify(value)}`);
    args.push(join(mechanical, 'bricohams.scad'));
    execFileSync(openscad, args, { stdio: ['ignore', 'ignore', 'pipe'] });
    assert(existsSync(target));
  }
  writeFileSync(join(folder, 'profile.json'), JSON.stringify(profile, null, 2) + '\n');
  console.log(`Exported ${id}: matching clock/tutor bases and lids (prototype)`);
}
