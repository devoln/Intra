"use strict";
// PCM → FLAC (libflacjs = libFLAC, собранный в WASM) для веб-плеера.
//
// ЗАЧЕМ. Панели «сырые сэмплы» и A/B отдавались браузеру как WAV (16 бит PCM):
// 71 МБ и 238 МБ соответственно, страница грузилась долго. FLAC хранит ровно
// те же биты, но сжимает их в 3-4 раза (замер: 6.9 МБ WAV → 1.8 МБ FLAC), а
// браузеры (Chrome, Firefox, Safari, Edge) играют .flac нативно в <audio>.
//
// ПОЧЕМУ НЕ MP3. Владелец подгоняет атаку и щелчки на слух, а кодек с потерями
// размазывает транзиент пре-эхо — то есть портит именно тот признак, по
// которому идёт подгонка. FLAC возвращает исходные 16 бит без изменений.
//
// КАК. libflacjs грузит свой .wasm через fetch(), а fetch() в Node не умеет
// обычные пути файловой системы (только http(s)/data/file:), поэтому на время
// инициализации fetch подменяется и локальный файл отдаётся как Response.
// Подмена снимается сразу после готовности модуля, чтобы не влиять на
// остальные инструменты (они грузят свои WASM сами).
import fs from "node:fs";
import crypto from "node:crypto";
import { createRequire } from "node:module";

const require = createRequire(import.meta.url);

const BITS = 16;
// ТА ЖЕ оцифровка, что была у WAV-записи (writeWavMono в render.mjs):
// переход на FLAC не меняет ни одного сэмпла, только контейнер.
const toInt16 = (x) => Math.round(Math.max(-1, Math.min(1, x)) * 32767);

let Flac = null;
let ready = null;

/// Инициализация (один раз на процесс). Все кодировщики/декодеры ниже требуют
/// `await initFlac()` до первого вызова.
export function initFlac() {
  if (!ready) ready = load();
  return ready;
}

async function load() {
  const realFetch = globalThis.fetch;
  globalThis.fetch = async (url, opts) => {
    if (typeof url === "string" && !/^(https?|data|file):/.test(url)) {
      return new Response(fs.readFileSync(url), { headers: { "Content-Type": "application/wasm" } });
    }
    return realFetch(url, opts);
  };
  try {
    const Mod = require("libflacjs")("wasm");
    await new Promise((res) => Mod.on("ready", res));
    Flac = Mod;
  } finally {
    globalThis.fetch = realFetch;
  }
  return Flac;
}

function md5OfPcm(pcm) {
  const le = Buffer.alloc(pcm.length * 2);
  for (let i = 0; i < pcm.length; i++) le.writeInt16LE(pcm[i], i * 2);
  return crypto.createHash("md5").update(le).digest("hex");
}

/// Общее ядро обоих входов: PCM уже разложен в interleaved Int32.
/// `verify: true` заставляет libFLAC распаковать поток обратно и сверить его с
/// входом; дополнительно сверяем MD5 из STREAMINFO с MD5 поданного PCM — так
/// «тихий» сбой кодировщика невозможен (формат хранит MD5 исходных сэмплов).
function encodeInterleaved(pcm, ch, frames, sampleRate, compression) {
  const { Encoder } = require("libflacjs/lib/encoder");
  const enc = new Encoder(Flac, {
    sampleRate, channels: ch, bitsPerSample: BITS, compression,
    totalSamples: frames, verify: true,
  });
  enc.encode(pcm, frames);
  enc.encode(); // пустой вызов = финализация потока
  const out = enc.getSamples();
  const meta = enc.metadata;
  enc.destroy();
  const buffer = Buffer.from(out.buffer || out, out.byteOffset || 0, out.length);
  const ours = md5OfPcm(pcm);
  if (meta && meta.md5sum && meta.md5sum !== "0".repeat(32) && meta.md5sum !== ours) {
    throw new Error(`flacEncode: MD5 не сошёлся (${meta.md5sum} != ${ours}) — поток повреждён`);
  }
  return { buffer, metadata: meta };
}

/// Кодирование `channels` (массив Float-каналов одинаковой длины) в FLAC-файл.
/// Возвращает Buffer с готовым .flac.
export function flacEncode(channels, sampleRate, { compression = 5 } = {}) {
  if (!Flac) throw new Error("flacEncode: сначала await initFlac()");
  const ch = channels.length;
  const frames = channels[0].length;
  const pcm = new Int32Array(frames * ch);
  for (let c = 0; c < ch; c++) {
    const src = channels[c];
    for (let i = 0; i < frames; i++) pcm[i * ch + c] = toInt16(src[i]);
  }
  return encodeInterleaved(pcm, ch, frames, sampleRate, compression);
}

/// То же, но для УЖЕ оцифрованного PCM (массив Int16Array на канал).
///
/// Нужно миграции существующих WAV-файлов: нормировка ±1 не представляет
/// значение -32768 (clamp упирается в -1 и округление даёт -32767), то есть
/// через `flacEncode` такой сэмпл молча сдвинулся бы на 1. Здесь значения
/// попадают в поток как есть, поэтому «контейнер сменился, звук — нет»
/// проверяемо: декодированный поток обязан совпасть с входом побитно.
export function flacEncodeInt16(channels, sampleRate, { compression = 5 } = {}) {
  if (!Flac) throw new Error("flacEncodeInt16: сначала await initFlac()");
  const ch = channels.length;
  const frames = channels[0].length;
  const pcm = new Int32Array(frames * ch);
  for (let c = 0; c < ch; c++) {
    const src = channels[c];
    for (let i = 0; i < frames; i++) pcm[i * ch + c] = src[i];
  }
  return encodeInterleaved(pcm, ch, frames, sampleRate, compression);
}

/// Декодирование FLAC обратно в PCM: { samples: interleaved Uint8Array (16 бит LE),
/// channels, sampleRate, bitsPerSample }. Нужно для проверок (миграция сравнивает
/// результат с исходным WAV) и для замеров по сохранённым файлам.
export function flacDecode(buffer) {
  if (!Flac) throw new Error("flacDecode: сначала await initFlac()");
  const { Decoder } = require("libflacjs/lib/decoder");
  const dec = new Decoder(Flac, { verify: true });
  const ok = dec.decode(new Uint8Array(buffer));
  const samples = ok ? dec.getSamples(true) : null;
  const meta = dec.metadata;
  dec.destroy();
  if (!samples) throw new Error("flacDecode: декодирование не удалось");
  return { samples, metadata: meta };
}

/// Удобная запись моно-файла: тот же интерфейс, что был у writeWavMono.
export function writeFlacMono(file, x, sampleRate = 44100, opts) {
  const { buffer } = flacEncode([x], sampleRate, opts);
  fs.writeFileSync(file, buffer);
  return buffer;
}
