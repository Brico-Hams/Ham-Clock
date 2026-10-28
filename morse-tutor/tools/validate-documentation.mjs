import { readFileSync, existsSync } from 'node:fs';
import { resolve, join, dirname, relative } from 'node:path';
import { fileURLToPath } from 'node:url';
import { inflateRawSync } from 'node:zlib';
import { createHash } from 'node:crypto';
import assert from 'node:assert/strict';

const root = resolve(fileURLToPath(new URL('..', import.meta.url)));
const site = join(root, '..', 'webflasher', 'morse-tutor');
const catalog = JSON.parse(readFileSync(join(root, 'mechanical', 'display-profiles.json'), 'utf8'));
const report = JSON.parse(readFileSync(join(site, 'downloads', 'family-validation.json'), 'utf8'));
assert.equal(report.length, catalog.profiles.length * 4);
assert(report.every(r => r.components === 1 && r.nonManifoldEdges === 0 && r.volumeMm3 > 0));
for (const name of ['manual.html', 'cajas.html']) {
  const file = join(site, 'docs', name), html = readFileSync(file, 'utf8');
  assert(html.includes('lang="es"'));
  for (const [, target] of html.matchAll(/(?:src|href)="([^"]+)"/g)) {
    if (/^https?:/.test(target)) continue;
    const [path, fragment] = target.split('#');
    const resolved = path ? resolve(dirname(file), path) : file;
    assert(!relative(join(root, '..', 'webflasher'), resolved).startsWith('..'), `Escaping site: ${target}`);
    assert(existsSync(resolved), `${name}: missing ${target}`);
    if (fragment && resolved.endsWith('.html'))
      assert(readFileSync(resolved, 'utf8').includes(`id="${fragment}"`), `Missing anchor ${target}`);
    if (resolved.endsWith('.png')) {
      const bytes = readFileSync(resolved);
      assert(bytes.length > 10000);
      assert.equal(bytes.subarray(0, 8).toString('hex'), '89504e470d0a1a0a', `PNG: ${target}`);
    }
    if (resolved.endsWith('.svg')) assert(readFileSync(resolved, 'utf8').includes('<svg '));
  }
}
const hash = data => createHash('sha256').update(data).digest('hex');
function zipEntries(file) {
  const bytes = readFileSync(file), entries = new Map();
  assert.equal(bytes.readUInt32LE(0), 0x04034b50, `${file}: ZIP`);
  let eocd = bytes.length - 22;
  while (eocd >= Math.max(0, bytes.length - 65557) && bytes.readUInt32LE(eocd) !== 0x06054b50) eocd--;
  assert(eocd >= 0, 'ZIP central directory');
  const count = bytes.readUInt16LE(eocd + 10);
  let cursor = bytes.readUInt32LE(eocd + 16);
  for (let i = 0; i < count; i++) {
    assert.equal(bytes.readUInt32LE(cursor), 0x02014b50);
    const method = bytes.readUInt16LE(cursor + 10), compressed = bytes.readUInt32LE(cursor + 20);
    const length = bytes.readUInt16LE(cursor + 28), extra = bytes.readUInt16LE(cursor + 30);
    const comment = bytes.readUInt16LE(cursor + 32), local = bytes.readUInt32LE(cursor + 42);
    const name = bytes.toString('utf8', cursor + 46, cursor + 46 + length).replaceAll('\\', '/');
    assert(!name.includes('..') && !name.startsWith('/'), `Unsafe archive entry ${name}`);
    const start = local + 30 + bytes.readUInt16LE(local + 26) + bytes.readUInt16LE(local + 28);
    const data = bytes.subarray(start, start + compressed);
    assert(method === 0 || method === 8, 'Supported ZIP compression');
    entries.set(name, method === 8 ? inflateRawSync(data) : data);
    cursor += 46 + length + extra + comment;
  }
  return entries;
}
const all = zipEntries(join(site, 'downloads', 'bricohams-display-family.zip'));
for (const p of catalog.profiles) {
  const zip = zipEntries(join(site, 'downloads', `${p.id}.zip`));
  assert.deepEqual(JSON.parse(zip.get('profile.json')), p);
  assert.equal(zip.size, 13, `${p.id}: four case parts, profile, four shared parts and four source/report files`);
  for (const product of ['clock', 'tutor']) for (const part of ['base', 'lid']) {
    const name = `${product}-${part}.stl`, source = readFileSync(join(root, 'mechanical', 'family', p.id, name));
    assert.equal(hash(zip.get(name)), hash(source), `${p.id}/${name}: published individual ZIP`);
    assert.equal(hash(all.get(`family/${p.id}/${name}`)), hash(source), `${p.id}/${name}: full ZIP`);
  }
}
console.log(`PASS: manual/catalog links, image signatures, ${report.length} mesh reports and ${catalog.profiles.length + 1} ZIPs with exact STL content`);
