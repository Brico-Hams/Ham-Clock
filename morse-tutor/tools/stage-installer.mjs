import { cpSync, mkdirSync } from 'node:fs';
import { resolve, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = resolve(fileURLToPath(new URL('..', import.meta.url)));
const target = join(root, '..', 'webflasher', 'morse-tutor');
mkdirSync(target, { recursive: true });
cpSync(join(root, 'webflasher'), target, { recursive: true });
cpSync(join(root, 'docs'), join(target, 'docs'), { recursive: true });
cpSync(join(root, 'mechanical', 'downloads'), join(target, 'downloads'), { recursive: true });
for (const name of ['family-validation.json', 'display-profiles.json', 'bricohams.scad'])
  cpSync(join(root, 'mechanical', name), join(target, 'downloads', name));
console.log(`Staged independent tutor installer: ${target}`);
