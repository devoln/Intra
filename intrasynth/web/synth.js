// Intra MIDI Synth — browser player driving the Emscripten-compiled
// MusicSynthesizer WASM module.
//
// The C side exposes:
//   _SourceCreateFromMidiFileData(dataPtr, len, sampleRate, numChannels) -> source*
//   _SourceCreateLive(sampleRate, numChannels) -> source* (бесконечный поток тишины)
//   _SourceFree(source*)
//   _SourceSamplesLeft(source*) -> uint (samples per channel remaining)
//   _SourceGetUninterleavedSamples(source*, dstPtr, count, bufferSizeInSamples) -> uint
//   _SourceSendMidiEvent(source*, status, data0, data1) -> void  //   _GetMidiInfoString(dataPtr, len) -> char* (malloc'd, must be freed)

//
// sendMidiEvent() is the single live-input entry point: it forwards one raw
// MIDI message (status + up to two data bytes, as in Web MIDI's data arrays)
// into the current stream (a loaded song or the live source). It replaces the
// old custom APIs (e.g. SourceSetProgram) and is what the Web MIDI keyboard
// and the on-screen piano both use.
//
// Realtime playback is synthesized by a second instance of the same WASM binary
// inside AudioWorkletProcessor, on the Web Audio rendering thread. The main-thread
// Emscripten Module is kept for MIDI metadata and full offline rendering only.
// When "full pre-generation" is enabled, playback is handed to a native
// AudioBufferSourceNode and the realtime worklet is disconnected entirely.

(() => {
  "use strict";

  const AUDIO_CHUNK = 8192; // max samples pulled per fast-forward step
  const MAX_PREGEN_BYTES = 512 * 1024 * 1024; // cap for the offline buffer

  // Yield that is NOT throttled by browser timer throttling. In iframe /
  // headless contexts setTimeout can be clamped to ~1 Hz, which made full
  // generation look frozen (percent barely moving). MessageChannel posts are
  // delivered at the normal task rate even when timers are throttled.
  // Waiters are queued (not a single onmessage slot) so that concurrent
  // awaiters — e.g. the generation loop and loadFromBytes waiting for it to
  // stop — each get their own wake-up instead of stealing each other's.
  const yieldResolvers = [];
  let yieldChannel = null;
  function yieldToUI() {
    if (!yieldChannel) {
      yieldChannel = new MessageChannel();
      yieldChannel.port1.onmessage = () => {
        const resolve = yieldResolvers.shift();
        if (resolve) resolve();
      };
      yieldChannel.port1.start();
    }
    return new Promise((resolve) => {
      yieldResolvers.push(resolve);
      yieldChannel.port2.postMessage(null);
    });
  }

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
    instrument: document.getElementById("instrument"),
    drumsCh: document.getElementById("drumsCh"),
    reverb: document.getElementById("reverb"),
    reverbLabel: document.getElementById("reverbLabel"),
    midiStatus: document.getElementById("midiStatus"),
    piano: document.getElementById("piano"),
    octDown: document.getElementById("octDown"),
    octUp: document.getElementById("octUp"),
    octLabel: document.getElementById("octLabel"),
    noteTestNotes: document.getElementById("noteTestNotes"),
    tracksPanel: document.getElementById("tracksPanel"),
    tracksHint: document.getElementById("tracksHint"),
    tracks: document.getElementById("tracks"),
  };

  // MIDI-клавиатура — независимый источник, работающий параллельно с песней.
  let keyboardSource = 0;
  let keyboardEnabled = false; // external Web MIDI input only
  let pianoSourceReady = false; // on-screen piano/test-note source only
  let liveChannel = 0; // 9 (канал ударных), когда включены ударные
  let currentProgram = 0; // выбранный GM-инструмент (Program Change)
  let midiAccess = null;
  const pressedNotes = new Map(); // pointerId -> { note, channel } на экранном пианино

  let Module = null;
  let audioCtx = null;
  let realtimeNode = null;
  let realtimeConnected = false;
  let realtimeInit = null;
  let realtimeSongLoaded = false;
  let realtimeMeterPending = false;
  let realtimeLevels = null;
  let realtimeStartSample = 0;
  let realtimeStartedAt = 0;
  let gainNode = null;
  let scratchPtr = 0;
  let paramsPtr = 0;
  let metersRaf = 0;
  // Realtime MIDI feedback and 16-channel note levels are owned by the
  // AudioWorklet and arrive as small control snapshots; no main-thread WASM
  // buffers are needed for meters anymore.
  const renderParams = { ReverbWet: 1 };
  let renderParamsGeneration = 0;
  // A/B: ?wasm=ref loads the last-commit baseline wasm (IntraSynth.ref.wasm).
  // Parsed synchronously so the toggle highlights the active build; switchWasmBuild()
  // re-instantiates and swaps it at runtime WITHOUT a page reload.
  let abBuild = new URLSearchParams(location.search).get("wasm") === "ref";
  let swapping = false; // true while the A/B WASM build is being hot-swapped

  // Дорожки загруженного MIDI-файла: индексы, имена, каналы, программы.
  // Заполняется после успешной загрузки файла (loadFromBytesInner).
  let midiTracks = [];
  // Переопределения инструментов каналов (channel -> GM-программа). Живут в JS,
  // потому что источник пересоздаётся при перемотке/стопе/A-B и переопределения
  // в C++ состоянии теряются. Применяются к каждому новому источнику.
  let trackOverrides = {};
  // Effective GM program per channel, kept in sync with file ProgramChange and UI overrides.
  let trackProgram = {};
  // Громкость дорожек — честный MIDI CC7 (0..127): UI шлёт сообщение в
  // синтезатор, а CC7/ProgramChange ИЗ ФАЙЛА приходят обратно кольцом фидбека
  // и отражаются в контролах. Мьют — чекбокс, и он НЕ трогает громкость: это
  // отдельный слой микшера (битовая маска каналов, SourceSetChannelMute).
  // Иначе файл, который сам присылает CC7 (tous les garçons — CC7 по 7 каналам
  // в начале), отменял бы мьют. Во время офлайн-рендера (полная генерация или
  // проигрывание готового буфера) фичи индикаторов не работают вовсе: события
  // фидбека осушаются и выбрасываются, чтобы кольцо не копило мусор.
  let trackCC7 = {};
  let muted = {};
  // Состояние индикаторов нот: ch -> { note, vel, target, level, live }.
  // Яркость — громкость ИГРАЮЩЕЙ ноты с учётом её огибающей: уровень огибающей
  // из синтезатора умножается на velocity ноты и CC7 дорожки. Опрос редкий
  // (LEVEL_POLL_MS), а между опросами яркость ИНТЕРПОЛИРУЕТСЯ в rAF: без этого
  // она менялась скачками (шаг опроса) и «не гасла» после события отпускания.
  // Живого потока нет (пауза, стоп, офлайн-рендер) — цель 0, индикатор гаснет.
  const noteGlow = {};
  const GLOW_TAU_MS = 55;    // постоянная сглаживания яркости (мс)
  const LEVEL_POLL_MS = 100; // период опроса уровней огибающих
  const NOTE_FALLBACK_MS = 600; // fixed-length fallback without a live meter
  const SILENT_LUM = "0.06"; // фон прямоугольника, когда нота не звучит
  let levelsDirty = true;    // опросить уровни вне очереди (событие отпускания)
  let lastLevelsAt = 0;
  let lastFrameAt = 0;       // время прошлого кадра (для интерполяции по dt)

  /// Relative audible note amplitude used by the UI glow. The synth now applies
  // the common SF2-like v^2 velocity law to every melodic voice and drum after
  // construction; keep the display on the same law. CC7 is amplitude-squared.
  function noteCcFactor(ch, noteNum, velocity) {
    if (muted[ch]) return 0;
    const v = Math.max(0, Math.min(1, velocity / 127));
    const cc = Math.max(0, (trackCC7[ch] ?? 100) / 100);
    // Titanic Standard Kit key 36 has a 1440 cB velocity attenuation override,
    // equivalent here to one extra velocity factor on top of the common v^2.
    const velAmp = ch === 9 && noteNum === 36 ? v * v * v : v * v;
    return Math.min(1, velAmp * cc * cc);
  }
  function noteGlowOn(ch, noteNum, velocity) {
    const g = noteGlow[ch] || (noteGlow[ch] = { note: -1, vel: 0, target: 0, level: 0, live: false, fallbackUntil: 0 });
    g.note = noteNum;
    g.vel = velocity;
    // Сразу ставим ЦЕЛЬ по ноте (вспышка на новом NoteOn), а показываемый
    // уровень подтягивается к ней интерполяцией; следующий опрос заменит цель
    // настоящим значением огибающей.
    g.target = noteCcFactor(ch, noteNum, velocity);
    g.live = true;
    g.fallbackUntil = performance.now() + NOTE_FALLBACK_MS;
    levelsDirty = true;
  }
  // Каналы, попавшие в панель дорожек, и их DOM-прямоугольники нот
  // (заполняются в renderTracks, читаются в pollTrackMeters).
  const channelsOfTracks = [];
  const noteBoxes = [];

  let midiStatLines = [];
  function renderSynthInfo() {
    const parts = ['<span>Sample rate: ' + escapeHtml(audioCtx ? audioCtx.sampleRate + " Hz" : "—") + '</span>'];
    for (const l of midiStatLines) parts.push("<span>" + escapeHtml(l) + "</span>");
    els.info.innerHTML = parts.join("");
  }

  let currentSource = 0; // WASM pointer, 0 = none
  let midiBytes = null; // last loaded MIDI bytes (for seek/replay)
  let midiName = "MIDI";
  let totalSamples = 0;
  let playedSamples = 0;
  let paused = true;
  let seeking = false;

  // Fully rendered song (used when the "full pre-generation" box is ticked).
  // Populated once per loaded file; playback is delegated to a native
  // AudioBufferSourceNode and therefore does not need JS audio callbacks.
  let pregenAudio = null;   // { buffer: AudioBuffer, len }
  let pregenMs = null;      // last full-render duration in ms
  let pregenPos = 0;        // read position inside pregenAudio
  let pregenNode = null;    // native AudioBufferSourceNode during playback
  let pregenStartSample = 0;
  let pregenStartedAt = 0;
  let playbackRaf = 0;
  let generationActive = false; // a full-generation pass is in flight
  let generationAbort = false;  // set by Stop while generating (safe abort)
  let loadingFile = false;      // a file read/load is in flight (async FileReader)

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
    if (s == null) return ""; // необязательные поля манифеста могут отсутствовать
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

  function connectProcessor() {
    if (!realtimeNode || !gainNode || realtimeConnected) return;
    realtimeNode.connect(gainNode);
    realtimeConnected = true;
  }

  function disconnectProcessor() {
    if (!realtimeNode || !realtimeConnected) return;
    try { realtimeNode.disconnect(); } catch (_e) { /* already disconnected */ }
    realtimeConnected = false;
  }

  function realtimePost(message, transfer) {
    if (!realtimeNode) return false;
    if (transfer) realtimeNode.port.postMessage(message, transfer);
    else realtimeNode.port.postMessage(message);
    return true;
  }

  function syncRealtimePosition() {
    if (!audioCtx || paused || pregenAudio || !realtimeSongLoaded) return;
    const elapsed = Math.max(0, audioCtx.currentTime - realtimeStartedAt);
    playedSamples = Math.min(
      totalSamples,
      realtimeStartSample + Math.floor(elapsed * audioCtx.sampleRate)
    );
  }

  function setRealtimePlaying(value) {
    if (value) {
      realtimeStartSample = playedSamples;
      realtimeStartedAt = audioCtx ? audioCtx.currentTime : 0;
    } else {
      syncRealtimePosition();
    }
    realtimePost({ type: "play", value: !!value });
  }

  function realtimeTrackState() {
    return {
      overrides: { ...trackOverrides },
      cc7: { ...trackCC7 },
      muteMask: channelMuteMask(),
      reverbWet: renderParams.ReverbWet,
    };
  }

  function loadRealtimeSong(position = 0, playing = false) {
    if (!realtimeNode || !midiBytes) return false;
    const copy = midiBytes.slice();
    const state = realtimeTrackState();
    realtimeSongLoaded = false;
    realtimePost({
      type: "loadSong",
      bytes: copy.buffer,
      position: position >>> 0,
      playing: !!playing,
      overrides: state.overrides,
      cc7: state.cc7,
      muteMask: state.muteMask,
      reverbWet: state.reverbWet,
    }, [copy.buffer]);
    realtimeStartSample = position >>> 0;
    realtimeStartedAt = audioCtx ? audioCtx.currentTime : 0;
    return true;
  }

  function onRealtimeMessage(event) {
    const message = event.data || {};
    if (message.type === "songLoaded") {
      realtimeSongLoaded = true;
      return;
    }
    if (message.type === "ended") {
      if (!pregenAudio && !paused) stopPlayback();
      return;
    }
    if (message.type === "meters") {
      realtimeMeterPending = false;
      if (message.feedback && message.feedback.length) {
        applyMidiFeedback(message.feedback, Math.floor(message.feedback.length / 3));
      }
      if (message.levels) realtimeLevels = message.levels;
      return;
    }
    if (message.type === "error") {
      setStatus("AudioWorklet: " + (message.message || "ошибка"), true);
    }
  }

  let workletModuleLoaded = false;
  const realtimeWasmCache = new Map();
  async function getRealtimeWasm() {
    const key = abBuild ? "ref" : "cur";
    if (realtimeWasmCache.has(key)) return realtimeWasmCache.get(key);
    const url = abBuild ? "IntraSynth.ref.wasm" : "IntraSynth.wasm";
    const response = await fetch(url);
    if (!response.ok) throw new Error("Не удалось загрузить " + url + ": HTTP " + response.status);
    const bytes = await response.arrayBuffer();
    const module = await WebAssembly.compile(bytes);
    const result = { module, bytes };
    realtimeWasmCache.set(key, result);
    return result;
  }

  async function createRealtimeNode() {
    if (!audioCtx) throw new Error("AudioContext не создан");
    if (!audioCtx.audioWorklet || typeof AudioWorkletNode !== "function") {
      throw new Error("AudioWorklet не поддерживается этим браузером");
    }
    if (!workletModuleLoaded) {
      await audioCtx.audioWorklet.addModule("synth-worklet.js");
      workletModuleLoaded = true;
    }
    const wasm = await getRealtimeWasm();
    let node;
    try {
      node = new AudioWorkletNode(audioCtx, "intra-midi-synth", {
        numberOfInputs: 0,
        numberOfOutputs: 1,
        outputChannelCount: [2],
        processorOptions: { wasmModule: wasm.module },
      });
    } catch (_cloneError) {
      // WebAssembly.Module is structured-cloneable in modern browsers. Keep a
      // bytes fallback for older implementations without changing the audio ABI.
      const copy = wasm.bytes.slice(0);
      node = new AudioWorkletNode(audioCtx, "intra-midi-synth", {
        numberOfInputs: 0,
        numberOfOutputs: 1,
        outputChannelCount: [2],
        processorOptions: { wasmBytes: copy },
      });
    }
    const ready = new Promise((resolve, reject) => {
      const timer = setTimeout(() => reject(new Error("AudioWorklet init timeout")), 5000);
      node.port.onmessage = (event) => {
        if (event.data && event.data.type === "ready") {
          clearTimeout(timer);
          resolve();
          return;
        }
        if (event.data && event.data.type === "error") {
          clearTimeout(timer);
          reject(new Error(event.data.message || "AudioWorklet init error"));
          return;
        }
        onRealtimeMessage(event);
      };
    });
    await ready;
    node.port.onmessage = onRealtimeMessage;
    realtimeNode = node;
    realtimeConnected = false;
    connectProcessor();
    realtimePost({ type: "params", reverbWet: renderParams.ReverbWet });
    return node;
  }

  async function ensureRealtimeNode() {
    if (realtimeNode) return realtimeNode;
    if (!realtimeInit) {
      realtimeInit = createRealtimeNode().catch((error) => {
        realtimeInit = null;
        throw error;
      });
    }
    return realtimeInit;
  }

  async function replaceRealtimeNode(position, wasPlaying) {
    const old = realtimeNode;
    if (old) {
      try { old.port.postMessage({ type: "dispose" }); } catch (_e) {}
      try { old.disconnect(); } catch (_e) {}
    }
    realtimeNode = null;
    realtimeConnected = false;
    realtimeInit = null;
    realtimeSongLoaded = false;
    await ensureRealtimeNode();
    if (midiBytes) loadRealtimeSong(position, wasPlaying);
  }

  function syncPregenPosition() {
    if (!pregenAudio || !pregenNode || paused || !audioCtx) return;
    const elapsed = Math.max(0, audioCtx.currentTime - pregenStartedAt);
    const advanced = Math.floor(elapsed * audioCtx.sampleRate);
    pregenPos = Math.min(pregenAudio.len, pregenStartSample + advanced);
    playedSamples = pregenPos;
  }

  function stopPregenNode() {
    if (!pregenNode) return;
    const node = pregenNode;
    pregenNode = null;
    node.onended = null;
    try { node.stop(); } catch (_e) { /* already ended */ }
    try { node.disconnect(); } catch (_e) { /* already disconnected */ }
  }

  function playbackUiTick() {
    playbackRaf = 0;
    if (pregenNode && !paused) syncPregenPosition();
    else if (!paused && !pregenAudio) syncRealtimePosition();
    updateProgressUI();
    if (!paused && (currentSource || pregenNode || realtimeSongLoaded)) {
      playbackRaf = requestAnimationFrame(playbackUiTick);
    }
  }

  function startPlaybackUiLoop() {
    if (!playbackRaf) playbackRaf = requestAnimationFrame(playbackUiTick);
  }

  function stopPlaybackUiLoop() {
    if (!playbackRaf) return;
    cancelAnimationFrame(playbackRaf);
    playbackRaf = 0;
  }

  function startPregenNode() {
    if (!pregenAudio || !audioCtx || paused) return;
    stopPregenNode();
    disconnectProcessor();
    const node = audioCtx.createBufferSource();
    node.buffer = pregenAudio.buffer;
    node.connect(gainNode);
    pregenNode = node;
    pregenStartSample = Math.min(pregenPos, pregenAudio.len);
    if (pregenStartSample >= pregenAudio.len) {
      pregenNode = null;
      node.disconnect();
      stopPlayback();
      return;
    }
    pregenStartedAt = audioCtx.currentTime;
    const offsetSec = pregenStartSample / audioCtx.sampleRate;
    const durationSec = (pregenAudio.len - pregenStartSample) / audioCtx.sampleRate;
    node.onended = () => {
      if (pregenNode !== node) return;
      pregenNode = null;
      pregenPos = pregenAudio ? pregenAudio.len : 0;
      playedSamples = pregenPos;
      stopPlayback();
    };
    node.start(0, offsetSec, durationSec);
    startPlaybackUiLoop();
  }

  function ensureAudio() {
    if (!audioCtx) {
      audioCtx = new (window.AudioContext || window.webkitAudioContext)({ latencyHint: "interactive" });
      renderSynthInfo();
      setStatus("Web Audio готов: " + audioCtx.sampleRate + " Hz");
      gainNode = audioCtx.createGain();
      gainNode.gain.value = parseFloat(els.volume.value);
      gainNode.connect(audioCtx.destination);
    }
    if (audioCtx.state === "suspended") audioCtx.resume().catch(() => {});
    return audioCtx;
  }

  function freeSource() {
    if (currentSource && Module) Module._SourceFree(currentSource);
    currentSource = 0;
  }

  function freeKeyboardSource() {
    keyboardSource = 0;
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
    // Переопределения инструментов и гейн/мьют дорожек живут в JS (C++-состояние
    // источника теряется при каждом пересоздании: перемотка, стоп, A/B-переключение,
    // полная генерация). Единая точка входа покрывает все эти пути.
    applyTrackOverrides(srcPtr);
    applyTrackMix(srcPtr);
    return { src: srcPtr, info: infoText };
  }

  async function loadFromBytes(bytes, name) {
    loadingFile = true;
    try {
      // If a full-generation pass is rendering the current source, never free
      // it out from under the loop (use-after-free made the WASM render loop
      // spin forever: the page froze, Stop did nothing). Ask it to stop and
      // wait until it has fully unwound before touching any source.
      if (generationActive) {
        generationAbort = true;
        setStatus("Ожидание остановки генерации…");
        while (generationActive) await yieldToUI();
      }
      await loadFromBytesInner(bytes, name);
    } finally {
      loadingFile = false;
    }
  }

  async function loadFromBytesInner(bytes, name) {
    releaseAllPianoNotes();
    stopPregenNode();
    connectProcessor();
    stopPlaybackUiLoop();
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

    // Новый файл — чистый лист: переопределения инструментов и громкость/мьют
    // сбрасываются ДО createSource (он применяет текущие настройки к источнику).
    trackOverrides = {};
    trackProgram = {};
    trackCC7 = {};
    muted = {};
    try {
      ensureAudio();
      await ensureRealtimeNode();
      const { src, info } = createSource(bytes);
      currentSource = src;
      if (!paramsPtr) paramsPtr = Module._malloc(4);
      totalSamples = Module._SourceSamplesLeft(src);
      applyRenderParams(currentSource);
      loadRealtimeSong(0, false);
      // Панель дорожек: парсим файл в JS, сбрасываем переопределения
      // инструментов (новый файл — чистый лист).
      try {
        midiTracks = parseMidiTracks(bytes);
      } catch (_e) {
        midiTracks = [];
      }
      renderTracks();
      midiStatLines = (info || "").split("\n").filter(Boolean);
      renderSynthInfo();
      els.player.classList.remove("hidden");
      setEnabled("playBtn", true);
      setEnabled("stopBtn", true);
      setEnabled("seek", true);
      updatePregenControls();
      setStatus("Готов к воспроизведению");
      updateProgressUI();
    } catch (err) {
      midiBytes = null;
      midiTracks = [];
      trackOverrides = {};
      trackProgram = {};
      els.tracksPanel.hidden = true;
      setStatus(err.message || "Ошибка загрузки", true);
      midiStatLines = [];
      renderSynthInfo();
    }
  }

  async function loadFromUrl(url) {
    loadingFile = true;
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
        loadingFile = false;
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
      ? '<svg viewBox="0 0 24 24"><path d="M8 5v14l11-7z"/></svg>'
      : '<svg viewBox="0 0 24 24"><path d="M6 5h4v14H6zM14 5h4v14h-4z"/></svg>';
  }

  // Во время оффлайн-генерации и при воспроизведении из pregen-буфера все
  // контролы панели дорожек неактивны: менять нельзя (рендер уже посчитан
  // или считается).
  function updatePregenControls() {
    const locked = generationActive || (!!pregenAudio && !paused);
    document.querySelectorAll(".track-vol, .track-mute, .track-prog").forEach((el) => {
      el.disabled = locked;
    });
  }
  // Держим блокировку актуальной: смена paused/pregen может произойти мимо
  // явных вызовов (конец файла, переменотка).
  const _updateProgressUICB = updateProgressUI;
  updateProgressUI = function () { _updateProgressUICB(); updatePregenControls(); };

  // Fully renders the current WASM source into an AudioBuffer, measuring the
  // wall-clock time. Yields periodically so the status bar keeps its progress
  // percentage visible.
  async function generateAll() {
    if (!currentSource || !Module || !totalSamples) return null;
    const bytes = totalSamples * 2 * 4;
    if (bytes > MAX_PREGEN_BYTES) {
      setStatus("Файл слишком длинный для полной генерации", true);
      return null;
    }
    // Если источник уже частично потреблён реальным временем (играли до этого),
    // пересоздаём его, чтобы буфер начинался с 0 и процент совпадал со всей песней.
    if (midiBytes && Module._SourceSamplesLeft(currentSource) < totalSamples) {
      try {
        const { src } = createSource(midiBytes);
        freeSource();
        currentSource = src;
        applyRenderParams(currentSource);
        applyTrackOverrides(currentSource);
      } catch (err) {
        setStatus(err.message || "Ошибка при пересоздании источника", true);
        return null;
      }
    }
    let buffer, left, right;
    try {
      buffer = audioCtx.createBuffer(2, totalSamples, audioCtx.sampleRate);
      left = buffer.getChannelData(0);
      right = buffer.getChannelData(1);
    } catch (err) {
      setStatus(
        "Не хватило памяти для полной генерации (~" + Math.round(bytes / 1048576) + " МБ)",
        true
      );
      return null;
    }
    // Capture the source locally: from here on we render THIS source. Nobody
    // may free it while the loop runs (loadFromBytes / seekTo / stopPlayback
    // are all excluded while generationActive is set), so the loop can never
    // dereference a freed WASM pointer (that used to hang the whole page).
    const src = currentSource;
    const off = scratchPtr >> 2;
    const t0 = performance.now();
    let pos = 0;
    let reportCounter = 0;
    while (pos < totalSamples) {
      // Stop во время генерации — безопасная остановка: выходим на следующем
      // чанке, не трогая источник (иначе use-after-free повесил бы WASM-цикл).
      if (generationAbort) {
        setStatus("Генерация отменена");
        return null;
      }
      // Keep the browser responsive on long files and avoid monopolising the
      // main thread while WASM renders a large chunk.
      const n = Math.min(AUDIO_CHUNK, totalSamples - pos);
      const written = Module._SourceGetUninterleavedSamples(
        src, scratchPtr, n, AUDIO_CHUNK
      );
      if (written > 0) {
        // WASM-куча может вырасти в середине рендера (аллокации голосов/
        // партиалов на нотах): Emscripten тогда заменяет Module.HEAPF32
        // свежим видом на новый буфер, а старый вид остаётся отцепленным
        // и его subarray() кидает "Cannot perform Construct on a detached
        // ArrayBuffer". Перечитываем вид на каждом чанке, а не один раз
        // до цикла.
        const heap = Module.HEAPF32;
        left.set(heap.subarray(off, off + written), pos);
        right.set(heap.subarray(off + AUDIO_CHUNK, off + AUDIO_CHUNK + written), pos);
      }
      pos += written;
      if (written === 0) {
        // Источник перестал отдавать семплы раньше конца: сообщаем, где
        // остановились, вместо бесконечного ожидания.
        if (pos < totalSamples) {
          setStatus(
            "Источник замолчал на " + Math.round(pos * 100 / totalSamples) + "%",
            true
          );
        }
        break;
      }
      if (++reportCounter % 4 === 0) {
        setStatus("Полная генерация… " + Math.round(pos * 100 / totalSamples) + "%");
        await yieldToUI();
      }
      if (pos >= totalSamples || written < n) break;
    }
    const ms = performance.now() - t0;
    return { buffer, len: pos, ms };
  }

  async function playPause() {
    if (loadingFile || (!currentSource && !pregenAudio)) return;
    ensureAudio();
    try { await ensureRealtimeNode(); }
    catch (err) { setStatus("AudioWorklet: " + err, true); return; }

    // Offline render first (with timing) when the checkbox is on and there is
    // no generated buffer yet. If generation fails, stay paused.
    if (els.pregen.checked && !pregenAudio && currentSource) {
      if (generationActive) return; // already generating — ignore double clicks
      generationActive = true;
      generationAbort = false;
      if (!paused) setRealtimePlaying(false);
      paused = true;
      els.playBtn.disabled = true;
      updatePregenControls();
      setPlayIcon(true);
      setStatus("Полная генерация…");
      let result;
      try {
        result = await generateAll();
      } catch (err) {
        setStatus("Полная генерация: " + err, true);
        result = null;
      } finally {
        // Кнопка «Играть» ВСЕГДА возвращается в рабочее состояние, даже если
        // генерация упала или была остановлена (раньше она оставалась
        // заблокированной навсегда).
        generationActive = false;
        els.playBtn.disabled = false;
        updatePregenControls();
      }
      if (!result || !result.len) {
        if (totalSamples) {
          els.pregenResult.textContent = "Офлайн-рендер: ошибка";
          els.pregenResult.classList.add("error");
          if (result) setStatus("Полная генерация не дала звука", true);
        }
        // После отмены генерации источник может быть частично потреблён —
        // вернём его в начало, чтобы следующее «Играть» работало с нуля.
        if (generationAbort && midiBytes) {
          try {
            const { src } = createSource(midiBytes);
            freeSource();
            currentSource = src;
            applyRenderParams(currentSource);
            applyTrackOverrides(currentSource);
          } catch (err) {
            setStatus(err.message || "Ошибка", true);
          }
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
      // The main-thread source was consumed by offline rendering. Recreate a
      // fresh idle copy so changing parameters can invalidate pregen without
      // forcing a WASM reload; realtime audio remains owned by AudioWorklet.
      if (midiBytes) {
        const { src } = createSource(midiBytes);
        freeSource();
        currentSource = src;
        applyRenderParams(currentSource);
      }
    }

    if (pregenAudio) {
      if (paused) {
        paused = false;
        startPregenNode();
      } else {
        syncPregenPosition();
        paused = true;
        stopPregenNode();
        connectProcessor();
        stopPlaybackUiLoop();
      }
    } else {
      if (paused) {
        paused = false;
        connectProcessor();
        setRealtimePlaying(true);
        startPlaybackUiLoop();
      } else {
        setRealtimePlaying(false);
        paused = true;
        stopPlaybackUiLoop();
      }
    }
    setPlayIcon(paused);
    setStatus(paused ? "Пауза" : "Воспроизведение…");
    metersRestart();
    updatePregenControls();
  }

  function stopPlayback() {
    // Генерация идёт: не освобождаем источник, который она использует (это
    // был бы use-after-free, вешающий WASM-цикл навсегда). Просим генерацию
    // остановиться на следующем чанке; playPause сам вернёт источник в начало.
    if (generationActive) {
      generationAbort = true;
      paused = true;
      setPlayIcon(true);
      els.seek.value = "0";
      els.seek.style.setProperty("--pct", "0%");
      els.time.textContent = "0:00 / " + fmtTime(totalSamples / (audioCtx ? audioCtx.sampleRate : 44100));
      updateProgressUI();
      setStatus("Остановка генерации…");
      return;
    }
    if (!currentSource && !pregenAudio) return;
    stopPregenNode();
    connectProcessor();
    stopPlaybackUiLoop();
    if (!paused && !pregenAudio) setRealtimePlaying(false);
    paused = true;
    seeking = false;
    playedSamples = 0;
    pregenPos = 0;
    realtimeStartSample = 0;
    setPlayIcon(true);

    if (pregenAudio) {
      // Fast restart from the in-memory render — no need to re-synthesize.
      pregenPos = 0;
      playedSamples = 0;
    } else if (midiBytes) {
      realtimePost({ type: "stop" });
    }
    els.seek.value = "0";
    els.seek.style.setProperty("--pct", "0%");
    els.time.textContent = "0:00 / " + fmtTime(totalSamples / (audioCtx ? audioCtx.sampleRate : 44100));
    updateProgressUI();
    setStatus("Остановлено. Нажмите «Играть», чтобы воспроизвести с начала.");
    metersRestart();
    updatePregenControls();
  }

  async function seekTo(sample) {
    const target = Math.max(0, Math.min(sample, totalSamples));

    // Pregen: перемотка — тривиальная подмена позиции в готовом буфере,
    // работает всегда (и во время генерации следующего… pregen единственный).
    if (pregenAudio) {
      const resume = !paused;
      stopPregenNode();
      pregenPos = target;
      playedSamples = target;
      if (resume) startPregenNode();
      updateProgressUI();
      return;
    }

    // Realtime file synthesis lives in AudioWorklet. Seeking therefore never
    // touches the main-thread source (kept pristine for possible offline render).
    if (!midiBytes || generationActive || seeking || !realtimeNode) return;
    seeking = true;
    realtimePost({ type: "seek", position: target >>> 0 });
    playedSamples = target;
    realtimeStartSample = target;
    realtimeStartedAt = audioCtx ? audioCtx.currentTime : 0;
    seeking = false;
    updateProgressUI();
    setStatus(paused ? "Готов к воспроизведению" : "Воспроизведение…");
    metersRestart();
  }

  // Sends one raw MIDI message into the current stream. status is the status
  // byte with the channel, e.g. 0x90 = Note On on channel 0; data0/data1 are
  // the data bytes (NoteOn: note, velocity; ProgramChange: program; CC123:
  // All Notes Off, etc.). The C side applies the event at the current stream
  // position, so it sounds immediately. Returns false when no source is loaded.
  function sendMidiEvent(status, data0, data1) {
    if (!realtimeNode || !keyboardSource) return false;
    realtimePost({
      type: "keyboardMidi",
      status: status & 0xFF, data0: data0 & 0xFF, data1: data1 & 0xFF,
    });
    return true;
  }

  function ensureKeyboardSource() {
    ensureAudio();
    if (!realtimeNode) return false;
    keyboardSource = 1; // readiness flag; the actual source pointer lives in worklet
    connectProcessor();
    return true;
  }

  function allNotesOff() {
    if (!keyboardSource) return;
    for (let ch = 0; ch < 16; ch++) sendMidiEvent(0xB0 | ch, 0x7B, 0);
  }

  // Starts the infinite live source (pure playing, no MIDI file) and switches
  // the audio output to it.
  // Starts the infinite live source (pure playing, no MIDI file). НЕ удаляет
  // загруженную песню: midiBytes сохраняется, чтобы stopLive() мог вернуть
  // её на место (раньше песня терялась безвозвратно).
  function enableExternalMidi() {
    if (!ensureKeyboardSource()) return false;
    keyboardEnabled = true;
    releaseAllPianoNotes();
    els.liveBtn.classList.add("btn-active");
    els.liveBtn.setAttribute("aria-pressed", "true");
    els.liveBtn.title = "Выключить MIDI-клавиатуру";
    els.liveBtn.innerHTML = '<svg viewBox="0 0 24 24"><path d="M3 10v4h4l5 4V6L7 10H3z"/><path d="M16 8.5a5 5 0 0 1 0 7M18.5 5.5a9 9 0 0 1 0 13"/></svg>';
    setStatus("MIDI-клавиатура включена; она играет независимо от MIDI-файла");
    if (!els.drumsCh.checked) sendMidiEvent(0xC0 | liveChannel, currentProgram, 0);
    return true;
  }

  const noteTestTimers = new Map();

  function auditionTestNote(note) {
    if (!Module) return;
    if (!pianoSourceReady && !ensureKeyboardSource()) return;
    pianoSourceReady = true;
    // Тест-нота играет текущий выбранный инструмент (Program Change) на
    // мелодическом канале — независимо от переключателя ударных. При
    // переключении вкладки семплов инструмент переключается на тот же, так
    // что тест-нота звучит как A/B к сырому семплу рядом.
    sendMidiEvent(0xC0, currentProgram, 0);
    sendMidiEvent(0x90, note, 100);
    const oldTimer = noteTestTimers.get(note);
    if (oldTimer) clearTimeout(oldTimer);
    // Hold the note as long as the sample next to it — an honest a/b.
    const holdMs = (window.__intraDebug && window.__intraDebug.holdMsFor(note)) || 1400;
    const timer = setTimeout(() => {
      if (keyboardSource) sendMidiEvent(0x80, note, 0);
      noteTestTimers.delete(note);
    }, holdMs);
    noteTestTimers.set(note, timer);
  }

  function stopKeyboard() {
    keyboardEnabled = false;
    if (!keyboardSource) return;
    allNotesOff();
    releaseAllPianoNotes();
    allNotesOff();
    pianoSourceReady = false;
    els.liveBtn.classList.remove("btn-active");
    els.liveBtn.setAttribute("aria-pressed", "false");
    els.liveBtn.title = "Включить MIDI-клавиатуру";
    els.liveBtn.innerHTML = '<svg viewBox="0 0 24 24"><path d="M3 10v4h4l5 4V6L7 10H3z"/><path d="M16 9l6 6M22 9l-6 6"/></svg>';
    setStatus("MIDI-клавиатура выключена");
  }


  // ---- Экранное пианино --------------------------------------------------

  const PIANO_WHITE = [0, 2, 4, 5, 7, 9, 11]; // смещения белых клавиш в октаве
  // Чёрная клавиша стоит после белой с индексом i в октаве (смещение = полутон).
  const PIANO_BLACK_AFTER = { 0: 1, 1: 3, 3: 6, 4: 8, 5: 10 };
  const NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
  const PIANO_START = 48; // C3 — базовая точка; октава сдвигается стрелками
  const PIANO_OCTAVES = 2; // видимых октав
  const PIANO_OCTAVE_MIN = -1; // самая низкая позиция: C2..B3 (C2 — низший семпл)
  const PIANO_OCTAVE_MAX = 4; // самая высокая: C7..B8 (C7 — высший семпл)
  let pianoOctave = 0; // 0 = C3..B4 (по умолчанию)

  function noteOctave(note) {
    return Math.floor(note / 12) - 1;
  }

  function buildPiano() {
    const piano = els.piano;
    piano.innerHTML = "";
    const whites = [];
    const startNote = PIANO_START + pianoOctave * 12;
    for (let oct = 0; oct < PIANO_OCTAVES; oct++) {
      for (const w of PIANO_WHITE) {
        const note = startNote + oct * 12 + w;
        const el = document.createElement("div");
        el.className = "key white";
        el.dataset.note = note;
        const label = document.createElement("span");
        label.className = "label";
        label.textContent = NOTE_NAMES[w] + noteOctave(note);
        el.appendChild(label);
        piano.appendChild(el);
        whites.push(el);
      }
    }
    const totalWhites = whites.length;
    for (let oct = 0; oct < PIANO_OCTAVES; oct++) {
      for (const [afterWhite, semitone] of Object.entries(PIANO_BLACK_AFTER)) {
        const note = startNote + oct * 12 + semitone;
        const el = document.createElement("div");
        el.className = "key black";
        el.dataset.note = note;
        const boundary = oct * 7 + parseInt(afterWhite, 10) + 1;
        el.style.left = (boundary / totalWhites * 100 - 3.5) + "%";
        piano.appendChild(el);
      }
    }
  }

  function updateOctaveArrows() {
    els.octDown.disabled = pianoOctave <= PIANO_OCTAVE_MIN;
    els.octUp.disabled = pianoOctave >= PIANO_OCTAVE_MAX;
    els.octLabel.textContent = "C" + (pianoOctave + 3) + "–B" + (pianoOctave + 4);
  }

  function shiftPiano(delta) {
    const next = Math.max(PIANO_OCTAVE_MIN, Math.min(PIANO_OCTAVE_MAX, pianoOctave + delta));
    if (next === pianoOctave) return;
    pianoOctave = next;
    releaseAllPianoNotes(); // не оставляем зажатых нот при перестроении клавиш
    buildPiano();
    updateOctaveArrows();
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
    if (!keyboardEnabled) return;
    const data = e.data;
    if (!data || data.length < 1) return;
    const status = data[0];
    if (status < 0x80) return;
    const data0 = data.length > 1 ? data[1] : 0;
    const data1 = data.length > 2 ? data[2] : 0;
    // External MIDI is opt-in; on-screen piano uses the same independent layer.
    if (keyboardEnabled && keyboardSource) sendMidiEvent(status, data0, data1);
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
    ["Флейты", [[72, "Piccolo"], [73, "Flute (macOS DLS)"], [74, "Recorder"], [75, "Pan Flute"], [76, "Blown Bottle"], [77, "Shakuhachi"], [78, "Whistle"], [79, "Ocarina"]]],
    ["Альтернативные флейты", [[43, "Flute (Titanic, чистый профиль)"], [115, "Flute (гибрид: атака DLS + тело Titanic)"]]],
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

  // ---- Дорожки MIDI-файла -----------------------------------------------
  // Лёгкий парсер формата SMF: извлекает по дорожкам имена (FF 03), каналы и
  // программы (Program Change). Этого достаточно для панели дорожек — сам
  // синтез остаётся в WASM (MidiFileParser там).
  // Имя дорожки в SMF — просто байты без объявленной кодировки: старые
  // редакторы писали его в системной кодировке (cp1251 для русских названий:
  // «скрипки» → ñêðèïêè при чтении как Latin-1), новые — в UTF-8.
  // Порядок: строгий UTF-8, иначе выбор между cp1251 и cp1252 по доле латинских
  // букв с диакритикой в cp1252-декодировании. Русский текст, прочитанный как
  // cp1252, почти целиком состоит из них («ñêðèïêè» — 7 из 7), а настоящий
  // западный текст — нет («Café» — 1 из 4, «Mélodie» — 1 из 6). Порог 0.5 и оба
  // исхода проверены в .scratch/encoding-probe.mjs.
  function decodeMidiText(data) {
    if (!data || !data.length) return "";
    try { return new TextDecoder("utf-8", { fatal: true }).decode(data); } catch (e) {}
    let cp1251 = "", cp1252 = "";
    try {
      cp1251 = new TextDecoder("windows-1251").decode(data);
      cp1252 = new TextDecoder("windows-1252").decode(data);
    } catch (e) {
      return String.fromCharCode.apply(null, data); // экзотический браузер
    }
    let cyrillic = 0;
    for (const ch of cp1251) {
      const c = ch.charCodeAt(0);
      if ((c >= 0x410 && c <= 0x44f) || c === 0x401 || c === 0x451) cyrillic++;
    }
    let ascii = 0, accented = 0;
    for (const ch of cp1252) {
      const c = ch.charCodeAt(0);
      if ((c >= 0x41 && c <= 0x5a) || (c >= 0x61 && c <= 0x7a)) ascii++;
      else if (c >= 0xa0 && c <= 0xff) accented++;
    }
    if (cyrillic > 0 && accented * 2 > ascii + accented) return cp1251;
    return cp1252;
  }

  function parseMidiTracks(bytes) {
    const dv = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    const u8 = bytes;
    let p = 0;
    const rdU32 = () => { const v = dv.getUint32(p, false); p += 4; return v; };
    const rdU16 = () => { const v = dv.getUint16(p, false); p += 2; return v; };
    const rdVLQ = () => {
      let v = 0, b;
      do { b = u8[p++]; v = (v << 7) | (b & 0x7f); } while (b & 0x80);
      return v;
    };
    const rdStr = (n) => { const s = String.fromCharCode.apply(null, u8.subarray(p, p + n)); p += n; return s; };

    if (rdStr(4) !== "MThd") throw new Error("не MIDI файл");
    const hdrLen = rdU32();
    // Порядок полей заголовка SMF: format, nTracks, division — именно в этом
    // порядке (формат-1 файл читался как 1 дорожка, пока nTracks не читался
    // из поля format).
    rdU16(); // format
    const nTracks = rdU16();
    rdU16(); // division
    p += hdrLen - 6;

    const tracks = [];
    for (let t = 0; t < nTracks; t++) {
      if (rdStr(4) !== "MTrk") throw new Error("битая дорожка MIDI");
      const len = rdU32();
      const end = p + len;
      const track = {
        index: t, name: "", channels: [],
        programs: {}, // channel -> последняя программа
        volumes: {},  // channel -> последний CC7 (начальная громкость канала)
        noteCount: 0,
      };
      let running = 0;
      while (p < end) {
        rdVLQ();
        let b = u8[p++];
        if (b === 0xff) {
          const type = u8[p++];
          const l = rdVLQ();
          const data = u8.subarray(p, p + l); p += l;
          if (type === 0x03) track.name = decodeMidiText(data);
          continue;
        }
        if (b === 0xf0 || b === 0xf7) { const l = rdVLQ(); p += l; continue; }
        if ((b & 0x80) === 0) { b = running; p--; } else running = b;
        const kind = b & 0xf0;
        const ch = b & 0x0f;
        if (kind === 0xc0) {
          track.programs[ch] = u8[p++];
        } else if (kind === 0xb0) {
          const d0 = u8[p], d1 = u8[p + 1];
          if (d0 === 0x07) track.volumes[ch] = d1; // начальный CC7 канала
          p += 2;
        } else if (kind === 0x90) {
          const vel = u8[p + 1];
          if (vel > 0) track.noteCount++;
          p += 2;
        } else if (kind === 0x80 || kind === 0xb0 || kind === 0xe0 || kind === 0xa0 || kind === 0xd0) {
          p += 2;
        } else {
          throw new Error("неизвестное MIDI-событие " + b.toString(16));
        }
      }
      track.channels = Object.keys(track.programs).map(Number);
      // Канал с CC7, но без Program Change тоже должен попасть в панель:
      // иначе начальная громкость канала нигде не покажется.
      for (const ch of Object.keys(track.volumes)) {
        const chNum = Number(ch);
        if (!track.channels.includes(chNum)) track.channels.push(chNum);
      }
      tracks.push(track);
    }
    return tracks;
  }

  // GM-программа по умолчанию для канала, у которого в файле не было
  // Program Change (в стандарте — фортепиано, кроме канала 9).
  const DEFAULT_PROG = 0;

  function renderTracks() {
    // Одна строка на канал (инструмент в MIDI назначается каналу, а не
    // дорожке; несколько дорожек могут делить канал). Имена дорожек
    // склеиваются для подсказки.
    const byChannel = new Map();
    for (const t of midiTracks) {
      for (const ch of t.channels) {
        if (!byChannel.has(ch)) byChannel.set(ch, { names: [], progs: [], vols: [] });
        const rec = byChannel.get(ch);
        if (t.name && !rec.names.includes(t.name)) rec.names.push(t.name);
        if (t.programs[ch] !== undefined) rec.progs.push(t.programs[ch]);
        if (t.volumes && t.volumes[ch] !== undefined) rec.vols.push(t.volumes[ch]);
      }
    }
    const rows = [];
    channelsOfTracks.length = 0;
    noteBoxes.length = 0;
    for (const k of Object.keys(noteGlow)) delete noteGlow[k];
    for (const [ch, rec] of byChannel) {
      channelsOfTracks.push(ch);
      // Начальный CC7 канала из файла: сидируем один раз, чтобы ползунок
      // показывал громкость, ЗАДАННУЮ ФАЙЛОМ, ещё до первого воспроизведения.
      if (trackCC7[ch] === undefined) {
        const fileVol = rec.vols.length ? rec.vols[rec.vols.length - 1] : 100;
        trackCC7[ch] = fileVol;
      }
      const fileProg = rec.progs.length ? rec.progs[rec.progs.length - 1] : DEFAULT_PROG;
      const isDrums = ch === 9;
      const row = document.createElement("div");
      row.className = "track-row";
      row.dataset.channel = ch;

      const chEl = document.createElement("span");
      chEl.className = "track-ch";
      chEl.textContent = String(ch + 1);
      chEl.title = "канал " + (ch + 1);
      row.appendChild(chEl);

      // Мьют — чекбокс слева: включён = играет, выключен = тише. При мьюте
      // шлём CC7=0; анмьют возвращает CC7, который был до мьюта.
      const mute = document.createElement("input");
      mute.type = "checkbox";
      mute.className = "track-mute";
      mute.checked = !muted[ch];
      mute.title = "Дорожка включена (выключить = мьют)";
      mute.setAttribute("aria-label", "Включить дорожку " + (ch + 1));
      mute.addEventListener("change", () => {
        if (!mute.checked) muted[ch] = true;
        else delete muted[ch];
        volLabel.textContent = volLabelOf(ch);
        row.classList.toggle("track-muted", !!muted[ch]);
        // Мьют — отдельный слой микшера (маска каналов): громкость дорожки
        // (CC7) остаётся как есть, поэтому файл, который сам присылает CC7,
        // мьют не отменяет. Звучащие ноты канала гасит сам синтезатор.
        applyChannelMute();
      });
      row.appendChild(mute);

      // Компактный слайдер CC7 (0..127), значение — мелко под слайдером.
      const volWrap = document.createElement("span");
      volWrap.className = "track-vol-wrap";
      const vol = document.createElement("input");
      vol.type = "range";
      vol.className = "track-vol";
      vol.min = "0";
      vol.max = "127";
      vol.step = "1";
      vol.value = String(trackCC7[ch] ?? 100);
      vol.title = "Громкость дорожки (MIDI CC7)";
      vol.setAttribute("aria-label", "Громкость дорожки " + (ch + 1));
      const volLabel = document.createElement("span");
      volLabel.className = "track-vol-label";
      volLabel.textContent = volLabelOf(ch);
      vol.addEventListener("input", () => {
        trackCC7[ch] = parseInt(vol.value, 10) || 0;
        volLabel.textContent = volLabelOf(ch);
        // Реальное время: CC7 уходит в синтезатор обычным MIDI-сообщением.
        if (Module && currentSource && !generationActive) {
          Module._SourceSendMidiEvent(currentSource, 0xB0 | ch, 0x07, trackCC7[ch] & 127);
        }
        if (!generationActive) {
          realtimePost({ type: "songMidi", status: 0xB0 | ch, data0: 0x07, data1: trackCC7[ch] & 127 });
        }
      });
      volWrap.appendChild(vol);
      volWrap.appendChild(volLabel);
      row.appendChild(volWrap);

      const nameEl = document.createElement("span");
      nameEl.className = "track-name";
      nameEl.title = rec.names.join(" · ") || "Без названия";
      nameEl.textContent = isDrums ? "Ударные" : (rec.names.join(" · ") || "—");
      row.appendChild(nameEl);

      // Индикатор последней ноты: приклеен вплотную слева к комбобоксу.
      const noteBox = document.createElement("div");
      noteBox.className = "track-note";
      noteBox.dataset.ch = ch;
      noteBox.textContent = "—";
      noteBox.title = "Последняя нота дорожки (яркость — громкость)";
      noteBoxes.push([ch, noteBox]);

      row.appendChild(noteBox);
      if (isDrums) {
        const tag = document.createElement("span");
        tag.className = "tracks-hint";
        tag.textContent = "канал ударных";
        row.appendChild(tag);
      } else {
        const sel = document.createElement("select");
        sel.className = "track-prog";
        const overridden = trackOverrides[ch] !== undefined;
        const current = overridden ? trackOverrides[ch] : fileProg;
        trackProgram[ch] = current;
        const orig = document.createElement("option");
        orig.value = "-1";
        orig.textContent = "Исходный из файла (" + progName(fileProg) + ")";
        sel.appendChild(orig);
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
        sel.value = String(current);
        if (overridden) sel.classList.add("btn-active");
        sel.addEventListener("change", () => {
          const v = parseInt(sel.value, 10);
          if (v >= 0) trackOverrides[ch] = v;
          else delete trackOverrides[ch];
          trackProgram[ch] = v >= 0 ? v : fileProg;
          sel.classList.toggle("btn-active", v >= 0);
          // Реальное время: последующие ноты канала играют новой программой.
          // 255 (0xFF) снимает переопределение в C++ (SetChannelProgram).
          // При pregen-проигрывании контрол заблокирован — сюда не попадём.
          if (Module && currentSource && !generationActive) {
            Module._SourceSendMidiEvent(currentSource, 0xC0 | ch, v >= 0 ? v : 0xFF, 0);
          }
          if (!generationActive) {
            realtimePost({ type: "songMidi", status: 0xC0 | ch, data0: v >= 0 ? v : 0xFF, data1: 0 });
          }
          // Офлайн-буфер содержит старый рендер — сбросить, как при ревербе.
          if (pregenAudio) {
            stopPregenNode();
            connectProcessor();
            stopPlaybackUiLoop();
            pregenAudio = null;
            pregenPos = 0;
            playedSamples = 0;
            els.pregenResult.textContent = "Офлайн-рендер: —";
          }
        });
        row.appendChild(sel);
      }
      rows.push(row);
    }
    els.tracks.innerHTML = "";
    rows.forEach((r) => els.tracks.appendChild(r));
    // Панель перерисована: прямоугольники заменены, цикл метров продолжит
    // обновлять новые элементы (он читает noteBoxes каждый кадр).
    metersRestart();
    if (midiTracks.length) {
      const withNotes = midiTracks.filter((t) => t.noteCount > 0).length;
      els.tracksHint.textContent =
        "Дорожек: " + midiTracks.length + ", с нотами: " + withNotes +
        ". Смена инструмента и громкость применяются к каналу в реальном времени.";
    }
    els.tracksPanel.hidden = !rows.length;
  }

  const volLabelOf = (ch) => String(trackCC7[ch] ?? 100);

  // Мьют дорожек = битовая маска каналов в синтезаторе (отдельный слой
  // микшера). Громкость (CC7) она не трогает: «снял галочку — тишина,
  // поставил — вернулась та же громкость».
  function channelMuteMask() {
    let mask = 0;
    for (const ch of Object.keys(muted)) if (muted[ch]) mask |= 1 << Number(ch);
    return mask;
  }
  /// Ставит маску мьюта живому источнику (во время генерации контролы
  /// заблокированы — менять мьют некому).
  function applyChannelMute() {
    const mask = channelMuteMask();
    if (Module && currentSource) Module._SourceSetChannelMute(currentSource, mask);
    realtimePost({ type: "mute", mask });
  }

  function progName(prog) {
    for (const [, items] of GM_GROUPS) {
      for (const [p, name] of items) if (p === prog) return name;
    }
    return "GM " + prog;
  }

  // Возвращает каналы с переопределениями (для повторного применения после
  // пересоздания источника: перемотка, стоп, A/B, полная генерация).
  function applyTrackOverrides(source) {
    if (!Module || !source) return;
    for (const [ch, prog] of Object.entries(trackOverrides)) {
      Module._SourceSendMidiEvent(source, 0xC0 | Number(ch), prog, 0);
    }
  }

  // Громкость дорожек = MIDI CC7, мьют = маска каналов; единая точка применения
  // к ЛЮБОМУ источнику (создание файла, перемотка, стоп, A-B, полная генерация).
  // CC7 раскатывается в стартовую громкость нот в C++ (OnNoteOn), маска гасит
  // канал целиком. Состояние источника в C++ теряется при каждом пересоздании,
  // поэтому известные JS значения возвращаются здесь.
  function sendChannelMix() { applyTrackMix(currentSource); }
  function applyTrackMix(source) {
    if (!Module || !source) return;
    // Синтезатор по GM/SF2 умолчанию держит CC7=100 на всех каналах, поэтому шлём
    // только каналы с известным JS-состоянием: иначе дефолтные CC7=100
    // возвращаются фидбеком и затирают начальную громкость из файла.
    for (let ch = 0; ch < 16; ch++) {
      if (trackCC7[ch] === undefined) continue;
      Module._SourceSendMidiEvent(source, 0xB0 | ch, 0x07, trackCC7[ch] & 127);
    }
    Module._SourceSetChannelMute(source, channelMuteMask());
  }

  // ---- Метры дорожек (rAF) ----------------------------------------------

  // Пока панель дорожек видима: раз в кадр осушает кольцо фидбека синтезатора
  // (NoteOn/CC7/Program Change из любого источника) и обновляет прямоугольники
  // нот с яркостью по громкости. Когда панель скрыта, rAF-цикл не идёт —
  // расходов нет. Цикл продолжает идти и на паузе: иначе ярлыки замирали
  // «залипшими яркими» до следующей ноты на том же канале.
  function pollTrackMeters() {
    metersRaf = 0;
    if (!Module) return;
    const panelVisible = midiTracks.length > 0 && !els.tracksPanel.hidden;
    if (!panelVisible) return;
    // Realtime source state lives in AudioWorklet. Meter snapshots are tiny
    // control messages (feedback ring + 16 envelope bytes), never PCM.
    const offline = generationActive || !!pregenAudio;
    const liveStream = !offline && realtimeSongLoaded && !paused;
    const now = performance.now();

    if (liveStream && !realtimeMeterPending) {
      const needLevels = levelsDirty || now - lastLevelsAt >= LEVEL_POLL_MS;
      realtimeMeterPending = realtimePost({ type: "meters", levels: needLevels });
      if (needLevels) {
        levelsDirty = false;
        lastLevelsAt = now;
      }
    }

    if (liveStream && realtimeLevels) {
      const levels = realtimeLevels;
      realtimeLevels = null;
      let meterStub = true;
      for (let ch = 0; ch < 16; ch++) {
        if (levels[ch] !== 0xFF) { meterStub = false; break; }
      }
      for (const ch of channelsOfTracks) {
        const g = noteGlow[ch];
        if (!g) continue;
        const env = meterStub ? 0 : (levels[ch] || 0) / 127;
        if (!meterStub && env > 0) {
          g.target = env * noteCcFactor(ch, g.note, g.vel);
          g.live = true;
        } else if (now < (g.fallbackUntil || 0)) {
          const left = Math.max(0, (g.fallbackUntil - now) / NOTE_FALLBACK_MS);
          g.target = noteCcFactor(ch, g.note, g.vel) * left;
          g.live = true;
        } else {
          g.target = 0;
          g.live = false;
        }
      }
    } else if (liveStream) {
      // Until the next worklet snapshot arrives, keep the fixed-duration
      // fallback alive for builds without INTRA_UI_METERS.
      for (const ch of channelsOfTracks) {
        const g = noteGlow[ch];
        if (!g || g.live) continue;
        if (now < (g.fallbackUntil || 0)) {
          const left = Math.max(0, (g.fallbackUntil - now) / NOTE_FALLBACK_MS);
          g.target = noteCcFactor(ch, g.note, g.vel) * left;
          g.live = true;
        }
      }
    } else {
      realtimeMeterPending = false;
      realtimeLevels = null;
      for (const ch of channelsOfTracks) {
        const g = noteGlow[ch];
        if (!g) continue;
        g.target = 0;
        g.live = false;
      }
    }

    // Интерполяция показанного уровня к цели: без неё яркость прыгала шагами
    // опроса, а на отпускании резко пропадала.
    const dt = lastFrameAt ? Math.min(120, now - lastFrameAt) : 16;
    lastFrameAt = now;
    const k = 1 - Math.exp(-dt / GLOW_TAU_MS);
    for (const [ch, box] of noteBoxes) {
      const g = noteGlow[ch];
      if (!g || g.note < 0) {
        if (box.textContent !== "—") box.textContent = "—";
        box.style.setProperty("--lum", SILENT_LUM);
        continue;
      }
      g.level += (g.target - g.level) * k;
      // Номер ноты НЕ забываем при гашении: на паузе индикатор гаснет, но та же
      // нота продолжает звучать в синтезаторе — при возобновлении он обязан
      // вернуться сам, без нового NoteOn. Гаснет только изображение.
      if (!g.live && g.level < 0.02) {
        g.level = 0;
        if (box.textContent !== "—") box.textContent = "—";
        box.style.setProperty("--lum", SILENT_LUM);
        continue;
      }
      box.textContent = NOTE_NAMES[g.note % 12] + noteOctave(g.note);
      box.style.setProperty("--lum", (0.12 + 0.88 * Math.min(1, g.level)).toFixed(3));
    }
    metersRaf = requestAnimationFrame(pollTrackMeters);
  }

  // Применяет события фидбека к UI: CC7 → слайдер/чекбокс, ProgramChange →
  // комбобокс дорожки (файл сам сменил инструмент канала).
  function applyMidiFeedback(u8, n) {
    for (let i = 0; i < n; i++) {
      const st = u8[i*3], d0 = u8[i*3+1], d1 = u8[i*3+2];
      const kind = st & 0xF0, ch = st & 0x0F;
      if (kind === 0x90) {
        // NoteOn из синтезатора: индикатор ноты дорожки.
        if (d1 > 0) noteGlowOn(ch, d0, d1);
      } else if (kind === 0x80) {
        // Отпускание: яркость пересчитываем сразу — видно спад релиза, не
        // дожидаясь очередного 100-мс опроса.
        levelsDirty = true;
      } else if (kind === 0xB0 && d0 === 0x07) {
        trackCC7[ch] = d1;
        // Файл изменил громкость канала: показываем её (мьют — отдельный
        // слой, он файловой громкостью не снимается и не ставится).
        const row = document.querySelector('.track-row[data-channel="' + ch + '"]');
        if (row) {
          const vol = row.querySelector(".track-vol");
          const label = row.querySelector(".track-vol-label");
          if (vol) vol.value = String(d1);
          if (label) label.textContent = volLabelOf(ch);
        }
      } else if (kind === 0xC0) {
        if (trackOverrides[ch] === undefined) trackProgram[ch] = d0;
        // ProgramChange из файла: обновить комбобокс дорожки (если она не
        // переопределена пользователем и не перетаскивается мышью сейчас).
        const row = document.querySelector('.track-row[data-channel="' + ch + '"]');
        const sel = row && row.querySelector(".track-prog");
        if (sel && trackOverrides[ch] === undefined && document.activeElement !== sel) {
          sel.value = String(d0);
        }
      }
    }
  }

  // Индикаторы строятся из NoteOn-фидбека и уровней огибающих
  // синтезатора; цикл запускается при перерисовке панели.
  function metersRestart() {
    levelsDirty = true;
    lastFrameAt = 0;
    if (!metersRaf && midiTracks.length) metersRaf = requestAnimationFrame(pollTrackMeters);
  }
  // Цикл на паузе/стопе не останавливается намеренно: он гасит
  // индикаторы и заканчивается сам, когда панель дорожек скрыта.

  // ---- Wiring ------------------------------------------------------------

  els.fileInput.addEventListener("change", (e) => {
    const file = e.target.files && e.target.files[0];
    if (!file) return;
    // Block Play while the file is still being read/loaded, so a click that
    // lands in this async window cannot start generation on the old source
    // (whose source loadFromBytes is about to free -> WASM use-after-free).
    loadingFile = true;
    const reader = new FileReader();
    reader.onload = () =>
      loadFromBytes(new Uint8Array(reader.result), file.name);
    reader.onerror = () => {
      loadingFile = false;
      setStatus("Не удалось прочитать файл", true);
    };
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
  els.stopBtn.addEventListener("click", stopPlayback);    els.noteTestNotes.querySelectorAll("[data-note]").forEach((button) => {
    button.addEventListener("click", () => {
      auditionTestNote(parseInt(button.dataset.note, 10));
    });
  });  els.liveBtn.addEventListener("click", () => {
    if (keyboardEnabled) stopKeyboard();
    else enableExternalMidi();
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
    if (!pianoSourceReady && !ensureKeyboardSource()) return;
    pianoSourceReady = true;
    pressedNotes.set(e.pointerId, { note, channel: liveChannel });
    if (sendMidiEvent(0x90 | liveChannel, note, 100)) {
      key.classList.add("active");
    }
  });
  els.piano.addEventListener("pointerup", releasePointer);
  els.piano.addEventListener("pointercancel", releasePointer);
  els.piano.addEventListener("lostpointercapture", releasePointer);

  els.octDown.addEventListener("click", () => shiftPiano(-1));
  els.octUp.addEventListener("click", () => shiftPiano(1));

  els.seek.addEventListener("input", () => {
    seekTo(parseInt(els.seek.value, 10) || 0);
  });

  els.volume.addEventListener("input", () => {
    const v = parseFloat(els.volume.value);
    els.volLabel.textContent = Math.round(v * 100) + "%";
    if (gainNode) gainNode.gain.value = v;
  });

  function applyRenderParams(source = currentSource) {
    if (!Module || !source || !paramsPtr) return;
    const view = Module.HEAPF32;
    const offset = paramsPtr >> 2;
    view[offset] = renderParams.ReverbWet;
    if (typeof Module._SourceSetParams !== "function") {
      setStatus("Загружен старый WASM: реверб недоступен. Пересоберите dist.", true);
      return;
    }
    Module._SourceSetParams(source, paramsPtr);
    renderParamsGeneration++;
  }

  function applyAllRenderParams() {
    applyRenderParams(currentSource);
    realtimePost({ type: "params", reverbWet: renderParams.ReverbWet });
  }

  els.reverb.addEventListener("input", () => {
    renderParams.ReverbWet = parseFloat(els.reverb.value);
    // A pre-generated buffer contains the old effect state. Force a fresh
    // source on the next Play so the slider is audible for full-generation too.
    if (pregenAudio) {
      stopPregenNode();
      connectProcessor();
      stopPlaybackUiLoop();
      pregenAudio = null;
      pregenPos = 0;
      playedSamples = 0;
    }
    els.reverbLabel.textContent = Math.round(renderParams.ReverbWet * 100) + "%";
    applyAllRenderParams();
  });

  // Toggling "full pre-generation" invalidates the generated buffer. When the
  // WASM source was consumed by an offline render, bring it back for the
  // real-time path.
  els.pregen.addEventListener("change", () => {
    stopPregenNode();
    connectProcessor();
    stopPlaybackUiLoop();
    pregenAudio = null;
    pregenMs = null;
    pregenPos = 0;
    els.pregenResult.textContent = "Офлайн-рендер: —";
    els.pregenResult.classList.remove("error");
    if (!currentSource && midiBytes && Module) {
      try {
        const { src } = createSource(midiBytes);
        currentSource = src;
        applyRenderParams(currentSource);
      } catch (err) {
        setStatus(err.message || "Ошибка", true);
      }
    }
    playedSamples = 0;
    realtimeStartSample = 0;
    realtimePost({ type: "stop" });
    updateProgressUI();
  });

  // ---- UI state persistence (survives A/B version-switch reload) ----------
  const STORAGE_KEY = "intra.playerState.v1";
  function loadState() {
    try { return JSON.parse(localStorage.getItem(STORAGE_KEY) || "{}") || {}; }
    catch (_e) { return {}; }
  }
  function saveState(patch) {
    try { localStorage.setItem(STORAGE_KEY, JSON.stringify(Object.assign({}, loadState(), patch))); }
    catch (_e) { /* private mode etc. */ }
  }

  // Apply saved state synchronously so boot()/buildInstrumentSelect() pick it up.
  {
    const saved = loadState();
    if (Number.isInteger(saved.instrument)) {
      currentProgram = Math.max(0, Math.min(127, saved.instrument));
    }
    if (saved.volume) els.volume.value = String(Math.max(0, Math.min(2, saved.volume)));
    els.volLabel.textContent = Math.round(parseFloat(els.volume.value) * 100) + "%";
    // Reverb defaults to 100% (index.html value); only a saved override moves it.
    if (Number.isFinite(saved.reverb)) {
      renderParams.ReverbWet = Math.max(0, Math.min(2, saved.reverb));
      els.reverb.value = String(renderParams.ReverbWet);
      els.reverbLabel.textContent = Math.round(renderParams.ReverbWet * 100) + "%";
    }
    if (typeof saved.pregen === "boolean") els.pregen.checked = saved.pregen;
    if (typeof saved.drums === "boolean") els.drumsCh.checked = saved.drums;
    els.instrument.disabled = els.drumsCh.checked;
  }
  function bindPersist(el, key, read) {
    if (!el) return;
    const ev = el.tagName === "SELECT" ? "change" : (el.type === "range" || el.type === "checkbox" ? "input" : "change");
    el.addEventListener(ev, () => saveState({ [key]: read(el) }));
  }
  bindPersist(els.instrument, "instrument", (el) => parseInt(el.value, 10));
  bindPersist(els.volume, "volume", (el) => parseFloat(el.value));
  bindPersist(els.reverb, "reverb", (el) => parseFloat(el.value));
  bindPersist(els.pregen, "pregen", (el) => el.checked);
  bindPersist(els.drumsCh, "drums", (el) => el.checked);
  function updateAbHighlight() {
    document.querySelectorAll("[data-abbuild]").forEach((btn) => {
      btn.classList.toggle("active", btn.dataset.abbuild === (abBuild ? "ref" : "cur"));
      btn.classList.toggle("busy", swapping);
    });
  }
  // Reload-free A/B: re-instantiates both the main-thread offline module and
  // the realtime AudioWorklet with the selected binary. Playback position and
  // UI state are preserved; realtime PCM still never crosses threads.
  async function switchWasmBuild(toRef) {
    if (swapping || toRef === abBuild) return;
    const wasPaused = paused;
    const savedPos = Math.max(0, playedSamples || 0);
    swapping = true;
    paused = true;
    updateAbHighlight();
    if (generationActive) {
      generationAbort = true;
      while (generationActive) await yieldToUI();
    }
    setStatus("Переключение сборки WASM…");
    try {
      // Free resources owned by the OLD module instance before dropping it.
      freeSource();
      freeKeyboardSource();
      if (scratchPtr && Module) Module._free(scratchPtr);
      if (paramsPtr && Module) Module._free(paramsPtr);
      scratchPtr = 0;
      paramsPtr = 0;
      lastLevelsAt = 0;
      // A pre-generated buffer is a render of the OLD binary — drop it.
      stopPregenNode();
      connectProcessor();
      stopPlaybackUiLoop();
      pregenAudio = null;
      pregenPos = 0;
      pregenMs = null;

      abBuild = toRef;
      const loaderCfg = toRef
        ? { locateFile: (path, dir) => (path === "IntraSynth.wasm" ? dir + "IntraSynth.ref.wasm" : path) }
        : {};
      Module = await IntraMidiSynth(loaderCfg);
      scratchPtr = Module._malloc(2 * AUDIO_CHUNK * 4);
      paramsPtr = Module._malloc(4);

      // Keep the main-thread source pristine at sample 0: it exists only for
      // metadata/offline rendering now. The realtime worklet restores position.
      if (midiBytes) {
        try {
          const { src } = createSource(midiBytes);
          currentSource = src;
          totalSamples = Module._SourceSamplesLeft(src);
          applyRenderParams(currentSource);
          applyTrackOverrides(currentSource);
          applyTrackMix(currentSource);
          playedSamples = Math.min(savedPos, totalSamples);
        } catch (err) {
          freeSource();
          currentSource = 0;
          playedSamples = 0;
        }
      } else {
        currentSource = 0;
        playedSamples = 0;
      }

      // Recreate realtime audio with the selected binary. This creates the
      // keyboard source inside the worklet and restores the song position.
      await replaceRealtimeNode(playedSamples, false);
      ensureKeyboardSource();
      if (keyboardSource && !els.drumsCh.checked) {
        sendMidiEvent(0xC0 | liveChannel, currentProgram, 0);
      }

      paused = wasPaused;
      if (!paused && midiBytes) {
        connectProcessor();
        setRealtimePlaying(true);
        startPlaybackUiLoop();
      }
      setPlayIcon(paused);
      updateProgressUI();
      setStatus("Сборка переключена на " + (toRef ? "A/B: HEAD a99c8ec" : "текущую") + ".");
    } catch (err) {
      setStatus("Не удалось переключить сборку WASM: " + err, true);
      paused = true;
    }
    swapping = false;
    updateAbHighlight();
  }

  // ---- Boot --------------------------------------------------------------
  async function boot() {
    buildInstrumentSelect();
    buildPiano();
    updateOctaveArrows();
    initMidiAccess();
    try {
      setStatus("Загрузка синтезатора…");
      if (typeof IntraMidiSynth !== "function") {
        throw new Error("WASM loader не найден (проверьте IntraSynth.js)");
      }
      // A/B build: ?wasm=ref loads the last-commit baseline wasm
      // (IntraSynth.ref.wasm, built from HEAD a99c8ec) next to the current
      // intraSynth.wasm. Both share the 1-float RenderParams ABI, so only the
      // binary needs swapping via locateFile. `abBuild` is module scope (parsed
      // synchronously above) so the header toggle highlights correctly.
      const loaderCfg = abBuild
        ? { locateFile: (path, dir) => (path === "IntraSynth.wasm" ? dir + "IntraSynth.ref.wasm" : path) }
        : {};
      Module = await IntraMidiSynth(loaderCfg);
      scratchPtr = Module._malloc(2 * AUDIO_CHUNK * 4);
      paramsPtr = Module._malloc(4);
      ensureAudio();
      await ensureRealtimeNode();
      ensureKeyboardSource();
      setStatus(abBuild
        ? "Синтезатор готов (A/B: HEAD a99c8ec). Загрузите MIDI или включите MIDI-клавиатуру."
        : "Синтезатор готов. Загрузите MIDI или включите MIDI-клавиатуру.");
    } catch (err) {
      setStatus("Не удалось загрузить WASM-модуль: " + err, true);
    }
  }

  // Minimal test hook for automated playback checks; production UI does not
  // depend on internal effect telemetry.
  window.__synthDebug = {
    hasKeySource() { return !!keyboardSource; },
    // Состояние индикаторов нот: цель/показанный уровень из синтезатора, что
    // реально нарисовано в DOM и идёт ли rAF-цикл (для автотестов панели).
    noteGlowState() {
      const boxes = {};
      for (const [ch, box] of noteBoxes) {
        const g = noteGlow[ch];
        boxes[ch] = {
          note: g ? g.note : null,
          target: g ? Number(g.target.toFixed(3)) : null,
          level: g ? Number(g.level.toFixed(3)) : null,
          live: g ? g.live : null,
          dom: box.textContent,
          lum: box.style.getPropertyValue("--lum"),
        };
      }
      return {
        raf: !!metersRaf, paused, hasSource: !!currentSource,
        generation: generationActive, pregen: !!pregenAudio, boxes,
      };
    },
    setReverb(w) {
      const v = Math.max(0, Math.min(2, w));
      if (pregenAudio) {
        stopPregenNode();
        connectProcessor();
        stopPlaybackUiLoop();
        pregenAudio = null;
        pregenPos = 0;
        playedSamples = 0;
      }
      renderParams.ReverbWet = v;
      els.reverb.value = String(v);
      if (els.reverbLabel) els.reverbLabel.textContent = Math.round(v * 100) + "%";
      applyAllRenderParams();
      return renderParams.ReverbWet;
    },
    renderChunk(srcName) {
      // Realtime sources live on the audio thread and are intentionally not
      // synchronously readable from this debug hook.
      if (srcName !== 'current' || !Module || !currentSource) return 0;
      const n = 4096;
      return Module._SourceGetUninterleavedSamples(currentSource, scratchPtr, n, n);
    },
    playNote(note, vel, chan) {
      if (!realtimeNode || !keyboardSource) return false;
      const c = chan === undefined ? 0 : chan;
      sendMidiEvent(0xC0 | c, currentProgram, 0);
      if (vel === 0) sendMidiEvent(0x80 | c, note, 0);
      else sendMidiEvent(0x90 | c, note, vel === undefined ? 100 : vel);
      return true;
    },
    releaseNote(note, chan) {
      if (!realtimeNode || !keyboardSource) return false;
      const c = chan === undefined ? 0 : chan;
      sendMidiEvent(0x80 | c, note, 0);
      return true;
    },
    // Offline-only diagnostic. Realtime PCM deliberately never crosses the
    // AudioWorklet boundary just to satisfy a debug probe.
    outputStats(srcName, blocks) {
      if (!Module || srcName !== 'current') return null;
      const src = currentSource;
      if (!src) return null;
      const off = scratchPtr >> 2;
      const n = 4096;
      let peakL = 0, peakR = 0, sumL2 = 0, sumR2 = 0, total = 0, clipped = 0;
      for (let k = 0; k < blocks; k++) {
        const written = Module._SourceGetUninterleavedSamples(src, scratchPtr, n, n);
        if (!written) break;
        // Heap view can go stale when wasm memory grows — re-read each chunk.
        const heap = Module.HEAPF32;
        for (let i = 0; i < written; i++) {
          const l = heap[off + i], r = heap[off + n + i];
          const al = Math.abs(l), ar = Math.abs(r);
          if (al > peakL) peakL = al;
          if (ar > peakR) peakR = ar;
          if (al > 0.99 || ar > 0.99) clipped++;
          sumL2 += l * l;
          sumR2 += r * r;
        }
        total += written;
      }
      const rmsL = total ? Math.sqrt(sumL2 / total) : 0;
      const rmsR = total ? Math.sqrt(sumR2 / total) : 0;
      return { peakL, peakR, rmsL, rmsR, total, clipped };
    },
  };

  // The optional debug module (web/debug.mjs) reaches the player through this
  // small bridge; when the module is absent nothing here is used.
  window.__intraHost = {
    els,
    sendMidiEvent,
    getModule: () => Module,
    isRefBuild: () => abBuild,
    switchWasmBuild,
    updateAbHighlight,
    loadState,
    saveState,
    realtimePost,
    setProgram(prog) {
      if (prog === undefined || !els.instrument) return;
      const opt = els.instrument.querySelector('option[value="' + prog + '"]');
      if (!opt) return;
      currentProgram = prog;
      els.instrument.value = String(prog);
      if (!els.drumsCh.checked) sendMidiEvent(0xC0 | liveChannel, currentProgram, 0);
    },
  };

  window.MidiSynthApp = {
    boot, loadFromBytes, loadFromUrl, sendMidiEvent,
    enableExternalMidi, stopKeyboard, allNotesOff,
  };
  boot();
})();