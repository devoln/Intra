// Intra MIDI Synth — browser player driving the Emscripten-compiled
// MusicSynthesizer WASM module.
//
// The C side exposes:
//   _SourceCreateFromMidiFileData(dataPtr, len, sampleRate, numChannels) -> source*
//   _SourceCreateLive(sampleRate, numChannels) -> source* (бесконечный поток тишины)
//   _SourceFree(source*)
//   _SourceSamplesLeft(source*) -> uint (samples per channel remaining)
//   _SourceGetUninterleavedSamples(source*, dstPtr, count, bufferSizeInSamples) -> uint
//   _SourceSendMidiEvent(source*, status, data0, data1) -> void
//   _GetMidiInfoString(dataPtr, len) -> char* (malloc'd, must be freed)
//
// sendMidiEvent() is the single live-input entry point: it forwards one raw
// MIDI message (status + up to two data bytes, as in Web MIDI's data arrays)
// into the current stream (a loaded song or the live source). It replaces the
// old custom APIs (e.g. SourceSetProgram) and is what the Web MIDI keyboard
// and the on-screen piano both use.
//
// Default playback pulls rendered samples from the WASM module in real time
// via a ScriptProcessorNode. When the "full pre-generation" checkbox is on,
// the whole song is first rendered into JS float buffers (with the measured
// wall-clock time shown in the UI), and playback then reads from those
// buffers with zero WASM calls per audio callback.

(() => {
  "use strict";

  const AUDIO_CHUNK = 8192; // max samples pulled per fast-forward step
  const PROC_BUFFER = 256; // ScriptProcessor buffer size (min для live-задержки)
  const MAX_PREGEN_BYTES = 512 * 1024 * 1024; // cap for the offline buffer

  const els = {
    drop: document.getElementById("drop"),
    fileInput: document.getElementById("fileInput"),
    urlInput: document.getElementById("urlInput"),
    urlSuggest: document.getElementById("urlSuggest"),
    loadUrl: document.getElementById("loadUrl"),
    status: document.getElementById("status"),
    info: document.getElementById("info"),
    player: document.getElementById("player"),
    title: document.getElementById("trackTitle"),
    playBtn: document.getElementById("playBtn"),
    stopBtn: document.getElementById("stopBtn"),
    seek: document.getElementById("seek"),
    time: document.getElementById("time"),
    volume: document.getElementById("volume"),
    volLabel: document.getElementById("volLabel"),
    pregen: document.getElementById("pregen"),
    pregenResult: document.getElementById("pregenResult"),
    liveBtn: document.getElementById("liveBtn"),
    allNotesOff: document.getElementById("allNotesOff"),
    instrument: document.getElementById("instrument"),
    drumsCh: document.getElementById("drumsCh"),
    midiStatus: document.getElementById("midiStatus"),
    piano: document.getElementById("piano"),
    audioSampleRate: document.getElementById("audioSampleRate"),
  };

  // Live-режим: игра без MIDI-файла на бесконечном источнике тишины.
  let liveMode = false;
  let liveChannel = 0; // 9 (канал ударных), когда включены ударные
  let currentProgram = 0; // выбранный GM-инструмент (Program Change)
  let midiAccess = null;
  const pressedNotes = new Map(); // pointerId -> { note, channel } на экранном пианино

  let Module = null;
  let audioCtx = null;
  let processor = null;
  let gainNode = null;
  let scratchPtr = 0;

  let currentSource = 0; // WASM pointer, 0 = none
  let midiBytes = null; // last loaded MIDI bytes (for seek/replay)
  let midiName = "MIDI";
  let totalSamples = 0;
  let playedSamples = 0;
  let paused = true;
  let seeking = false;

  // Fully rendered song (used when the "full pre-generation" box is ticked).
  // Populated once per loaded file; playback then reads from these plain JS
  // arrays instead of pulling from the WASM source on every callback.
  let pregenAudio = null;   // { left: Float32Array, right: Float32Array, len }
  let pregenMs = null;      // last full-render duration in ms
  let pregenPos = 0;        // read position inside pregenAudio

  function fmtTime(sec) {
    if (!isFinite(sec) || sec < 0) sec = 0;
    const m = Math.floor(sec / 60);
    const s = Math.floor(sec % 60);
    return m + ":" + String(s).padStart(2, "0");
  }

  function setStatus(text, isError) {
    els.status.textContent = text;
    els.status.classList.toggle("error", !!isError);
  }

  function setEnabled(id, enabled) {
    document.getElementById(id).disabled = !enabled;
  }

  function escapeHtml(s) {
    return s.replace(/[&<>"']/g, (c) => ({
      "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;",
    }[c]));
  }

  function updateProgressUI() {
    if (!totalSamples || !audioCtx) return;
    const duration = totalSamples / audioCtx.sampleRate;
    const pos = playedSamples / audioCtx.sampleRate;
    els.seek.max = String(Math.max(1, Math.floor(totalSamples)));
    els.seek.value = String(Math.min(playedSamples, totalSamples));
    els.time.textContent = fmtTime(pos) + " / " + fmtTime(duration);
    const pct = totalSamples ? (playedSamples / totalSamples) * 100 : 0;
    els.seek.style.setProperty("--pct", pct + "%");
  }

  function ensureAudio() {
    if (!audioCtx) {
      audioCtx = new (window.AudioContext || window.webkitAudioContext)();
      if (els.audioSampleRate) els.audioSampleRate.textContent = audioCtx.sampleRate + " Hz";
      setStatus("Web Audio готов: " + audioCtx.sampleRate + " Hz");
      gainNode = audioCtx.createGain();
      gainNode.gain.value = parseFloat(els.volume.value);
      processor = audioCtx.createScriptProcessor(PROC_BUFFER, 0, 2);
      processor.onaudioprocess = onAudioProcess;
      processor.connect(gainNode);
      gainNode.connect(audioCtx.destination);
    }
    if (audioCtx.state === "suspended") audioCtx.resume().catch(() => {});
    return audioCtx;
  }

  function onAudioProcess(e) {
    const outL = e.outputBuffer.getChannelData(0);
    const outR = e.outputBuffer.getChannelData(1);
    const n = outL.length;

    if ((!currentSource && !pregenAudio) || paused || !Module) {
      outL.fill(0);
      outR.fill(0);
      return;
    }

    if (pregenAudio) {
      // Playback from the fully rendered buffer: zero WASM calls per callback.
      const len = pregenAudio.len;
      const copy = Math.min(n, Math.max(0, len - pregenPos));
      if (copy > 0) {
        outL.set(pregenAudio.left.subarray(pregenPos, pregenPos + copy), 0);
        outR.set(pregenAudio.right.subarray(pregenPos, pregenPos + copy), 0);
      }
      for (let i = copy; i < n; i++) {
        outL[i] = 0;
        outR[i] = 0;
      }
      pregenPos += n;
      playedSamples = Math.min(pregenPos, len);
      updateProgressUI();
      if (pregenPos >= len) stopPlayback();
      return;
    }

    // Channel 0 -> [0 .. n), channel 1 -> [n .. 2n) in the scratch buffer.
    const written = Module._SourceGetUninterleavedSamples(
      currentSource, scratchPtr, n, n
    );
    const heap = Module.HEAPF32;
    const off = scratchPtr >> 2;
    for (let i = 0; i < n; i++) {
      outL[i] = heap[off + i];
      outR[i] = heap[off + n + i];
    }
    for (let i = written; i < n; i++) {
      outL[i] = 0;
      outR[i] = 0;
    }

    playedSamples += n;
    updateProgressUI();

    if (written === 0 || Module._SourceSamplesLeft(currentSource) === 0) {
      stopPlayback();
    }
  }

  function freeSource() {
    if (currentSource && Module) Module._SourceFree(currentSource);
    currentSource = 0;
  }

  // Parses bytes and builds a synth source. Returns { src, info }.
  function createSource(bytes) {
    const len = bytes.byteLength;
    const dataPtr = Module._malloc(len);
    Module.HEAPU8.set(bytes, dataPtr);

    let infoPtr = 0;
    let infoText = "";
    try {
      infoPtr = Module._GetMidiInfoString(dataPtr, len);
      infoText = infoPtr ? Module.UTF8ToString(infoPtr) : "";
    } finally {
      if (infoPtr) Module._free(infoPtr);
    }

    const srcPtr = Module._SourceCreateFromMidiFileData(
      dataPtr, len, audioCtx.sampleRate, 2
    );
    Module._free(dataPtr);

    if (!srcPtr) {
      throw new Error(infoText || "Не удалось разобрать MIDI файл");
    }
    return { src: srcPtr, info: infoText };
  }

  function loadFromBytes(bytes, name) {
    if (liveMode) stopLive();
    releaseAllPianoNotes();
    freeSource();
    midiBytes = bytes;
    midiName = name || "MIDI";
    els.title.textContent = midiName;
    paused = true;
    playedSamples = 0;
    totalSamples = 0;
    setPlayIcon(true);

    // Reset the offline-render state for the new file.
    pregenAudio = null;
    pregenMs = null;
    pregenPos = 0;
    els.pregenResult.textContent = "Офлайн-рендер: —";
    els.pregenResult.classList.remove("error");

    try {
      ensureAudio();
      const { src, info } = createSource(bytes);
      currentSource = src;
      totalSamples = Module._SourceSamplesLeft(src);
      const lines = (info || "").split("\n").filter(Boolean);
      els.info.innerHTML = lines.length
        ? lines.map((l) => `<span>${escapeHtml(l)}</span>`).join("")
        : "<span>MIDI загружен</span>";
      els.player.classList.remove("hidden");
      setEnabled("playBtn", true);
      setEnabled("stopBtn", true);
      setEnabled("seek", true);
      setStatus("Готов к воспроизведению");
      updateProgressUI();
    } catch (err) {
      midiBytes = null;
      setStatus(err.message || "Ошибка загрузки", true);
      els.info.innerHTML = "";
    }
  }

  async function loadFromUrl(url) {
    setStatus("Загрузка MIDI по URL…");
    let bytes;
    try {
      const res = await fetch(url);
      if (!res.ok) throw new Error("HTTP " + res.status);
      bytes = new Uint8Array(await res.arrayBuffer());
    } catch (e) {
      // Direct fetch may fail due to CORS; retry through a public proxy.
      try {
        const res = await fetch(
          "https://api.allorigins.win/raw?url=" + encodeURIComponent(url)
        );
        if (!res.ok) throw new Error("HTTP " + res.status);
        bytes = new Uint8Array(await res.arrayBuffer());
      } catch (e2) {
        setStatus(
          "Не удалось загрузить URL (CORS/сеть). Скачайте файл и откройте локально.",
          true
        );
        return;
      }
    }
    const name =
      decodeURIComponent(url.split("/").pop().split("?")[0]) || "MIDI";
    loadFromBytes(bytes, name);
  }

  function setPlayIcon(play) {
    els.playBtn.innerHTML = play
      ? '<svg viewBox="0 0 24 24"><path d="M8 5v14l11-7z"/></svg> Играть'
      : '<svg viewBox="0 0 24 24"><path d="M6 5h4v14H6zM14 5h4v14h-4z"/></svg> Пауза';
  }

  // Fully renders the current WASM source into JS float buffers, measuring
  // the wall-clock time. Yields periodically so the status bar keeps its
  // progress percentage visible.
  async function generateAll() {
    if (!currentSource || !Module || !totalSamples) return null;
    const bytes = totalSamples * 2 * 4;
    if (bytes > MAX_PREGEN_BYTES) {
      setStatus("Файл слишком длинный для полной генерации", true);
      return null;
    }
    const left = new Float32Array(totalSamples);
    const right = new Float32Array(totalSamples);
    const heap = Module.HEAPF32;
    const off = scratchPtr >> 2;
    const t0 = performance.now();
    let pos = 0;
    let reportCounter = 0;
    while (pos < totalSamples) {
      const n = Math.min(AUDIO_CHUNK, totalSamples - pos);
      const written = Module._SourceGetUninterleavedSamples(
        currentSource, scratchPtr, n, AUDIO_CHUNK
      );
      if (written > 0) {
        left.set(heap.subarray(off, off + written), pos);
        right.set(heap.subarray(off + AUDIO_CHUNK, off + AUDIO_CHUNK + written), pos);
      }
      pos += written;
      if (written === 0) break;
      if (++reportCounter % 16 === 0) {
        setStatus("Полная генерация… " + Math.round(pos * 100 / totalSamples) + "%");
        await new Promise((r) => setTimeout(r, 0));
      }
    }
    const ms = performance.now() - t0;
    return { left, right, len: pos, ms };
  }

  async function playPause() {
    if (!currentSource && !pregenAudio) return;
    ensureAudio();

    // Offline render first (with timing) when the checkbox is on and there is
    // no generated buffer yet. If generation fails, stay paused.
    if (els.pregen.checked && !pregenAudio && currentSource) {
      paused = true;
      setPlayIcon(true);
      setStatus("Полная генерация…");
      const result = await generateAll();
      if (!result || !result.len) {
        if (totalSamples) {
          setStatus("Полная генерация не дала звука", true);
          els.pregenResult.textContent = "Офлайн-рендер: ошибка";
          els.pregenResult.classList.add("error");
        }
        return;
      }
      pregenAudio = result;
      pregenMs = result.ms;
      pregenPos = 0;
      playedSamples = 0;
      const durSec = result.len / audioCtx.sampleRate;
      const rt = durSec > 0 ? (result.ms / 1000) / durSec : 0;
      els.pregenResult.textContent =
        "Офлайн-рендер: " + result.ms.toFixed(1) + " мс (x" + rt.toFixed(3) +
        " реального времени, " + durSec.toFixed(1) + " с)";
      els.pregenResult.classList.remove("error");
      // The WASM source has been consumed by the offline render; playback now
      // reads straight from the float buffers.
      freeSource();
    }

    paused = !paused;
    setPlayIcon(paused);
    setStatus(paused ? "Пауза" : "Воспроизведение…");
  }

  function stopPlayback() {
    if (liveMode) return; // у live-источника нет конца, чтобы останавливаться
    if (!currentSource && !pregenAudio) return;
    paused = true;
    setPlayIcon(true);

    if (pregenAudio) {
      // Fast restart from the in-memory render — no need to re-synthesize.
      pregenPos = 0;
      playedSamples = 0;
    } else if (midiBytes) {
      try {
        const { src } = createSource(midiBytes);
        freeSource();
        currentSource = src;
      } catch (err) {
        freeSource();
        setStatus(err.message || "Ошибка", true);
        return;
      }
    }
    els.seek.value = "0";
    els.seek.style.setProperty("--pct", "0%");
    els.time.textContent = "0:00 / 0:00";
    updateProgressUI();
    setStatus("Остановлено. Нажмите «Играть», чтобы воспроизвести с начала.");
  }

  async function seekTo(sample) {
    if (!midiBytes) return;
    const target = Math.max(0, Math.min(sample, totalSamples));

    if (pregenAudio) {
      // Seek straight in the offline-rendered buffer — no WASM re-synthesis.
      pregenPos = target;
      playedSamples = target;
      updateProgressUI();
      return;
    }

    if (!currentSource || seeking) return;
    seeking = true;
    const wasPaused = paused;
    paused = true;
    setStatus("Перемотка…");

    const { src } = createSource(midiBytes);
    freeSource();
    currentSource = src;

    let remaining = target;
    while (remaining > 0) {
      const n = Math.min(AUDIO_CHUNK, remaining);
      const written = Module._SourceGetUninterleavedSamples(
        currentSource, scratchPtr, n, AUDIO_CHUNK
      );
      if (written === 0) break;
      remaining -= written;
      // Yield so the UI stays responsive on long seeks.
      await new Promise((r) => setTimeout(r, 0));
    }

    playedSamples = target - remaining;
    paused = wasPaused;
    seeking = false;
    updateProgressUI();
    setStatus(wasPaused ? "Готов к воспроизведению" : "Воспроизведение…");
  }

  // Sends one raw MIDI message into the current stream. status is the status
  // byte with the channel, e.g. 0x90 = Note On on channel 0; data0/data1 are
  // the data bytes (NoteOn: note, velocity; ProgramChange: program; CC123:
  // All Notes Off, etc.). The C side applies the event at the current stream
  // position, so it sounds immediately. Returns false when no source is loaded.
  function sendMidiEvent(status, data0, data1) {
    if (!Module || !currentSource) return false;
    Module._SourceSendMidiEvent(
      currentSource, status & 0xFF, data0 & 0xFF, data1 & 0xFF
    );
    return true;
  }

  function allNotesOff() {
    if (!Module || !currentSource) return;
    for (let ch = 0; ch < 16; ch++) sendMidiEvent(0xB0 | ch, 0x7B, 0);
  }

  // Starts the infinite live source (pure playing, no MIDI file) and switches
  // the audio output to it.
  // Starts the infinite live source (pure playing, no MIDI file). НЕ удаляет
  // загруженную песню: midiBytes сохраняется, чтобы stopLive() мог вернуть
  // её на место (раньше песня терялась безвозвратно).
  function startLive() {
    if (!Module) return false;
    ensureAudio();
    releaseAllPianoNotes();
    // Полностью отрендеренный буфер несовместим с живым источником в
    // onAudioProcess (он главнее) — выбрасываем буфер, но не байты песни.
    pregenAudio = null;
    pregenMs = null;
    pregenPos = 0;
    freeSource();
    totalSamples = 0;
    playedSamples = 0;
    paused = false;
    liveMode = true;
    currentSource = Module._SourceCreateLive(audioCtx.sampleRate, 2);
    els.player.classList.add("hidden");
    els.liveBtn.classList.add("btn-active");
    els.liveBtn.innerHTML =
      '<svg viewBox="0 0 24 24"><path d="M12 3v10.55A4 4 0 1 0 14 17V7h4V3h-6z"/></svg> Live вкл.';
    setStatus("Live-режим: играйте на экранном пианино или MIDI-клавиатуре");
    if (!els.drumsCh.checked) sendMidiEvent(0xC0 | liveChannel, currentProgram, 0);
    return true;
  }

  function stopLive() {
    if (!liveMode) return;
    allNotesOff();
    releaseAllPianoNotes();
    freeSource();
    liveMode = false;
    els.liveBtn.classList.remove("btn-active");
    els.liveBtn.innerHTML =
      '<svg viewBox="0 0 24 24"><path d="M12 3v10.55A4 4 0 1 0 14 17V7h4V3h-6z"/></svg> Live';
    if (midiBytes) {
      // Возвращаем загруженную песню (с начала — источник был выключен).
      try {
        const { src } = createSource(midiBytes);
        currentSource = src;
        totalSamples = Module._SourceSamplesLeft(src);
        playedSamples = 0;
        els.player.classList.remove("hidden");
        setEnabled("playBtn", true);
        setEnabled("stopBtn", true);
        setEnabled("seek", true);
        updateProgressUI();
        setStatus("Песня восстановлена — нажмите «Играть»");
      } catch (err) {
        setStatus("Live-режим выключен (песня не восстановилась: " + err.message + ")", true);
      }
      return;
    }
    setStatus("Live-режим выключен");
  }


  // ---- Экранное пианино --------------------------------------------------

  const PIANO_WHITE = [0, 2, 4, 5, 7, 9, 11]; // смещения белых клавиш в октаве
  // Чёрная клавиша стоит после белой с индексом i в октаве (смещение = полутон).
  const PIANO_BLACK_AFTER = { 0: 1, 1: 3, 3: 6, 4: 8, 5: 10 };
  const NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
  const PIANO_START = 48; // C3
  const PIANO_OCTAVES = 2; // C3..B4

  function buildPiano() {
    const piano = els.piano;
    piano.innerHTML = "";
    const whites = [];
    for (let oct = 0; oct < PIANO_OCTAVES; oct++) {
      for (const w of PIANO_WHITE) {
        const note = PIANO_START + oct * 12 + w;
        const el = document.createElement("div");
        el.className = "key white";
        el.dataset.note = note;
        const label = document.createElement("span");
        label.className = "label";
        label.textContent = NOTE_NAMES[w] + (oct + 3);
        el.appendChild(label);
        piano.appendChild(el);
        whites.push(el);
      }
    }
    const totalWhites = whites.length;
    for (let oct = 0; oct < PIANO_OCTAVES; oct++) {
      for (const [afterWhite, semitone] of Object.entries(PIANO_BLACK_AFTER)) {
        const note = PIANO_START + oct * 12 + semitone;
        const el = document.createElement("div");
        el.className = "key black";
        el.dataset.note = note;
        const boundary = oct * 7 + parseInt(afterWhite, 10) + 1;
        el.style.left = (boundary / totalWhites * 100 - 3.5) + "%";
        piano.appendChild(el);
      }
    }
  }

  function releasePointer(e) {
    const entry = pressedNotes.get(e.pointerId);
    if (!entry) return;
    pressedNotes.delete(e.pointerId);
    sendMidiEvent(0x80 | entry.channel, entry.note, 0);
    const key = els.piano.querySelector('[data-note="' + entry.note + '"]');
    if (key) key.classList.remove("active");
  }

  function releaseAllPianoNotes() {
    for (const entry of pressedNotes.values()) {
      sendMidiEvent(0x80 | entry.channel, entry.note, 0);
      const key = els.piano.querySelector('[data-note="' + entry.note + '"]');
      if (key) key.classList.remove("active");
    }
    pressedNotes.clear();
  }

  // ---- Web MIDI ----------------------------------------------------------

  function setMidiStatus(text, off) {
    els.midiStatus.textContent = text;
    els.midiStatus.classList.toggle("off", !!off);
  }

  function initMidiAccess() {
    if (!navigator.requestMIDIAccess) {
      setMidiStatus("Web MIDI не поддерживается — экранное пианино работает", true);
      return;
    }
    navigator.requestMIDIAccess({ sysex: false })
      .then((access) => {
        midiAccess = access;
        access.onstatechange = () => connectMidiInputs();
        connectMidiInputs();
      })
      .catch(() => {
        setMidiStatus("MIDI-устройства недоступны (нужно разрешение)", true);
      });
  }

  function connectMidiInputs() {
    if (!midiAccess) return;
    const names = [];
    for (const input of midiAccess.inputs.values()) {
      input.onmidimessage = onMidiMessage;
      names.push(input.name || "MIDI-вход");
    }
    if (names.length) setMidiStatus("MIDI: " + names.join(", "));
    else setMidiStatus("MIDI-устройств нет — играйте на экранном пианино", true);
  }

  // Прямая пересылка сырых байтов Web MIDI в текущий поток: нота, CC,
  // pitch bend, program change — как есть, байт-в-байт.
  function onMidiMessage(e) {
    const data = e.data;
    if (!data || data.length < 1) return;
    const status = data[0];
    if (status < 0x80) return;
    const data0 = data.length > 1 ? data[1] : 0;
    const data1 = data.length > 2 ? data[2] : 0;
    // MIDI-клавиатура звучит только в Live-режиме: при выключенном live — тишина.
    if (!liveMode) return;
    sendMidiEvent(status, data0, data1);
  }

  // ---- Инструменты (GM Program Change) -----------------------------------

  // Только программы, у которых в маппинге реально есть инструмент (остальные
  // в синтезаторе молчат, как и в web-midisynth).
  const GM_GROUPS = [
    ["Фортепиано", [[0, "Acoustic Grand Piano"], [1, "Bright Acoustic Piano"], [2, "Electric Grand Piano"], [3, "Honky-tonk Piano"], [4, "Electric Piano 1"], [5, "Electric Piano 2"], [6, "Harpsichord"], [7, "Clavinet"]]],
    ["Хроматические ударные", [[8, "Celesta"], [9, "Glockenspiel"], [10, "Music Box"], [11, "Vibraphone"], [12, "Marimba"], [13, "Xylophone"], [14, "Tubular Bells"], [15, "Dulcimer"]]],
    ["Органы", [[16, "Drawbar Organ"], [17, "Percussive Organ"], [18, "Rock Organ"], [19, "Church Organ"], [20, "Reed Organ"], [21, "Accordion"], [22, "Harmonica"], [23, "Tango Accordion"]]],
    ["Гитары", [[24, "Acoustic Guitar (nylon)"], [25, "Acoustic Guitar (steel)"], [26, "Electric Guitar (jazz)"], [27, "Electric Guitar (clean)"], [28, "Electric Guitar (muted)"], [29, "Overdriven Guitar"], [30, "Distortion Guitar"], [31, "Guitar Harmonics"]]],
    ["Басы", [[32, "Acoustic Bass"], [33, "Electric Bass (finger)"], [34, "Electric Bass (pick)"], [35, "Fretless Bass"], [36, "Slap Bass 1"], [37, "Slap Bass 2"], [38, "Synth Bass 1"], [39, "Synth Bass 2"]]],
    ["Струнные", [[40, "Violin"], [41, "Viola"], [42, "Cello"], [44, "Tremolo Strings"], [45, "Pizzicato Strings"], [46, "Orchestral Harp"], [47, "Timpani"]]],
    ["Ансамбли", [[48, "String Ensemble 1"], [49, "String Ensemble 2"], [50, "Synth Strings 1"], [51, "Synth Strings 2"], [52, "Choir Aahs"], [53, "Voice Oohs"], [54, "Synth Voice"], [55, "Orchestra Hit"]]],
    ["Медь", [[56, "Trumpet"], [57, "Trombone"], [58, "Tuba"], [59, "Muted Trumpet"], [60, "French Horn"], [61, "Brass Section"], [62, "Synth Brass 1"], [63, "Synth Brass 2"]]],
    ["Духовые", [[64, "Soprano Sax"], [65, "Alto Sax"], [66, "Tenor Sax"], [67, "Baritone Sax"], [68, "Oboe"], [69, "English Horn"], [70, "Bassoon"], [71, "Clarinet"]]],
    ["Флейты", [[72, "Piccolo"], [73, "Flute"], [74, "Recorder"], [75, "Pan Flute"], [76, "Blown Bottle"], [77, "Shakuhachi"], [78, "Whistle"], [79, "Ocarina"]]],
    ["Синт-лиды", [[80, "Lead 1 (square)"], [81, "Lead 2 (sawtooth)"], [82, "Lead 3 (calliope)"], [83, "Lead 4 (chiff)"], [84, "Lead 5 (charang)"], [85, "Lead 6 (voice)"], [86, "Lead 7 (fifths)"], [87, "Lead 8 (bass + lead)"]]],
    ["Синт-пэды", [[88, "Pad 1 (new age)"], [89, "Pad 2 (warm)"], [90, "Pad 3 (polysynth)"], [91, "Pad 4 (choir)"], [92, "Pad 5 (bowed)"], [93, "Pad 6 (metallic)"], [94, "Pad 7 (halo)"], [95, "Pad 8 (sweep)"]]],
    ["Синт-эффекты", [[96, "FX 1 (rain)"], [97, "FX 2 (soundtrack)"], [98, "FX 3 (crystal)"], [99, "FX 4 (atmosphere)"], [100, "FX 5 (brightness)"], [101, "FX 6 (goblins)"], [102, "FX 7 (echoes)"], [103, "FX 8 (sci-fi)"]]],
    ["Этнические", [[104, "Sitar"], [105, "Banjo"], [106, "Shamisen"], [107, "Koto"], [108, "Kalimba"], [109, "Bag Pipe"], [110, "Fiddle"], [111, "Shanai"]]],
    ["Мелодические ударные", [[112, "Tinkle Bell"], [113, "Agogo"], [114, "Steel Drums"]]],
    ["Звуковые эффекты", [[119, "Reverse Cymbal"], [122, "Seashore"], [124, "Telephone Ring"], [125, "Helicopter"], [126, "Applause"], [127, "Gunshot"]]],
  ];

  function buildInstrumentSelect() {
    const sel = els.instrument;
    sel.innerHTML = "";
    for (const [group, items] of GM_GROUPS) {
      const og = document.createElement("optgroup");
      og.label = group;
      for (const [prog, name] of items) {
        const opt = document.createElement("option");
        opt.value = String(prog);
        opt.textContent = prog + " · " + name;
        og.appendChild(opt);
      }
      sel.appendChild(og);
    }
    sel.value = String(currentProgram);
  }

  // ---- Wiring ------------------------------------------------------------

  els.fileInput.addEventListener("change", (e) => {
    const file = e.target.files && e.target.files[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = () =>
      loadFromBytes(new Uint8Array(reader.result), file.name);
    reader.onerror = () => setStatus("Не удалось прочитать файл", true);
    reader.readAsArrayBuffer(file);
    e.target.value = "";
  });

  els.loadUrl.addEventListener("click", () => {
    const url = els.urlInput.value.trim();
    if (url) loadFromUrl(url);
  });

  els.urlInput.addEventListener("keydown", (e) => {
    if (e.key === "Enter" && els.urlInput.value.trim()) {
      loadFromUrl(els.urlInput.value.trim());
    }
  });

  // Подсказки с тестовыми MIDI: показываются по клику/фокусу на поле URL,
  // скрываются при выборе, клике вне или Escape.
  function showUrlSuggest() {
    els.urlSuggest.hidden = false;
  }
  function hideUrlSuggest() {
    els.urlSuggest.hidden = true;
  }
  els.urlInput.addEventListener("focus", showUrlSuggest);
  els.urlInput.addEventListener("click", showUrlSuggest);
  els.urlSuggest.addEventListener("click", (e) => {
    const item = e.target.closest(".sug-item");
    if (!item) return;
    const url = item.dataset.url;
    els.urlInput.value = url;
    hideUrlSuggest();
    loadFromUrl(url);
  });
  els.urlSuggest.addEventListener("keydown", (e) => {
    if (e.key === "Escape") hideUrlSuggest();
  });
  document.addEventListener("click", (e) => {
    if (!e.target.closest(".urlrow")) hideUrlSuggest();
  });

  els.drop.addEventListener("click", () => els.fileInput.click());

  els.drop.addEventListener("dragover", (e) => {
    e.preventDefault();
    els.drop.classList.add("dragging");
  });
  els.drop.addEventListener("dragleave", () => {
    els.drop.classList.remove("dragging");
  });
  els.drop.addEventListener("drop", (e) => {
    e.preventDefault();
    els.drop.classList.remove("dragging");
    const file = e.dataTransfer.files && e.dataTransfer.files[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = () =>
      loadFromBytes(new Uint8Array(reader.result), file.name);
    reader.readAsArrayBuffer(file);
  });

  els.playBtn.addEventListener("click", playPause);
  els.stopBtn.addEventListener("click", stopPlayback);

  els.liveBtn.addEventListener("click", () => {
    if (liveMode) stopLive();
    else startLive();
  });

  els.allNotesOff.addEventListener("click", () => {
    releaseAllPianoNotes();
    allNotesOff();
    setStatus("Все ноты отпущены");
  });

  els.instrument.addEventListener("change", () => {
    currentProgram = parseInt(els.instrument.value, 10);
    sendMidiEvent(0xC0 | liveChannel, currentProgram, 0);
  });

  els.drumsCh.addEventListener("change", () => {
    releaseAllPianoNotes();
    allNotesOff();
    liveChannel = els.drumsCh.checked ? 9 : 0;
    els.instrument.disabled = els.drumsCh.checked;
    if (!els.drumsCh.checked) {
      sendMidiEvent(0xC0 | liveChannel, currentProgram, 0);
    }
  });

  els.piano.addEventListener("pointerdown", (e) => {
    const key = e.target.closest(".key");
    if (!key) return;
    e.preventDefault();
    els.piano.setPointerCapture(e.pointerId);
    const note = parseInt(key.dataset.note, 10);
    if (!liveMode) {
      setStatus("Сначала включите Live-режим, чтобы играть на пианино", true);
      return;
    }
    pressedNotes.set(e.pointerId, { note, channel: liveChannel });
    if (sendMidiEvent(0x90 | liveChannel, note, 100)) {
      key.classList.add("active");
    }
  });
  els.piano.addEventListener("pointerup", releasePointer);
  els.piano.addEventListener("pointercancel", releasePointer);
  els.piano.addEventListener("lostpointercapture", releasePointer);

  els.seek.addEventListener("input", () => {
    seekTo(parseInt(els.seek.value, 10) || 0);
  });

  els.volume.addEventListener("input", () => {
    const v = parseFloat(els.volume.value);
    els.volLabel.textContent = Math.round(v * 100) + "%";
    if (gainNode) gainNode.gain.value = v;
  });

  // Toggling "full pre-generation" invalidates the generated buffer. When the
  // WASM source was consumed by an offline render, bring it back for the
  // real-time path.
  els.pregen.addEventListener("change", () => {
    pregenAudio = null;
    pregenMs = null;
    pregenPos = 0;
    els.pregenResult.textContent = "Офлайн-рендер: —";
    els.pregenResult.classList.remove("error");
    if (!currentSource && midiBytes && Module) {
      try {
        const { src } = createSource(midiBytes);
        currentSource = src;
      } catch (err) {
        setStatus(err.message || "Ошибка", true);
      }
    }
    playedSamples = 0;
    updateProgressUI();
  });

  // ---- Boot --------------------------------------------------------------

  async function boot() {
    buildInstrumentSelect();
    buildPiano();
    initMidiAccess();
    try {
      setStatus("Загрузка синтезатора…");
      Module = await IntraMidiSynth();
      scratchPtr = Module._malloc(2 * AUDIO_CHUNK * 4);
      setStatus("Синтезатор готов. Загрузите MIDI или включите Live-режим.");
    } catch (err) {
      setStatus("Не удалось загрузить WASM-модуль: " + err, true);
    }
  }

  window.MidiSynthApp = {
    boot, loadFromBytes, loadFromUrl, sendMidiEvent,
    startLive, stopLive, allNotesOff,
  };
  boot();
})();