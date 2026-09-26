// Intra MIDI Synth realtime AudioWorklet.
//
// The main UI keeps its regular Emscripten Module for MIDI metadata and full
// offline rendering. Realtime audio owns a second instance of the *same* WASM
// binary here, on the Web Audio rendering thread. Only control/MIDI messages
// cross the MessagePort; PCM never leaves the audio thread.
//
// Emscripten minifies the export names and inserts its own entries (memory, the
// indirect-function table, __wasm_call_ctors) whose letters shift with the build
// flags, so the mapping is resolved from the module itself instead of being
// hard-coded per build. The synth C ABI follows __wasm_call_ctors in its
// declaration order (see EmscriptenInterface.cpp).
const C_ABI_ORDER = [
  "createFile", "freeSource", "createLive", "sendMidi", "setParams",
  "drainFeedback", "getNoteLevels", "setMute", "fastForward",
  "setGuitarTweaks", "samplesLeft", "render", "midiInfo", "malloc", "free",
];

function resolveAbi(exports) {
  const names = Object.keys(exports);
  const memory = names.find((n) => exports[n] instanceof WebAssembly.Memory);
  if (!memory) throw new Error("WASM module has no exported memory");
  const funcs = names.filter((n) => typeof exports[n] === "function");
  if (funcs.length < C_ABI_ORDER.length + 1) {
    throw new Error("WASM module is missing synth ABI exports: " + funcs.join(","));
  }
  const abi = { memory, init: funcs[0] };
  C_ABI_ORDER.forEach((key, i) => { abi[key] = funcs[i + 1]; });
  return abi;
}

const MAX_RENDER_QUANTUM = 4096;
const FEEDBACK_CAP = 64;
const GUITAR_TWEAK_FLOATS = 24;

class IntraSynthProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    this.wasm = null;
    this.abi = null;
    this.memory = null;
    this.song = 0;
    this.keyboard = 0;
    this.songBytes = null;
    this.playing = false;
    this.disposed = false;
    this.endedReported = false;
    this.reverbWet = 0;
    this.trackOverrides = {};
    this.trackCC7 = {};
    this.muteMask = 0;

    try {
      const opts = options.processorOptions || {};
      const module = opts.wasmModule || new WebAssembly.Module(opts.wasmBytes);
      const imports = this.makeImports();
      const instance = new WebAssembly.Instance(module, imports);
      this.wasm = instance.exports;
      this.abi = resolveAbi(this.wasm);
      this.memory = this.wasm[this.abi.memory];
      this.heapBuffer = null;
      this.heapU8View = null;
      this.heapF32View = null;
      this.refreshHeapViews();
      this.wasm[this.abi.init]();

      this.scratchPtr = this.wasm[this.abi.malloc](MAX_RENDER_QUANTUM * 2 * 4);
      this.paramsPtr = this.wasm[this.abi.malloc](4);
      this.feedbackPtr = this.wasm[this.abi.malloc](FEEDBACK_CAP * 3);
      this.levelsPtr = this.wasm[this.abi.malloc](16);
      this.guitarPtr = this.wasm[this.abi.malloc](GUITAR_TWEAK_FLOATS * 4);
      this.keyboard = this.wasm[this.abi.createLive](sampleRate, 2);
      this.applyParamsTo(this.keyboard);

      this.port.onmessage = (event) => this.onMessage(event.data || {});
      this.port.postMessage({ type: "ready" });
    } catch (error) {
      this.disposed = true;
      this.port.postMessage({ type: "error", message: String(error && error.stack || error) });
    }
  }

  makeImports() {
    return { a: {
      // emscripten_resize_heap. In practice the current build starts with
      // ~16 MiB and does not grow while creating the tested live/file sources,
      // but keep the ABI complete for unusual MIDI/instrument mixes.
      a: (requestedSize) => {
        if (!this.memory) return 0;
        const oldSize = this.memory.buffer.byteLength;
        if (requestedSize <= oldSize) return 1;
        const maxHeapSize = 0x80000000;
        if (requestedSize > maxHeapSize) return 0;
        for (let cutDown = 1; cutDown <= 4; cutDown *= 2) {
          let newSize = oldSize * (1 + 0.2 / cutDown);
          newSize = Math.min(newSize, requestedSize + 100663296);
          newSize = Math.min(maxHeapSize, Math.ceil(Math.max(requestedSize, newSize) / 65536) * 65536);
          try {
            this.memory.grow((newSize - oldSize) / 65536);
            return 1;
          } catch (_error) {}
        }
        return 0;
      },
      // _emscripten_memcpy_js: bulk copy inside wasm memory.
      b: (dest, src, num) => { this.heapU8().copyWithin(dest, src, src + num); },
      // abort / __abort_js.
      c: () => { throw new Error("WASM abort"); },
      // exit and fd_write: kept for builds that retain stdio; the production
      // synth must not print from the realtime thread.
      d: (code) => { throw new Error("WASM exit " + code); },
      e: () => 0,
    }};
  }

  refreshHeapViews() {
    const buffer = this.memory.buffer;
    if (buffer === this.heapBuffer) return;
    this.heapBuffer = buffer;
    this.heapU8View = new Uint8Array(buffer);
    this.heapF32View = new Float32Array(buffer);
  }

  heapU8() {
    this.refreshHeapViews();
    return this.heapU8View;
  }

  heapF32() {
    this.refreshHeapViews();
    return this.heapF32View;
  }

  freeSong() {
    if (this.song) this.wasm[this.abi.freeSource](this.song);
    this.song = 0;
    this.endedReported = false;
  }

  createSong(position = 0) {
    this.freeSong();
    if (!this.songBytes || !this.songBytes.byteLength) return;
    const ptr = this.wasm[this.abi.malloc](this.songBytes.byteLength);
    this.heapU8().set(this.songBytes, ptr);
    try {
      this.song = this.wasm[this.abi.createFile](ptr, this.songBytes.byteLength, sampleRate, 2);
    } finally {
      this.wasm[this.abi.free](ptr);
    }
    if (!this.song) throw new Error("Не удалось создать realtime MIDI source");
    this.applyParamsTo(this.song);
    this.applyTrackState();
    if (position > 0) this.wasm[this.abi.fastForward](this.song, position >>> 0);
  }

  applyParamsTo(source) {
    if (!source) return;
    this.heapF32()[this.paramsPtr >> 2] = this.reverbWet;
    this.wasm[this.abi.setParams](source, this.paramsPtr);
  }

  applyTrackState() {
    if (!this.song) return;
    for (const [chText, program] of Object.entries(this.trackOverrides)) {
      this.wasm[this.abi.sendMidi](this.song, 0xC0 | (+chText & 15), program & 255, 0);
    }
    for (const [chText, value] of Object.entries(this.trackCC7)) {
      this.wasm[this.abi.sendMidi](this.song, 0xB0 | (+chText & 15), 0x07, value & 127);
    }
    this.wasm[this.abi.setMute](this.song, this.muteMask >>> 0);
  }

  postMeterSnapshot(includeLevels) {
    if (!this.song) {
      this.port.postMessage({ type: "meters", feedback: null, levels: null });
      return;
    }
    const n = this.wasm[this.abi.drainFeedback](this.song, this.feedbackPtr, FEEDBACK_CAP);
    const heap = this.heapU8();
    const feedback = n ? Array.from(heap.subarray(this.feedbackPtr, this.feedbackPtr + n * 3)) : null;
    let levels = null;
    if (includeLevels) {
      heap.fill(0xFF, this.levelsPtr, this.levelsPtr + 16);
      this.wasm[this.abi.getNoteLevels](this.song, this.levelsPtr);
      levels = Array.from(heap.subarray(this.levelsPtr, this.levelsPtr + 16));
    }
    this.port.postMessage({ type: "meters", feedback, levels });
  }

  onMessage(message) {
    if (this.disposed || !this.wasm) return;
    try {
      switch (message.type) {
        case "loadSong": {
          this.songBytes = new Uint8Array(message.bytes || 0);
          this.trackOverrides = message.overrides || {};
          this.trackCC7 = message.cc7 || {};
          this.muteMask = message.muteMask >>> 0;
          this.reverbWet = +message.reverbWet || 0;
          this.createSong(message.position >>> 0);
          this.playing = !!message.playing;
          this.port.postMessage({ type: "songLoaded" });
          break;
        }
        case "play":
          this.playing = !!message.value && !!this.song;
          break;
        case "stop":
          this.playing = false;
          if (this.songBytes) this.createSong(0);
          break;
        case "seek":
          if (this.songBytes) this.createSong(message.position >>> 0);
          break;
        case "songMidi":
          if (this.song) this.wasm[this.abi.sendMidi](this.song, message.status & 255, message.data0 & 255, message.data1 & 255);
          break;
        case "keyboardMidi":
          if (this.keyboard) this.wasm[this.abi.sendMidi](this.keyboard, message.status & 255, message.data0 & 255, message.data1 & 255);
          break;
        case "params":
          this.reverbWet = +message.reverbWet || 0;
          this.applyParamsTo(this.song);
          this.applyParamsTo(this.keyboard);
          break;
        case "mute":
          this.muteMask = message.mask >>> 0;
          if (this.song) this.wasm[this.abi.setMute](this.song, this.muteMask);
          break;
        case "guitarTweaks": {
          const values = message.values || [];
          const view = this.heapF32();
          const off = this.guitarPtr >> 2;
          for (let i = 0; i < GUITAR_TWEAK_FLOATS; i++) view[off + i] = +values[i] || 0;
          this.wasm[this.abi.setGuitarTweaks](this.guitarPtr);
          break;
        }
        case "meters":
          this.postMeterSnapshot(!!message.levels);
          break;
        case "dispose":
          this.playing = false;
          this.freeSong();
          if (this.keyboard) this.wasm[this.abi.freeSource](this.keyboard);
          this.keyboard = 0;
          this.disposed = true;
          break;
      }
    } catch (error) {
      this.port.postMessage({ type: "error", message: String(error && error.stack || error) });
    }
  }

  mixSource(source, left, right, add) {
    if (!source) return 0;
    const n = left.length;
    if (n > MAX_RENDER_QUANTUM) return 0;
    const written = this.wasm[this.abi.render](source, this.scratchPtr, n, n);
    const heap = this.heapF32();
    const off = this.scratchPtr >> 2;
    if (add) {
      for (let i = 0; i < written; i++) {
        left[i] += heap[off + i];
        right[i] += heap[off + n + i];
      }
    } else {
      // process() has already zeroed the whole output quantum, so overwrite
      // only the samples produced by this source. A scalar copy avoids
      // per-quantum TypedArray.subarray view allocations on the audio thread.
      for (let i = 0; i < written; i++) {
        left[i] = heap[off + i];
        right[i] = heap[off + n + i];
      }
    }
    return written;
  }

  process(_inputs, outputs) {
    const output = outputs[0];
    if (!output || output.length < 2) return true;
    const left = output[0], right = output[1];
    left.fill(0); right.fill(0);
    if (this.disposed || !this.wasm) return true;

    if (this.playing && this.song) {
      const written = this.mixSource(this.song, left, right, false);
      if (written === 0 && this.wasm[this.abi.samplesLeft](this.song) === 0) {
        this.playing = false;
        if (!this.endedReported) {
          this.endedReported = true;
          this.port.postMessage({ type: "ended" });
        }
      }
    }
    if (this.keyboard) this.mixSource(this.keyboard, left, right, true);
    return true;
  }
}

registerProcessor("intra-midi-synth", IntraSynthProcessor);
