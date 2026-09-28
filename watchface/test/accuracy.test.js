const test = require('node:test');
const assert = require('node:assert/strict');
global.Pebble = { addEventListener() {}, openURL() {}, sendAppMessage() {} };
const api = require('../src/pkjs/index.js');

test('DOP is unavailable with fewer than four directions', () => {
  assert.equal(api.computeDOP([[1, 0, 0, 1], [0, 1, 0, 1], [0, 0, 1, 1]]), null);
});

test('DOP rejects singular geometry instead of inventing a fallback', () => {
  const sameDirection = Array.from({length: 4}, () => [1, 0, 0, 1]);
  assert.equal(api.computeDOP(sameDirection), null);
});

test('DOP solves a symmetric tetrahedral geometry', () => {
  const q = 1 / Math.sqrt(3);
  const rows = [[q,q,q,1], [q,-q,-q,1], [-q,q,-q,1], [-q,-q,q,1]];
  const d = api.computeDOP(rows);
  assert.ok(Math.abs(d.pdop - 1.5) < 1e-8);
  assert.ok(Math.abs(d.tdop - 0.5) < 1e-8);
  assert.ok(Math.abs(d.gdop - Math.sqrt(2.5)) < 1e-8);
});

test('catalog number comes from the TLE satellite-number field', () => {
  const lines = ['GPS BIIR-2  (PRN 13)',
    '1 24876U 97035A   26271.50000000 -.00000071  00000+0  00000+0 0  9990',
    '2 24876  55.0000 120.0000 0001000  20.0000 340.0000  2.00000000123456'];
  const sats = api.parseTleCatalog(lines, 1);
  assert.equal(sats.length, 1);
  assert.equal(sats[0].catalogId, 24876);
});
