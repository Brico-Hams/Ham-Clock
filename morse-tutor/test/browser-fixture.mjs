import { readFileSync } from 'node:fs';
import { createServer } from 'node:http';
import assert from 'node:assert/strict';

// Serves the actual embedded HTML/catalog with simulated device responses, not ESP-NOW.
const catalogSource = readFileSync(new URL('../include/Languages.h', import.meta.url), 'utf8');
const catalog = Object.fromEntries([...catalogSource.matchAll(/^X\((\w+),(.*)\) ?\\?$/gm)]
  .map(([, key, values]) => [key, JSON.parse(`[${values}]`)]));
assert(Object.keys(catalog).length > 90);
for (const values of Object.values(catalog)) assert.equal(values.length, 5);
const source = readFileSync(new URL('../src/network_page.h', import.meta.url), 'utf8');
const state = { ssid: 'Bricohams-CW-TEST', decode: true, pending: 1, drops: 0, invalid: 0,
  sendFailures: 0, language: 0, vocabulary: 0, sequence: 0, wordGap: 1, custom: 'KM',
  memories: Array.from({length: 8}, (_, i) => `CQ MEMORY ${i + 1}`),
  groups: 2, correct: 1, scores: [['K', 2, 0], ['M', 2, 1]],
  messages: [{from: 'NOCALL', tx: false, text: 'HOLA RED 73', morse: '.... --- .-.. .- / .-. . -..'}] };
const normalized = text => text.replace(/ß|ẞ/g, 'SS').replace(/œ|Œ/g, 'OE')
  .replace(/æ|Æ/g, 'AE').normalize('NFD').replace(/\p{M}/gu, '').toUpperCase().replace(/\s+/g, ' ').trim();
const valid = text => text.length <= 120 && /^[A-Z0-9 .,?/+=@\-:;()!'"&$]+$/.test(text);
createServer((req, res) => {
  const reply = (code, body, type = 'text/plain; charset=utf-8') => {
    res.writeHead(code, {'Content-Type': type}); res.end(body);
  };
  const tr = id => catalog[id][state.language];
  if (req.url === '/') {
    const html = source.split('R"HTML(')[1].split(')HTML"')[0].replace('@TOKEN@', 'fixture-token')
      .replace('@CATALOG@', JSON.stringify(catalog)).replace('@LANGUAGE@', String(state.language));
    return reply(200, html, 'text/html; charset=utf-8');
  }
  if (req.url === '/api/state') return reply(200, JSON.stringify({...state,
    messages: state.messages.map(m => {
      if (state.decode || m.tx) return m;
      const {text, ...hidden} = m; return hidden;
    })}), 'application/json');
  let body = '';
  req.on('data', chunk => body += chunk);
  req.on('end', () => {
    if (req.headers['x-bricohams-token'] !== 'fixture-token') return reply(403, tr('Unauthorized'));
    if (req.url === '/api/language' && /^[0-4]$/.test(body)) {
      state.language = Number(body); return reply(200, tr('Saved'));
    }
    if (req.url === '/api/decode' && /^[01]$/.test(body)) {
      state.decode = body === '1'; return reply(200, tr('DecodeSaved'));
    }
    if (req.url === '/api/trainer') {
      const form = new URLSearchParams(body);
      const custom = form.get('custom') ?? '';
      if (!/^[0-4]$/.test(form.get('vocabulary')) || !/^[0-3]$/.test(form.get('sequence')) ||
          !/^[1-5]$/.test(form.get('wordGap')) || custom.length < 2 || custom.length > 40 ||
          new Set(custom).size !== custom.length || !valid(custom) || custom.includes(' '))
        return reply(400, tr('InvalidInput'));
      for (const id of ['vocabulary', 'sequence', 'wordGap']) state[id] = Number(form.get(id));
      state.custom = custom; return reply(200, tr('Saved'));
    }
    const separator = body.indexOf('\n'), value = normalized(body.slice(separator + 1));
    if (req.url === '/api/memory' && separator === 1 && /^[0-7]$/.test(body[0])) {
      if (value && !valid(value)) return reply(400, tr('InvalidInput'));
      state.memories[Number(body[0])] = value; return reply(200, tr('Saved'));
    }
    if (req.url === '/api/send' && /^[A-Z0-9/]{1,9}$/i.test(body.slice(0, separator)) && valid(value)) {
      state.messages.push({from: body.slice(0, separator).toUpperCase(), tx: true, text: value, morse: '-.-. --'});
      return reply(202, tr('Queued'));
    }
    reply(400, tr('InvalidInput'));
  });
}).listen(8174, '127.0.0.1', () => console.log('Multilingual UI fixture ready: http://127.0.0.1:8174'));
