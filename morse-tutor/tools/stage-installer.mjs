import { cpSync, mkdirSync } from 'node:fs';
import { resolve, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = resolve(fileURLToPath(new URL('..', import.meta.url)));
const target = join(root, '..', 'webflasher', 'morse-tutor');
mkdirSync(target, { recursive: true });
cpSync(join(root, 'webflasher'), target, { recursive: true });
console.log(`Staged independent tutor installer: ${target}`);
