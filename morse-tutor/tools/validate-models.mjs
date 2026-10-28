import { readFileSync, readdirSync, writeFileSync } from 'node:fs';
import { join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import assert from 'node:assert/strict';

const root = resolve(fileURLToPath(new URL('..', import.meta.url)));
const family = process.argv.includes('--family');
const folder = join(root, 'mechanical', family ? 'family' : 'stl');
const profiles = JSON.parse(readFileSync(join(root, 'mechanical', 'display-profiles.json'), 'utf8')).profiles;
const names = family ? profiles.flatMap(p => ['clock', 'tutor'].flatMap(product =>
  ['base', 'lid'].map(part => join(p.id, `${product}-${part}.stl`)))) :
  readdirSync(folder).filter(n => n.endsWith('.stl'));
const results = [];
for (const name of names) {
  const source = readFileSync(join(folder, name), 'utf8');
  const vertices = [...source.matchAll(/vertex\s+([-\d.e+]+)\s+([-\d.e+]+)\s+([-\d.e+]+)/gi)]
    .map(m => m.slice(1).map(Number));
  assert(vertices.length > 0 && vertices.length % 3 === 0, `${name}: ASCII triangle mesh`);
  const key = v => v.map(n => n.toFixed(5)).join(',');
  const edges = new Map(), neighbors = new Map();
  let volume = 0;
  for (let i = 0; i < vertices.length; i += 3) {
    const [a, b, c] = vertices.slice(i, i + 3);
    volume += (a[0] * (b[1] * c[2] - b[2] * c[1]) -
      a[1] * (b[0] * c[2] - b[2] * c[0]) + a[2] * (b[0] * c[1] - b[1] * c[0])) / 6;
    for (const [v1, v2] of [[a, b], [b, c], [c, a]]) {
      const k1 = key(v1), k2 = key(v2), edge = [k1, k2].sort().join('|');
      edges.set(edge, (edges.get(edge) ?? 0) + 1);
      if (!neighbors.has(k1)) neighbors.set(k1, new Set());
      if (!neighbors.has(k2)) neighbors.set(k2, new Set());
      neighbors.get(k1).add(k2); neighbors.get(k2).add(k1);
    }
  }
  const badEdges = [...edges.values()].filter(n => n !== 2).length;
  assert.equal(badEdges, 0, `${name}: non-manifold mesh edges`);
  let components = 0;
  const visited = new Set(), componentBoxes = [];
  for (const first of neighbors.keys()) {
    if (visited.has(first)) continue;
    components++;
    const stack = [first], members = [];
    visited.add(first);
    while (stack.length) {
      const current = stack.pop(); members.push(current.split(',').map(Number));
      for (const next of neighbors.get(current)) if (!visited.has(next)) { visited.add(next); stack.push(next); }
    }
    componentBoxes.push([0, 1, 2].map(axis => [
      Math.min(...members.map(v => v[axis])), Math.max(...members.map(v => v[axis])),
    ]));
  }
  assert.equal(components, 1, `${name}: disconnected shells ${JSON.stringify(componentBoxes)}`);
  const bounds = [0, 1, 2].map(axis => [
    Math.min(...vertices.map(v => v[axis])), Math.max(...vertices.map(v => v[axis])),
  ]);
  assert(volume > 0, `${name}: positive enclosed volume`);
  if (family) {
    const profile = profiles.find(p => name.startsWith(p.id + '/') || name.startsWith(p.id + '\\'));
    const tutor = name.includes('tutor-'), base = name.endsWith('base.stl');
    const width = Math.max(tutor ? 174 : 116, profile.pcb[0] + 28, 116);
    const height = Math.max(profile.pcb[1] + 24, 80) + (tutor && base ? 56 : 0);
    const depth = base ? Math.max(tutor ? 60 : 48, profile.stack + 36) : 3.9;
    for (const [axis, expected] of [width, height, depth].entries())
      assert(Math.abs(bounds[axis][1] - bounds[axis][0] - expected) < 0.02,
        `${name}: unexpected envelope on axis ${axis}`);
  }
  results.push({ name, triangles: vertices.length / 3, components, nonManifoldEdges: badEdges,
    boundsMm: bounds, volumeMm3: Math.round(volume) });
}
assert.equal(results.length, family ? profiles.length * 4 : 8, 'Expected complete printable parts');
writeFileSync(join(root, 'mechanical', family ? 'family-validation.json' : 'mesh-validation.json'), JSON.stringify(results, null, 2) + '\n');
console.log(JSON.stringify(results, null, 2));
