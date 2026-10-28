import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { resolve, join } from 'node:path';
import { fileURLToPath } from 'node:url';
const root = resolve(fileURLToPath(new URL('..', import.meta.url)));
const catalog = JSON.parse(readFileSync(join(root, 'mechanical', 'display-profiles.json'), 'utf8'));
const assets = join(root, 'docs', 'assets');
mkdirSync(assets, { recursive: true });
const escape = s => String(s).replace(/[&<>"']/g, c =>
  ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&apos;' })[c]);
const rows = [];
for (const p of catalog.profiles) {
  const cw = Math.max(116, p.pcb[0] + 28), ch = Math.max(80, p.pcb[1] + 24);
  const tw = Math.max(174, p.pcb[0] + 28), th = ch + 56;
  const x = (tw - p.window[0]) / 2, y = 56 + (ch - p.window[1]) / 2;
  const svg = `<svg xmlns="http://www.w3.org/2000/svg" width="720" height="620" viewBox="-12 -14 ${tw + 24} ${th + 34}" role="img">
<title>${escape(p.name)}: plano esquematico del tutor, no render de STL ni montaje verificado</title>
<rect x="0" y="0" width="${tw}" height="${th}" rx="5" fill="#213047" stroke="#ff9933"/>
<rect x="0" y="56" width="${tw}" height="${ch}" rx="5" fill="#304866" stroke="#93e5ff"/>
<rect x="${(tw - p.pcb[0]) / 2}" y="${56 + (ch - p.pcb[1]) / 2}" width="${p.pcb[0]}" height="${p.pcb[1]}" fill="none" stroke="#cbd9ef" stroke-dasharray="2,2"/>
<rect x="${x}" y="${y}" width="${p.window[0]}" height="${p.window[1]}" fill="#101725" stroke="#ff9933"/>
<text x="${tw / 2}" y="66" text-anchor="middle" fill="#ffb15c" font-family="sans-serif" font-size="5">BRICOHAMS</text>
<text x="${tw / 2}" y="${y + p.window[1] / 2}" text-anchor="middle" fill="white" font-family="sans-serif" font-size="5">${p.window.join(' x ')} mm</text>
<g fill="#ffb15c" stroke="#101725"><rect x="53" y="3" width="14" height="43" rx="3"/><rect x="107" y="3" width="14" height="43" rx="3"/></g>
<text x="${tw / 2}" y="${th + 10}" text-anchor="middle" fill="#111" font-family="sans-serif" font-size="5">Tutor ${tw.toFixed(2)} x ${th.toFixed(2)} mm - envolvente provisional</text>
</svg>`;
  writeFileSync(join(assets, `case-${p.id}.svg`), svg + '\n');
  rows.push(`<article id="${p.id}"><h2>${escape(p.name)}</h2>
<img src="assets/case-${p.id}.svg" alt="Plano esquemático de caja ${escape(p.name)}">
<p><strong>Prototipo no verificado físicamente.</strong> Esquema de planta, no render del STL.</p>
<dl><dt>Envolvente PCB de diseño</dt><dd>${p.pcb.join(' × ')} mm</dd><dt>Ventana centrada</dt><dd>${p.window.join(' × ')} mm</dd>
<dt>Altura reservada del conjunto</dt><dd>${p.stack} mm (medir componentes)</dd>
<dt>Reloj, base exterior</dt><dd>${cw.toFixed(2)} × ${ch.toFixed(2)} × ${Math.max(48, p.stack + 36)} mm</dd>
<dt>Tutor, base exterior</dt><dd>${tw.toFixed(2)} × ${th.toFixed(2)} × ${Math.max(60, p.stack + 36)} mm</dd>
<dt>Perfil de firmware publicado</dt><dd>${p.firmware.length ? p.firmware.map(escape).join(', ') : '<strong>Ninguno. Caja mecánica solamente.</strong>'}</dd></dl>
<p>${escape(p.basis)}</p><p><a href="${escape(p.source)}">Fuente de referencia</a> ·
<a href="../downloads/${p.id}.zip" download>Descargar conjunto STL + fuente</a></p></article>`);
}
writeFileSync(join(root, 'docs', 'cajas.html'), `<!doctype html>
<html lang="es"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Bricohams · Cajas para nueve variantes de pantalla</title>
<style>body{font:17px/1.6 system-ui;max-width:1100px;margin:auto;padding:24px;color:#eef4ff;background:#101725}
h1,h2{color:#ffb15c}a{color:#93e5ff}.notice,article{background:#213047;padding:20px;border-radius:12px;margin:20px 0}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%,300px),1fr));gap:20px}img{width:100%;height:auto;background:#eee;border-radius:8px}
dt{font-weight:bold}dd{margin:0 0 10px;overflow-wrap:anywhere}code{overflow-wrap:anywhere}</style></head>
<body><h1>Cajas Bricohams: mismo estilo, nueve variantes</h1>
<p><a href="manual.html#caja">Montaje detallado</a> · <a href="../index.html">Instalador</a> ·
<a href="../downloads/bricohams-display-family.zip" download>Descargar toda la familia</a></p>
<div class="notice"><strong>36 piezas principales:</strong> nueve variantes × reloj/tutor × base/tapa.
Cada ZIP individual añade las dos palas, abrazadera y cupón de ajuste comunes, el SCAD y los parámetros.
Las tapas tienen wordmark BRICOHAMS grabado. El power bank previsto es USB protegido, cerrado, de hasta 100 × 60 × 22 mm.
<p>Las cotas indicadas son <strong>envolventes de diseño</strong>; las inferidas no se presentan como planos del fabricante.
No se ha probado ajuste ni impresión física. Los apoyos, altura, ventana centrada y pasos de cable deben comprobarse en la revisión real.
Las fuentes acreditan familias/controladores y, donde se indica, dimensiones de módulo; no un ranking de ventas.</p>
<p>Una caja <strong>no</strong> añade soporte de firmware ni habilita palas externas.
N/R/C no son equivalentes: N necesita navegación sin táctil; R usa XPT2046; C de esta familia usa GT911.
El firmware publicado sigue teniendo cuatro perfiles, no nueve.</p>
<p><a href="../downloads/family-validation.json">Informe de mallas</a> ·
<a href="../downloads/display-profiles.json">Parámetros y procedencia</a> ·
<a href="../downloads/bricohams.scad">Fuente OpenSCAD común</a></p></div>
<h2>Vistas CAD de referencia</h2>
<p>Renderizados reales del modelo OpenSCAD CYD 2,8″. No son fotografías ni demuestran ajuste físico.
La base lleva soportes internos que requieren soporte de impresión.</p>
<div class="grid"><figure><img src="assets/cad-clock.png" alt="Render CAD de la base reloj con batería"><figcaption>Base reloj.</figcaption></figure>
<figure><img src="assets/cad-tutor.png" alt="Render CAD de base tutor con plataforma de palas"><figcaption>Base tutor; las palas se imprimen por separado.</figcaption></figure></div>
<div class="grid">${rows.join('\n')}</div>
<p><a href="manual.html">Volver al manual de configuración</a></p></body></html>\n`);
console.log(`Generated enclosure catalog for ${catalog.profiles.length} profiles`);
