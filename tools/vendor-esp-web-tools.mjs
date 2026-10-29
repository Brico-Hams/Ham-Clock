/**
 * Stage an integrity-pinned, same-origin ESP Web Tools distribution for Pages.
 * This downloads the npm tarball at BUILD TIME, never in the user's browser.
 * Run from the repository root: node tools/vendor-esp-web-tools.mjs
 */
import { createHash } from 'node:crypto';
import { execFileSync } from 'node:child_process';
import { createWriteStream, cpSync, existsSync, mkdirSync, mkdtempSync, readFileSync, readdirSync, rmSync, copyFileSync, writeFileSync } from 'node:fs';
import { get } from 'node:https';
import { tmpdir } from 'node:os';
import { dirname, join, relative, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { pipeline } from 'node:stream/promises';
import assert from 'node:assert/strict';

const root = resolve(fileURLToPath(new URL('..', import.meta.url)));
const lock = JSON.parse(readFileSync(join(root, 'tools/esp-web-tools.lock.json'), 'utf8'));
assert.equal(lock.name, 'esp-web-tools');
assert.match(lock.version, /^\d+\.\d+\.\d+$/);
assert.equal(lock.tarball, `https://registry.npmjs.org/esp-web-tools/-/esp-web-tools-${lock.version}.tgz`);
assert.match(lock.integrity, /^sha512-[A-Za-z0-9+/]+={0,2}$/);
const assetPath = `vendor/esp-web-tools/${lock.version}/install-button.js`;
const target = join(root, 'webflasher', 'vendor', 'esp-web-tools', lock.version);

function checkInstaller(file, expected) {
  const html = readFileSync(join(root, file), 'utf8');
  assert(html.includes(`src="${expected}"`), `${file}: expected locally hosted ESP Web Tools`);
  assert(!/<script\b[^>]*\bsrc\s*=\s*["'](?:https?:)?\/\//i.test(html), `${file}: external JavaScript source`);
  assert(!/unpkg\.com|cdn\.jsdelivr\.net/i.test(html), `${file}: CDN URL remains`);
}
checkInstaller('webflasher/index.html', assetPath);
checkInstaller('morse-tutor/webflasher/index.html', `../${assetPath}`);

function listFiles(dir) {
  return readdirSync(dir, { withFileTypes: true }).flatMap(entry => {
    const path = join(dir, entry.name);
    return entry.isDirectory() ? listFiles(path) : entry.isFile() ? [path] : [];
  }).sort();
}
function auditInstalledFiles() {
  const files = listFiles(target);
  assert(files.some(p => p.endsWith('/install-button.js')), 'install-button.js missing');
  assert(files.some(p => p.endsWith('.js') && !p.endsWith('/install-button.js')), 'web bundle chunks missing');
  for (const file of files.filter(p => p.endsWith('.js'))) {
    const js = readFileSync(file, 'utf8');
    assert(!/\b(?:import|from)\s*(?:\(\s*)?["']https?:\/\//i.test(js), `Remote module import in ${file}`);
    assert(!/\bimportScripts\s*\(\s*["']https?:\/\//i.test(js), `Remote worker import in ${file}`);
  }
  const sums = files.filter(p => !p.endsWith('/SHA256SUMS')).map(p => {
    const hash = createHash('sha256').update(readFileSync(p)).digest('hex');
    return `${hash}  ${relative(target, p).split('\\').join('/')}`;
  });
  writeFileSync(join(target, 'SHA256SUMS'), sums.join('\n') + '\n');
  console.log(`PASS: ESP Web Tools ${lock.version}, ${sums.length} vendored files and SHA-256 checksums`);
}

const tmp = mkdtempSync(join(tmpdir(), 'bricohams-ewt-'));
try {
  const archive = join(tmp, 'package.tgz');
  await new Promise((resolveDownload, rejectDownload) => {
    get(lock.tarball, async response => {
      if (response.statusCode !== 200) {
        response.resume();
        rejectDownload(new Error(`npm tarball HTTP ${response.statusCode}`));
        return;
      }
      try { await pipeline(response, createWriteStream(archive)); resolveDownload(); }
      catch (error) { rejectDownload(error); }
    }).on('error', rejectDownload);
  });
  const actual = 'sha512-' + createHash('sha512').update(readFileSync(archive)).digest('base64');
  assert.equal(actual, lock.integrity, 'npm archive SHA-512 mismatch; refusing to package unverified JavaScript');
  execFileSync('tar', ['-xzf', archive, '-C', tmp], { stdio: 'pipe' });
  const dist = join(tmp, 'package', 'dist', 'web');
  assert(existsSync(join(dist, 'install-button.js')), 'Published package lacks dist/web/install-button.js');
  assert(existsSync(join(tmp, 'package', 'LICENSE')), 'Missing Apache-2.0 license');
  rmSync(target, { recursive: true, force: true });
  mkdirSync(dirname(target), { recursive: true });
  cpSync(dist, target, { recursive: true });
  copyFileSync(join(tmp, 'package', 'LICENSE'), join(target, 'LICENSE'));
  writeFileSync(join(target, 'SOURCE.json'), JSON.stringify(lock, null, 2) + '\n');
  auditInstalledFiles();
} finally {
  rmSync(tmp, { recursive: true, force: true });
}
