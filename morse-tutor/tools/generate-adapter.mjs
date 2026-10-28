import { mkdirSync, writeFileSync } from 'node:fs';
import { resolve, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import assert from 'node:assert/strict';

// Original, wired THT adapter. This is not a pin-compatible CYD shield.
const root = resolve(fileURLToPath(new URL('..', import.meta.url)));
const out = join(root, 'hardware', 'adapter');
const gerbers = join(out, 'gerber');
mkdirSync(gerbers, { recursive: true });
const width = 80, height = 40, traceWidth = 0.6, clearance = 0.25, step = 0.5;
const components = [];
const pads = [];
function component(ref, value, points) {
  const entries = points.map(([x, y, net], i) => ({ ref, pin: i + 1, x, y, net, diameter: 1.8, drill: 0.8 }));
  components.push({ ref, value, pads: entries });
  pads.push(...entries);
}
component('J1', 'CYD wire pads, 5 mm pitch', [
  [6, 7, 'GND'], [6, 12, '3V3'], [6, 17, 'GPIO_DIT'],
  [6, 22, 'GPIO_DAH'], [6, 27, 'AUDIO_PWM'], [6, 32, '5V'],
]);
component('J2', 'External KEY: sleeve, tip, ring', [[74, 7, 'GND'], [74, 17, 'KEY_DIT'], [74, 22, 'KEY_DAH']]);
component('J3', 'Integrated paddle contacts: common, dit, dah', [[65, 7, 'GND'], [65, 17, 'KEY_DIT'], [65, 22, 'KEY_DAH']]);
component('R1', '1k, axial 10 mm', [[20, 17, 'GPIO_DIT'], [30, 17, 'KEY_DIT']]);
component('R2', '1k, axial 10 mm', [[20, 22, 'GPIO_DAH'], [30, 22, 'KEY_DAH']]);
component('R3', '10k, axial 10 mm', [[40, 7, '3V3'], [40, 17, 'KEY_DIT']]);
component('R4', '10k, axial 10 mm', [[50, 12, '3V3'], [50, 22, 'KEY_DAH']]);
component('C1', '10nF, 5 mm', [[34, 12, 'KEY_DIT'], [34, 7, 'GND']]);
component('C2', '10nF, 5 mm', [[56, 22, 'KEY_DAH'], [56, 27, 'GND']]);
component('R5', '10k, axial 10 mm', [[20, 27, 'AUDIO_PWM'], [30, 27, 'ATTENUATED']]);
component('R6', '1k, vertical axial 5 mm', [[40, 27, 'ATTENUATED'], [40, 32, 'GND']]);
component('C3', '1uF nonpolar film, 5 mm', [[46, 27, 'ATTENUATED'], [51, 27, 'LINE_OUT']]);
component('J4', 'Active speaker: signal, ground', [[74, 27, 'LINE_OUT'], [74, 32, 'GND']]);
component('J5', 'External amp: 5V, GND, IN', [[60, 32, '5V'], [65, 32, 'GND'], [70, 32, 'LINE_OUT']]);
const mounts = [[3, 3], [77, 3], [3, 37], [77, 37]];
const nets = [...new Set(pads.map(p => p.net))];
const segments = [];
const vias = [];
const copper = pads.map(p => ({ ...p, kind: 'pad', layer: -1 }));
const distance = (a, b) => Math.hypot(a.x - b.x, a.y - b.y);
function pointSegment(p, a, b) {
  const dx = b.x - a.x, dy = b.y - a.y;
  const length = dx * dx + dy * dy;
  const t = length ? Math.max(0, Math.min(1, ((p.x - a.x) * dx + (p.y - a.y) * dy) / length)) : 0;
  return Math.hypot(p.x - a.x - t * dx, p.y - a.y - t * dy);
}
function segmentDistance(a, b, c, d) {
  const cross = (p, q, r) => (q.x - p.x) * (r.y - p.y) - (q.y - p.y) * (r.x - p.x);
  const abC = cross(a, b, c), abD = cross(a, b, d), cdA = cross(c, d, a), cdB = cross(c, d, b);
  if (abC * abD < 0 && cdA * cdB < 0) return 0;
  return Math.min(pointSegment(a, c, d), pointSegment(b, c, d), pointSegment(c, a, b), pointSegment(d, a, b));
}
function blocked(point, layer, net, radius = traceWidth / 2) {
  if (point.x < 1 || point.x > width - 1 || point.y < 1 || point.y > height - 1) return true;
  if (mounts.some(([x, y]) => Math.hypot(point.x - x, point.y - y) < 1.6 + radius + clearance)) return true;
  return copper.some(item => item.net !== net && (item.layer === -1 || item.layer === layer) &&
    (item.kind === 'segment'
      ? pointSegment(point, item.a, item.b) < radius + item.width / 2 + clearance + 0.04
      : distance(point, item) < radius + item.diameter / 2 + clearance + 0.04));
}
class Heap {
  items = [];
  push(item) {
    let i = this.items.length;
    this.items.push(item);
    while (i > 0) {
      const parent = (i - 1) >> 1;
      if (this.items[parent].f <= item.f) break;
      this.items[i] = this.items[parent]; i = parent;
    }
    this.items[i] = item;
  }
  pop() {
    const first = this.items[0], last = this.items.pop();
    if (this.items.length) {
      let i = 0;
      while (i * 2 + 1 < this.items.length) {
        let child = i * 2 + 1;
        if (child + 1 < this.items.length && this.items[child + 1].f < this.items[child].f) child++;
        if (this.items[child].f >= last.f) break;
        this.items[i] = this.items[child]; i = child;
      }
      this.items[i] = last;
    }
    return first;
  }
}
const nx = width / step + 1, ny = height / step + 1, plane = nx * ny;
const encode = (x, y, z) => z * plane + y * nx + x;
const decode = id => ({ x: (id % nx) * step, y: Math.floor((id % plane) / nx) * step, layer: Math.floor(id / plane) });
function route(a, b, net) {
  const heap = new Heap(), costs = new Map(), previous = new Map();
  const forbidden = [new Uint8Array(plane), new Uint8Array(plane)];
  for (let layer = 0; layer < 2; layer++)
    for (let y = 0; y < ny; y++)
      for (let x = 0; x < nx; x++)
        forbidden[layer][y * nx + x] = blocked({ x: x * step, y: y * step }, layer, net);
  for (const layer of [0, 1]) {
    const id = encode(a.x / step, a.y / step, layer);
    costs.set(id, 0); heap.push({ id, f: distance(a, b) });
  }
  let end;
  while (heap.items.length) {
    const { id } = heap.pop();
    const p = decode(id), cost = costs.get(id);
    if (p.x === b.x && p.y === b.y) { end = id; break; }
    for (const [dx, dy, dz] of [[1, 0, 0], [-1, 0, 0], [0, 1, 0], [0, -1, 0], [0, 0, 1]]) {
      const x = p.x / step + dx, y = p.y / step + dy, layer = dz ? 1 - p.layer : p.layer;
      if (x < 0 || x >= nx || y < 0 || y >= ny || forbidden[layer][y * nx + x]) continue;
      if (dz && (blocked(p, 0, net, 0.5) || blocked(p, 1, net, 0.5))) continue;
      const next = encode(x, y, layer), newCost = cost + (dz ? 5 : step);
      if (newCost >= (costs.get(next) ?? Infinity)) continue;
      costs.set(next, newCost); previous.set(next, id);
      heap.push({ id: next, f: newCost + distance({ x: x * step, y: y * step }, b) });
    }
  }
  assert.notEqual(end, undefined, `Unroutable net ${net}`);
  const path = [];
  for (let id = end; id !== undefined; id = previous.get(id)) path.push(decode(id));
  path.reverse();
  for (let i = 1; i < path.length; i++) {
    const a = path[i - 1], b = path[i];
    if (a.layer !== b.layer) {
      if (!vias.some(v => v.net === net && distance(v, a) === 0)) {
        const via = { x: a.x, y: a.y, net, diameter: 1, drill: 0.4, layer: -1, kind: 'via' };
        vias.push(via); copper.push(via);
      }
    } else {
      const last = segments.at(-1);
      if (last && last.net === net && last.layer === a.layer && distance(last.b, a) === 0 &&
          (last.b.x - last.a.x) * (b.y - a.y) === (last.b.y - last.a.y) * (b.x - a.x)) {
        last.b = { x: b.x, y: b.y };
      } else {
        const segment = { a: { x: a.x, y: a.y }, b: { x: b.x, y: b.y }, width: traceWidth, layer: a.layer, net, kind: 'segment' };
        segments.push(segment); copper.push(segment);
      }
    }
  }
}
for (const net of [...nets].sort((a, b) => pads.filter(p => p.net === a).length - pads.filter(p => p.net === b).length)) {
  const remaining = pads.filter(p => p.net === net), connected = [remaining.shift()];
  while (remaining.length) {
    let best;
    for (const a of connected) for (let i = 0; i < remaining.length; i++) {
      const d = distance(a, remaining[i]);
      if (!best || d < best.d) best = { a, i, d };
    }
    const b = remaining.splice(best.i, 1)[0];
    route(best.a, b, net); connected.push(b);
  }
}

// Independent geometric clearance and graph checks, performed before any export.
let minClearance = Infinity;
for (let i = 0; i < copper.length; i++) for (let j = i + 1; j < copper.length; j++) {
  const a = copper[i], b = copper[j];
  if (a.net === b.net || (a.layer !== -1 && b.layer !== -1 && a.layer !== b.layer)) continue;
  const ra = a.kind === 'segment' ? a.width / 2 : a.diameter / 2;
  const rb = b.kind === 'segment' ? b.width / 2 : b.diameter / 2;
  const d = a.kind === 'segment' && b.kind === 'segment' ? segmentDistance(a.a, a.b, b.a, b.b)
    : a.kind === 'segment' ? pointSegment(b, a.a, a.b)
    : b.kind === 'segment' ? pointSegment(a, b.a, b.b) : distance(a, b);
  minClearance = Math.min(minClearance, d - ra - rb);
  assert(d - ra - rb >= clearance - 0.001, `Copper clearance ${a.net} / ${b.net}: ${d - ra - rb}`);
}
for (const net of nets) {
  const items = copper.filter(p => p.net === net);
  const reached = new Set([0]);
  let changed = true;
  while (changed) {
    changed = false;
    for (let i = 0; i < items.length; i++) {
      if (reached.has(i)) continue;
      const a = items[i];
      for (const j of reached) {
        const b = items[j];
        if (a.layer !== -1 && b.layer !== -1 && a.layer !== b.layer) continue;
        const d = a.kind === 'segment' && b.kind === 'segment' ? segmentDistance(a.a, a.b, b.a, b.b)
          : a.kind === 'segment' ? pointSegment(b, a.a, a.b)
          : b.kind === 'segment' ? pointSegment(a, b.a, b.b) : distance(a, b);
        if (d < 0.001) { reached.add(i); changed = true; break; }
      }
    }
  }
  assert.equal(reached.size, items.length, `Disconnected copper on ${net}`);
}

const coord = n => String(Math.round(n * 1e6)).padStart(10, '0');
const xy = (x, y) => `X${coord(x)}Y${coord(height - y)}`;
const header = '%FSLAX46Y46*%\n%MOMM*%\n%LPD*%\nG01*\n';
function copperGerber(layer) {
  let text = header + `%ADD10C,${traceWidth}*%\n%ADD11C,1.8*%\n%ADD12C,1.0*%\nD10*\n`;
  for (const s of segments.filter(s => s.layer === layer))
    text += `${xy(s.a.x, s.a.y)}D02*\n${xy(s.b.x, s.b.y)}D01*\n`;
  text += 'D11*\n' + pads.map(p => `${xy(p.x, p.y)}D03*\n`).join('');
  text += 'D12*\n' + vias.map(p => `${xy(p.x, p.y)}D03*\n`).join('');
  return text + 'M02*\n';
}
writeFileSync(join(gerbers, 'Bricohams-F_Cu.gbr'), copperGerber(0));
writeFileSync(join(gerbers, 'Bricohams-B_Cu.gbr'), copperGerber(1));
const mask = header + '%ADD10C,2.0*%\n%ADD11C,1.2*%\nD10*\n' +
  pads.map(p => `${xy(p.x, p.y)}D03*\n`).join('') + 'D11*\n' +
  vias.map(p => `${xy(p.x, p.y)}D03*\n`).join('') + 'M02*\n';
writeFileSync(join(gerbers, 'Bricohams-F_Mask.gbr'), mask);
writeFileSync(join(gerbers, 'Bricohams-B_Mask.gbr'), mask);
const outline = header + '%ADD10C,0.1*%\nD10*\n' +
  `${xy(0, 0)}D02*\n${xy(width, 0)}D01*\n${xy(width, height)}D01*\n${xy(0, height)}D01*\n${xy(0, 0)}D01*\nM02*\n`;
writeFileSync(join(gerbers, 'Bricohams-Edge_Cuts.gbr'), outline);
const drillCoord = (x, y) => `X${x.toFixed(3)}Y${(height - y).toFixed(3)}\n`;
writeFileSync(join(gerbers, 'Bricohams-PTH.drl'), 'M48\nMETRIC,TZ\nT01C0.800\nT02C0.400\n%\nG90\nG05\nT01\n' +
  pads.map(p => drillCoord(p.x, p.y)).join('') + 'T02\n' + vias.map(p => drillCoord(p.x, p.y)).join('') + 'M30\n');
writeFileSync(join(gerbers, 'Bricohams-NPTH.drl'), 'M48\nMETRIC,TZ\nT01C3.200\n%\nG90\nG05\nT01\n' +
  mounts.map(([x, y]) => drillCoord(x, y)).join('') + 'M30\n');

const font = {
  A: ['01110', '10001', '10001', '11111', '10001', '10001', '10001'],
  B: ['11110', '10001', '10001', '11110', '10001', '10001', '11110'],
  C: ['01111', '10000', '10000', '10000', '10000', '10000', '01111'],
  D: ['11110', '10001', '10001', '10001', '10001', '10001', '11110'],
  H: ['10001', '10001', '10001', '11111', '10001', '10001', '10001'],
  I: ['11111', '00100', '00100', '00100', '00100', '00100', '11111'],
  M: ['10001', '11011', '10101', '10101', '10001', '10001', '10001'],
  O: ['01110', '10001', '10001', '10001', '10001', '10001', '01110'],
  R: ['11110', '10001', '10001', '11110', '10100', '10010', '10001'],
  S: ['01111', '10000', '10000', '01110', '00001', '00001', '11110'],
  J: ['00111', '00010', '00010', '00010', '10010', '10010', '01100'],
  '1': ['00100', '01100', '00100', '00100', '00100', '00100', '01110'],
  '2': ['01110', '10001', '00001', '00010', '00100', '01000', '11111'],
  '3': ['11110', '00001', '00001', '01110', '00001', '00001', '11110'],
  '4': ['00010', '00110', '01010', '10010', '11111', '00010', '00010'],
  '5': ['11111', '10000', '10000', '11110', '00001', '00001', '11110'],
  '6': ['01110', '10000', '10000', '11110', '10001', '10001', '01110'],
};
let silk = header + '%ADD10C,0.22*%\nD10*\n';
for (let c = 0; c < 'BRICOHAMS'.length; c++) {
  const bitmap = font['BRICOHAMS'[c]];
  bitmap.forEach((row, y) => [...row].forEach((value, x) => {
    if (value === '1') silk += `${xy(25 + c * 1.8 + x * 0.3, 2 + y * 0.3)}D03*\n`;
  }));
}
// One identification tick per connector pad; detailed labels in assembly drawing.
for (const p of pads.filter(p => p.ref.startsWith('J'))) {
  silk += `${xy(p.x + 1.4, p.y)}D02*\n${xy(p.x + 2.1, p.y)}D01*\n`;
}
for (const component of components) {
  for (let c = 0; c < component.ref.length; c++) {
    font[component.ref[c]].forEach((row, y) => [...row].forEach((value, x) => {
      if (value === '1') silk += `${xy(component.pads[0].x + 1 + c * 1.2 + x * 0.2,
        component.pads[0].y - 2.8 + y * 0.2)}D03*\n`;
    }));
  }
}
writeFileSync(join(gerbers, 'Bricohams-F_Silkscreen.gbr'), silk + 'M02*\n');

const escape = s => s.replaceAll('&', '&amp;').replaceAll('<', '&lt;');
let svg = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="-2 -2 84 44"><rect width="80" height="40" fill="#163a2c"/><g fill="none" stroke-width="${traceWidth}">`;
for (const s of segments) svg += `<path stroke="${s.layer ? '#6aaaff' : '#f09649'}" d="M${s.a.x},${s.a.y}L${s.b.x},${s.b.y}"/>`;
svg += '</g><g font-family="sans-serif" font-size="1.5" fill="white">';
for (const c of components) {
  svg += `<text x="${c.pads[0].x + 1}" y="${c.pads[0].y - 1.5}">${c.ref}</text>`;
  for (const p of c.pads) svg += `<circle cx="${p.x}" cy="${p.y}" r=".9" fill="#dbb758"/><circle cx="${p.x}" cy="${p.y}" r=".4" fill="black"/>`;
}
for (const [x, y] of mounts) svg += `<circle cx="${x}" cy="${y}" r="1.6" fill="white"/>`;
svg += '<text x="25" y="4">BRICOHAMS - PROTOTYPE R1</text></g></svg>';
writeFileSync(join(out, 'assembly.svg'), svg);
let schematic = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1100 900"><rect width="1100" height="900" fill="white"/><g font-family="monospace" fill="black"><text x="30" y="30" font-size="24">Bricohams wired adapter R1 - connectivity schematic</text>';
components.forEach((c, i) => {
  const x = 30 + (i % 2) * 540, y = 65 + Math.floor(i / 2) * 115;
  schematic += `<rect x="${x}" y="${y}" width="510" height="100" fill="#eef5ff" stroke="black"/><text x="${x + 10}" y="${y + 22}" font-size="14">${escape(c.ref + ' ' + c.value)}</text>`;
  c.pads.forEach((p, index) => {
    schematic += `<text x="${x + 10}" y="${y + 42 + index * 9}" font-size="10">pin ${p.pin}: ${escape(p.net)} (${p.x}, ${p.y} mm)</text>`;
  });
});
schematic += '<text x="30" y="885" font-size="14">Same net labels are electrically connected. No RF keying output. No battery charging circuit.</text></g></svg>';
writeFileSync(join(out, 'schematic.svg'), schematic);
writeFileSync(join(out, 'BOM.csv'), 'Reference,Value,Mounting\n' +
  components.map(c => `"${c.ref}","${c.value}","THT/wired"`).join('\n') + '\n');
const netId = net => nets.indexOf(net) + 1;
let pcb = `(kicad_pcb (version 20221018) (generator bricohams)\n(general (thickness 1.6))\n(paper "A4")\n(layers (0 "F.Cu" signal) (31 "B.Cu" signal) (36 "B.SilkS" user "b.silkscreen") (37 "F.SilkS" user "f.silkscreen") (38 "B.Mask" user) (39 "F.Mask" user) (44 "Edge.Cuts" user) (48 "B.Fab" user) (49 "F.Fab" user))\n(setup (pad_to_mask_clearance 0.1))\n(net 0 "")\n`;
for (const net of nets) pcb += `(net ${netId(net)} "${net}")\n`;
for (const c of components) {
  pcb += `(footprint "Bricohams:${c.ref}" (layer "F.Cu") (at 0 0) (attr through_hole)\n(fp_text reference "${c.ref}" (at ${c.pads[0].x + 1} ${c.pads[0].y - 2}) (layer "F.SilkS") (effects (font (size 1 1) (thickness 0.15))))\n(fp_text value "${c.value}" (at ${c.pads[0].x} ${c.pads[0].y}) (layer "F.Fab") hide (effects (font (size 1 1) (thickness 0.15))))\n`;
  for (const p of c.pads) pcb += `(pad "${p.pin}" thru_hole circle (at ${p.x} ${p.y}) (size 1.8 1.8) (drill 0.8) (layers "*.Cu" "*.Mask") (net ${netId(p.net)} "${p.net}"))\n`;
  pcb += ')\n';
}
for (const [x, y] of mounts) pcb += `(footprint "Bricohams:mount" (layer "F.Cu") (at ${x} ${y}) (attr exclude_from_pos_files exclude_from_bom) (pad "" np_thru_hole circle (at 0 0) (size 3.2 3.2) (drill 3.2) (layers "*.Cu" "*.Mask")))\n`;
for (const s of segments) pcb += `(segment (start ${s.a.x} ${s.a.y}) (end ${s.b.x} ${s.b.y}) (width ${s.width}) (layer "${s.layer ? 'B.Cu' : 'F.Cu'}") (net ${netId(s.net)}))\n`;
for (const v of vias) pcb += `(via (at ${v.x} ${v.y}) (size 1) (drill 0.4) (layers "F.Cu" "B.Cu") (net ${netId(v.net)}))\n`;
pcb += '(gr_rect (start 0 0) (end 80 40) (stroke (width 0.1) (type default)) (fill none) (layer "Edge.Cuts"))\n(gr_text "BRICOHAMS R1" (at 40 3) (layer "F.SilkS") (effects (font (size 1.2 1.2) (thickness 0.2))))\n)\n';
writeFileSync(join(out, 'bricohams-adapter.kicad_pcb'), pcb);
writeFileSync(join(out, 'design.json'), JSON.stringify({ width, height, traceWidth, clearance, components, segments, vias, mounts }, null, 2) + '\n');
const report = { status: 'PROTOTYPE_NOT_PHYSICALLY_VALIDATED', nets: nets.length, pads: pads.length,
  segments: segments.length, vias: vias.length, minimumCopperClearanceMm: Number(minClearance.toFixed(3)),
  checks: ['all labelled nets connected', 'copper clearance >= 0.25 mm',
    'mount keep-outs during routing', '0.6 mm tracks', '0.8 mm component drills', '0.4 mm via drills'],
  notChecked: ['KiCad ERC/DRC', 'fabricator CAM', 'real board header pinout', 'ESD immunity', 'audio measurements', 'physical assembly'] };
writeFileSync(join(out, 'validation.json'), JSON.stringify(report, null, 2) + '\n');
console.log(JSON.stringify(report, null, 2));
