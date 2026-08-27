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
    allNotesOff: document.getElementById("allNotesOff"),
    instrument: document.getElementById("instrument"),
    drumsCh: document.getElementById("drumsCh"),
    reverb: document.getElementById("reverb"),
    reverbLabel: document.getElementById("reverbLabel"),
    midiStatus: document.getElementById("midiStatus"),
    piano: document.getElementById("piano"),
    noteTestNotes: document.getElementById("noteTestNotes"),
    noteTestStatus: document.getElementById("noteTestStatus"),
    audioSampleRate: document.getElementById("audioSampleRate"),
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
  let processor = null;
  let gainNode = null;
  let scratchPtr = 0;
  let paramsPtr = 0;
  const renderParams = { ReverbWet: 0 };
  let renderParamsGeneration = 0;

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

    if ((!currentSource && !pregenAudio && !keyboardSource) || !Module) {
      outL.fill(0);
      outR.fill(0);
      return;
    }

    if (pregenAudio && !paused) {
      // Render parameters are applied before generation; changing them invalidates
      // the cached buffer so this path never hides the live WASM effect state.
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
    const written = currentSource && !paused
      ? Module._SourceGetUninterleavedSamples(currentSource, scratchPtr, n, n)
      : 0;
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

    if (keyboardSource) {
      const keyboardWritten = Module._SourceGetUninterleavedSamples(
        keyboardSource, scratchPtr, n, n
      );
      for (let i = 0; i < keyboardWritten; i++) {
        outL[i] += heap[off + i];
        outR[i] += heap[off + n + i];
      }
    }

    if (currentSource && !paused) {
      playedSamples += n;
      updateProgressUI();
    }

    if (currentSource && !paused && written === 0 && Module._SourceSamplesLeft(currentSource) === 0) {
      stopPlayback();
    }
  }

  function freeSource() {
    if (currentSource && Module) Module._SourceFree(currentSource);
    currentSource = 0;
  }

  function freeKeyboardSource() {
    if (keyboardSource && Module) Module._SourceFree(keyboardSource);
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
      if (!paramsPtr) paramsPtr = Module._malloc(4);
      totalSamples = Module._SourceSamplesLeft(src);
      applyRenderParams(currentSource);
    applyRenderParams(keyboardSource);
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
    // Если источник уже частично потреблён реальным временем (играли до этого),
    // пересоздаём его, чтобы буфер начинался с 0 и процент совпадал со всей песней.
    if (midiBytes && Module._SourceSamplesLeft(currentSource) < totalSamples) {
      try {
        const { src } = createSource(midiBytes);
        freeSource();
        currentSource = src;
        applyRenderParams(currentSource);
      } catch (err) {
        setStatus(err.message || "Ошибка при пересоздании источника", true);
        return null;
      }
    }
    let left, right;
    try {
      left = new Float32Array(totalSamples);
      right = new Float32Array(totalSamples);
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
    const heap = Module.HEAPF32;
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
    return { left, right, len: pos, ms };
  }

  async function playPause() {
    if (loadingFile || (!currentSource && !pregenAudio)) return;
    ensureAudio();

    // Offline render first (with timing) when the checkbox is on and there is
    // no generated buffer yet. If generation fails, stay paused.
    if (els.pregen.checked && !pregenAudio && currentSource) {
      if (generationActive) return; // already generating — ignore double clicks
      generationActive = true;
      generationAbort = false;
      paused = true;
      els.playBtn.disabled = true;
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
      // The WASM source has been consumed by the offline render; playback now
      // reads straight from the float buffers.
      freeSource();
    }

    paused = !paused;
    setPlayIcon(paused);
    setStatus(paused ? "Пауза" : "Воспроизведение…");
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
    paused = true;
    seeking = false;
    playedSamples = 0;
    pregenPos = 0;
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
        // Новый источник стартует с параметрами по умолчанию (реверб 0) —
        // возвращаем текущие, иначе после Стоп/перемотки реверб на файле
        // «пропадал», хотя на клавишах оставался.
        applyRenderParams(currentSource);
      } catch (err) {
        freeSource();
        setStatus(err.message || "Ошибка", true);
        return;
      }
    }
    els.seek.value = "0";
    els.seek.style.setProperty("--pct", "0%");
    els.time.textContent = "0:00 / " + fmtTime(totalSamples / (audioCtx ? audioCtx.sampleRate : 44100));
    updateProgressUI();
    setStatus("Остановлено. Нажмите «Играть», чтобы воспроизвести с начала.");
  }

  async function seekTo(sample) {
    // Never seek while full generation is rendering: seekTo recreates the
    // source (freeSource + new), which would free the source the loop is
    // pulling from and hang the page in a WASM render loop.
    if (!midiBytes || generationActive) return;
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
    applyRenderParams(currentSource);

    let remaining = target;
    while (remaining > 0) {
      const n = Math.min(AUDIO_CHUNK, remaining);
      const written = Module._SourceGetUninterleavedSamples(
        currentSource, scratchPtr, n, AUDIO_CHUNK
      );
      if (written === 0) break;
      remaining -= written;
      // Yield so the UI stays responsive on long seeks (MessageChannel, not
      // setTimeout: timer throttling in iframes would make seeks crawl).
      await yieldToUI();
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
    if (!Module || !keyboardSource) return false;
    Module._SourceSendMidiEvent(
      keyboardSource, status & 0xFF, data0 & 0xFF, data1 & 0xFF
    );
    return true;
  }

  function ensureKeyboardSource() {
    if (!Module) return false;
    ensureAudio();
    if (!keyboardSource) {
      keyboardSource = Module._SourceCreateLive(audioCtx.sampleRate, 2);
      applyRenderParams(keyboardSource);
    }
    return true;
  }

  function allNotesOff() {
    if (!Module || !keyboardSource) return;
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
    els.liveBtn.textContent = "MIDI-клавиатура включена";
    setStatus("MIDI-клавиатура включена; она играет независимо от MIDI-файла");
    if (!els.drumsCh.checked) sendMidiEvent(0xC0 | liveChannel, currentProgram, 0);
    return true;
  }

  const noteTestTimers = new Map();

  function auditionTestNote(note) {
    if (!Module) {
      if (els.noteTestStatus) els.noteTestStatus.textContent = "Синтезатор ещё загружается.";
      return;
    }
    if (!pianoSourceReady && !ensureKeyboardSource()) return;
    pianoSourceReady = true;
    sendMidiEvent(0xC0, 0, 0);
    sendMidiEvent(0x90, note, 100);
    if (els.noteTestStatus) {
      els.noteTestStatus.textContent = "Звучит MIDI-нота " + note + ".";
    }
    const oldTimer = noteTestTimers.get(note);
    if (oldTimer) clearTimeout(oldTimer);
    const timer = setTimeout(() => {
      if (keyboardSource) sendMidiEvent(0x80, note, 0);
      noteTestTimers.delete(note);
    }, 1400);
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
    els.liveBtn.textContent = "Включить MIDI-клавиатуру";
    setStatus("MIDI-клавиатура выключена");
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
    applyRenderParams(keyboardSource);
  }

  els.reverb.addEventListener("input", () => {
    renderParams.ReverbWet = parseFloat(els.reverb.value);
    // A pre-generated buffer contains the old effect state. Force a fresh
    // source on the next Play so the slider is audible for full-generation too.
    if (pregenAudio) {
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
    updateProgressUI();
  });

  // ---- Boot --------------------------------------------------------------
  async function boot() {
    buildInstrumentSelect();
    buildPiano();
    initMidiAccess();
    try {
      setStatus("Загрузка синтезатора…");
      if (typeof IntraMidiSynth !== "function") {
        throw new Error("WASM loader не найден (проверьте IntraSynth.js)");
      }
      Module = await IntraMidiSynth();
      scratchPtr = Module._malloc(2 * AUDIO_CHUNK * 4);
      paramsPtr = Module._malloc(4);
      ensureAudio();
      ensureKeyboardSource();
      setStatus("Синтезатор готов. Загрузите MIDI или включите MIDI-клавиатуру.");
    } catch (err) {
      setStatus("Не удалось загрузить WASM-модуль: " + err, true);
    }
  }

  // Minimal test hook for automated playback checks; production UI does not
  // depend on internal effect telemetry.
  window.__synthDebug = {
    hasKeySource() { return !!keyboardSource; },
    setReverb(w) {
      const v = Math.max(0, Math.min(1, w));
      if (pregenAudio) { pregenAudio = null; pregenPos = 0; playedSamples = 0; }
      renderParams.ReverbWet = v;
      els.reverb.value = String(v);
      if (els.reverbLabel) els.reverbLabel.textContent = Math.round(v * 100) + "%";
      applyAllRenderParams();
      return renderParams.ReverbWet;
    },
    renderChunk(srcName) {
      const src = srcName === 'current' ? currentSource : keyboardSource;
      if (!Module || !src) return 0;
      const n = 4096;
      return Module._SourceGetUninterleavedSamples(src, scratchPtr, n, n);
    },
    playNote(note, vel, chan) {
      if (!Module || !keyboardSource) return false;
      const c = chan === undefined ? 0 : chan;
      Module._SourceSendMidiEvent(keyboardSource, 0xC0 | c, currentProgram, 0);
      if (vel === 0) {
        Module._SourceSendMidiEvent(keyboardSource, 0x80 | c, note, 0);
      } else {
        Module._SourceSendMidiEvent(keyboardSource, 0x90 | c, note, vel === undefined ? 100 : vel);
      }
      return true;
    },
    releaseNote(note, chan) {
      if (!Module || !keyboardSource) return false;
      const c = chan === undefined ? 0 : chan;
      Module._SourceSendMidiEvent(keyboardSource, 0x80 | c, note, 0);
      return true;
    },
    // Renders `blocks` chunks (4096 samples each) from the given source and
    // returns output levels for automated playback checks.
    outputStats(srcName, blocks) {
      if (!Module) return null;
      const src = srcName === 'current' ? currentSource : keyboardSource;
      if (!src) return null;
      const heap = Module.HEAPF32;
      const off = scratchPtr >> 2;
      const n = 4096;
      let peakL = 0, peakR = 0, sumL2 = 0, sumR2 = 0, total = 0, clipped = 0;
      for (let k = 0; k < blocks; k++) {
        const written = Module._SourceGetUninterleavedSamples(src, scratchPtr, n, n);
        if (!written) break;
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

  window.MidiSynthApp = {
    boot, loadFromBytes, loadFromUrl, sendMidiEvent,
    enableExternalMidi, stopKeyboard, allNotesOff,
  };
  boot();
})();