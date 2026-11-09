// Round-trip test: encode every record of the campaign datasets with the
// documented 20-byte frame layout, decode it with firmware/decoder/decoder.js,
// and compare with the values stored in the CSV files.
// Run: node tests/decoder.test.js
"use strict";
const fs = require("fs");
const path = require("path");
const vm = require("vm");

const root = path.resolve(__dirname, "..");
const code = fs.readFileSync(path.join(root, "firmware", "decoder", "decoder.js"), "utf8");
const ctx = {};
vm.createContext(ctx);
vm.runInContext(code, ctx);
const decodeUplink = ctx.decodeUplink;

function encode(r) {
  const b = new Array(20).fill(0);
  b[0] = ((r.GPSV & 0x3f) << 2) | ((r.BDSV >> 4) & 0x03);
  b[1] = ((r.BDSV & 0x0f) << 4) | ((r.GLOV >> 2) & 0x0f);
  b[2] = ((r.GLOV & 0x03) << 6) | (r.GALV & 0x3f);
  b[3] = ((r.SATU & 0x3f) << 2) | (r.FIXT & 0x03);
  b[4] = ((r.SATX & 1) << 7) | (Math.round(r.PDOP * 10) & 0x7f);
  b[5] = Math.round(r.HDOP * 10) & 0xff;
  b[6] = Math.round(r.VDOP * 10) & 0xff;
  const lat = Math.round(r.LATI * 1e6) >>> 0, lon = Math.round(r.LONG * 1e6) >>> 0;
  for (let i = 0; i < 4; i++) { b[7 + i] = (lat >>> (24 - 8 * i)) & 0xff; b[11 + i] = (lon >>> (24 - 8 * i)) & 0xff; }
  const alt = Math.round(r.ALTI * 10) & 0xffff;
  b[15] = alt >> 8; b[16] = alt & 0xff;
  b[17] = r.PITC & 0xff;
  b[18] = Math.round(r.ROLL / 2) & 0xff;
  b[19] = ((r.VERS & 0x0f) << 4) | (r.TXPW & 0x0f);
  return b;
}

function parseCsv(file) {
  const [head, ...rows] = fs.readFileSync(file, "utf8").trim().split(/\r?\n/);
  const cols = head.split(",");
  return rows.map((line) => {
    const v = line.split(",");
    const o = {};
    cols.forEach((c, i) => { o[c] = isNaN(Number(v[i])) ? v[i] : Number(v[i]); });
    return o;
  });
}

const dir = path.join(root, "data", "campaigns");
let n = 0, bad = 0;
for (const f of fs.readdirSync(dir).filter((x) => x.endsWith(".csv")).sort()) {
  for (const r of parseCsv(path.join(dir, f))) {
    n++;
    const res = decodeUplink({ bytes: encode(r) });
    const d = res.data;
    const ok = d &&
      ["VERS", "GPSV", "BDSV", "GLOV", "GALV", "SATU", "SATX", "FIXT", "PITC", "ROLL", "TXPW"].every((k) => d[k] === r[k]) &&
      Math.abs(d.PDOP - r.PDOP) < 1e-9 && Math.abs(d.HDOP - r.HDOP) < 1e-9 && Math.abs(d.VDOP - r.VDOP) < 1e-9 &&
      Math.abs(d.LATI - r.LATI) < 1e-6 && Math.abs(d.LONG - r.LONG) < 1e-6 && Math.abs(d.ALTI - r.ALTI) < 0.05 + 1e-9;
    if (!ok) { bad++; if (bad <= 5) console.error("MISMATCH", f, JSON.stringify(r), JSON.stringify(res)); }
  }
}
// Error handling
const e1 = decodeUplink({ bytes: [1, 2, 3] });
const e2 = decodeUplink({ bytes: new Array(20).fill(0) });
const rejects = Array.isArray(e1.errors) && Array.isArray(e2.errors);
console.log(`records: ${n}, mismatches: ${bad}, invalid payloads rejected: ${rejects}`);
process.exit(bad === 0 && rejects ? 0 : 1);
