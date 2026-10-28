import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import assert from 'node:assert/strict';

// Documentation illustrations, not framebuffer captures or a hardware simulator.
const root = resolve(fileURLToPath(new URL('..', import.meta.url)));
const source = readFileSync(join(root, 'src', 'main.cpp'), 'utf8');
const catalog = readFileSync(join(root, 'include', 'Languages.h'), 'utf8');
const labels = Object.fromEntries([...catalog.matchAll(/^X\((\w+),(.*)\) ?\\?$/gm)]
  .map(([, key, values]) => [key, JSON.parse(`[${values}]`)[0]]));
const out = join(root, 'docs', 'assets');
mkdirSync(out, { recursive: true });
const escape = s => String(s).replace(/[&<>"']/g, c =>
  ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&apos;' })[c]);
let shapes = [], width = 320, height = 240;
const rect = (x, y, w, h, fill, stroke = fill, radius = 0) =>
  shapes.push(`<rect x="${x}" y="${y}" width="${w}" height="${h}" rx="${radius}" fill="${fill}" stroke="${stroke}"/>`);
function text(value, x, y, size = 2, color = '#ffffff') {
  shapes.push(`<text x="${x}" y="${y + size * 7}" fill="${color}" font-family="monospace" font-size="${size * 8}" textLength="${[...value].length * size * 6}" lengthAdjust="spacingAndGlyphs">${escape(value)}</text>`);
}
function button(x, y, w, h, label, selected = false) {
  rect(x, y, w, h, selected ? '#ff6519' : '#212429', '#ff6519', 6);
  const size = [...label].length * 12 > w - 8 ? 1 : 2;
  text(label, x + Math.max(2, (w - [...label].length * size * 6) / 2), y + (h - size * 8) / 2, size);
}
function footer(values) { values.forEach((v, i) => button(i * width / 5 + 2, height - 36, width / 5 - 4, 33, v)); }
function keyFooter(values) {
  footer([...values, '', '']);
  button(3 * width / 5 + 2, height - 36, 2 * width / 5 - 4, 33, labels.TouchKey);
}
function header(title) {
  button(6, 5, 76, 30, labels.Back);
  const size = [...title].length * 12 > width - 94 ? 1 : 2;
  text(title, 87 + Math.max(0, (width - 94 - [...title].length * 6 * size) / 2), 13, size, '#ff6519');
  rect(6, 41, width - 12, 1, '#ff6519');
}
function frame(name, title, draw, large = false) {
  width = large ? 480 : 320; height = large ? 320 : 240; shapes = [];
  rect(0, 0, width, height, '#101819'); draw();
  writeFileSync(join(out, `tft-${name}.svg`),
    `<svg xmlns="http://www.w3.org/2000/svg" width="${width * 2}" height="${height * 2}" viewBox="0 0 ${width} ${height}" role="img"><title>${escape(title)} - ilustracion de interfaz, no captura fisica</title>${shapes.join('')}</svg>\n`);
}
frame('inicio', 'Inicio', () => {
  text('brico', 10, 12); text('hams', 70, 12, 2, '#ff6519');
  text('MORSE TUTOR 2.1.1', width - 104, 18, 1, '#adabaa'); rect(8, 39, width - 16, 1, '#ff6519');
  ['Receive', 'Send', 'Config'].forEach((key, i) => {
    const w = (width - 24) / 3, x = 8 + i * (width - 8) / 3;
    button(x, 57, w, height - 106, '', i === 0);
    text(['RX', 'TX', '*'][i], x + 24, 75, 3, '#ffb060');
    const size = labels[key].length * 12 > w ? 1 : 2;
    text(labels[key], x + Math.max(0, (w - labels[key].length * 6 * size) / 2), 133, size);
  });
  text('Red CW: 0 Pendientes', 8, height - 51, 1); footer(['-', '+', 'OK', '', '']);
});
for (const [array, title, stem] of [['kReceive', 'Receive', 'recibir'], ['kSend', 'Send', 'enviar'], ['kSettings', 'Config', 'ajustes']]) {
  const block = source.match(new RegExp(`constexpr (?:MenuEntry|Text) ${array}\\[\\] = ([\\s\\S]*?);`));
  assert(block, `Menu source ${array}`);
  const keys = [...block[1].matchAll(/Text::(\w+)/g)].map(m => m[1]);
  for (let page = 0; page < Math.ceil(keys.length / 4); page++)
    frame(`${stem}-${page + 1}`, `${labels[title]}, pagina ${page + 1}`, () => {
      header(labels[title]);
      keys.slice(page * 4, page * 4 + 4).forEach((key, i) =>
        button(8 + i % 2 * width / 2, 52 + Math.floor(i / 2) * 55, width / 2 - 16, 47, labels[key], i === 0));
      text(`${page + 1}/${Math.ceil(keys.length / 4)}`, width / 2 - 9, height - 56, 1);
      footer(['<', '>', 'OK', '', '']);
    });
}
frame('idioma', 'Seleccion de idioma', () => {
  header(labels.UiLanguage); text('Español', 118, 100, 2, '#ff6519');
  footer(['-', '+', labels.Save, 'DIT', '']);
});
frame('red', 'Red CW, ejemplo', () => {
  header(labels.Network); text('Texto + audio | RX 1', 8, 49, 1, '#ff6519');
  text('Bricohams-CW-EJEMPLO', 8, 63, 1);
  text('Clave: BricohamsCW | 192.168.4.1', 8, 76, 1);
  text('RX NOCALL: CQ CQ 73', 8, 94); keyFooter(['CW', 'TXT', labels.Send]);
});
frame('eco', 'Entrenador eco, ejemplo', () => {
  header(labels.Echo); text('18/12 WPM  700 Hz  0/0', 8, 49, 1);
  text(labels.AnswerPrompt, 8, 76, 1, '#ff6519'); keyFooter(['-', '+', 'OK']);
});
frame('memorias', 'Memorias, ejemplo', () => {
  header(labels.Memories);
  ['1: CQ CQ DE NOCALL K', '2: HOLA RED 73', '3: KM KM', '4:'].forEach((line, i) =>
    button(8, 49 + i * 36, width - 16, 32, line, i === 0));
  footer(['<', '>', labels.Play, labels.Edit, '']);
});
frame('estadisticas', 'Estadisticas, ejemplo', () => {
  header(labels.Statistics); text('Intentos: 2  Aciertos: 50%', 8, 50, 1, '#ff6519');
  text('K  Intentos: 2  Errores: 0', 8, 75, 1);
  text('M  Intentos: 2  Errores: 1', 8, 101, 1); footer(['<', '>', '', '', '']);
});
frame('grande', 'Ejemplo 480 x 320', () => {
  header(labels.Network); text('Texto + audio | RX 1', 8, 49, 1, '#ff6519');
  text('Bricohams-CW-EJEMPLO', 8, 63, 1);
  text('Clave: BricohamsCW | 192.168.4.1', 8, 76, 1);
  text('RX NOCALL: CQ CQ 73', 8, 94); keyFooter(['CW', 'TXT', labels.Send]);
}, true);
console.log('Generated source-labeled TFT illustrations for the manual');
