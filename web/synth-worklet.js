// Intra MIDI Synth realtime AudioWorklet.
//
// The main UI keeps its regular Emscripten Module for MIDI metadata and full
// offline rendering. Realtime audio owns a second instance of the *same* WASM
// binary here, on the Web Audio rendering thread. Only control/MIDI messages
// cross the MessagePort; PCM never leaves the audio thread.

const ABI = Object.freeze({
  memory: "e",
  init: "f",
  createFile: "g",
  freeSource: "h",
  createLive: "i",
  sendMidi: "j",
  setParams: "k",
  drainFeedback: "l",
  getNoteLevels: "m",
  setMute: "n",
  fastForward: "o",
  setGuitarTweaks: "p",
  samplesLeft: "q",
  render: "r",
  malloc: "t",
  free: "u",
});

const MAX_RENDER_QUANTUM = 4096;
const FEEDBACK_CAP = 64;
const GUITAR_TWEAK_FLOATS = 24;

class IntraSynthProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    this.wasm = null;
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
      this.memory = this.wasm[ABI.memory];
      this.heapBuffer = null;
      this.heapU8View = null;
      this.heapF32View = null;
      this.refreshHeapViews();
      this.wasm[ABI.init]();

      this.scratchPtr = this.wasm[ABI.malloc](MAX_RENDER_QUANTUM * 2 * 4);
      this.paramsPtr = this.wasm[ABI.malloc](4);
      this.feedbackPtr = this.wasm[ABI.malloc](FEEDBACK_CAP * 3);
      this.levelsPtr = this.wasm[ABI.malloc](16);
      this.guitarPtr = this.wasm[ABI.malloc](GUITAR_TWEAK_FLOATS * 4);
      this.keyboard = this.wasm[ABI.createLive](sampleRate, 2);
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
      // emscripten_resize_heap. In practice the current 226799-byte build starts
      // with ~16 MiB and does not grow while creating the tested live/file
      // sources, but keep the ABI complete for unusual MIDI/instrument mixes.
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
      b: () => { throw new Error("WASM abort"); },
      // fd_write: production synth should not print from the realtime thread.
      c: () => 0,
      d: (code) => { throw new Error("WASM exit " + code); },
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
    if (this.song) this.wasm[ABI.freeSource](this.song);
    this.song = 0;
    this.endedReported = false;
  }

  createSong(position = 0) {
    this.freeSong();
    if (!this.songBytes || !this.songBytes.byteLength) return;
    const ptr = this.wasm[ABI.malloc](this.songBytes.byteLength);
    this.heapU8().set(this.songBytes, ptr);
    try {
      this.song = this.wasm[ABI.createFile](ptr, this.songBytes.byteLength, sampleRate, 2);
    } finally {
      this.wasm[ABI.free](ptr);
    }
    if (!this.song) throw new Error("Не удалось создать realtime MIDI source");
    this.applyParamsTo(this.song);
    this.applyTrackState();
    if (position > 0) this.wasm[ABI.fastForward](this.song, position >>> 0);
  }

  applyParamsTo(source) {
    if (!source) return;
    this.heapF32()[this.paramsPtr >> 2] = this.reverbWet;
    this.wasm[ABI.setParams](source, this.paramsPtr);
  }

  applyTrackState() {
    if (!this.song) return;
    for (const [chText, program] of Object.entries(this.trackOverrides)) {
      this.wasm[ABI.sendMidi](this.song, 0xC0 | (+chText & 15), program & 255, 0);
    }
    for (const [chText, value] of Object.entries(this.trackCC7)) {
      this.wasm[ABI.sendMidi](this.song, 0xB0 | (+chText & 15), 0x07, value & 127);
    }
    this.wasm[ABI.setMute](this.song, this.muteMask >>> 0);
  }

  postMeterSnapshot(includeLevels) {
    if (!this.song) {
      this.port.postMessage({ type: "meters", feedback: null, levels: null });
      return;
    }
    const n = this.wasm[ABI.drainFeedback](this.song, this.feedbackPtr, FEEDBACK_CAP);
    const heap = this.heapU8();
    const feedback = n ? Array.from(heap.subarray(this.feedbackPtr, this.feedbackPtr + n * 3)) : null;
    let levels = null;
    if (includeLevels) {
      heap.fill(0xFF, this.levelsPtr, this.levelsPtr + 16);
      this.wasm[ABI.getNoteLevels](this.song, this.levelsPtr);
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
          if (this.song) this.wasm[ABI.sendMidi](this.song, message.status & 255, message.data0 & 255, message.data1 & 255);
          break;
        case "keyboardMidi":
          if (this.keyboard) this.wasm[ABI.sendMidi](this.keyboard, message.status & 255, message.data0 & 255, message.data1 & 255);
          break;
        case "params":
          this.reverbWet = +message.reverbWet || 0;
          this.applyParamsTo(this.song);
          this.applyParamsTo(this.keyboard);
          break;
        case "mute":
          this.muteMask = message.mask >>> 0;
          if (this.song) this.wasm[ABI.setMute](this.song, this.muteMask);
          break;
        case "guitarTweaks": {
          const values = message.values || [];
          const view = this.heapF32();
          const off = this.guitarPtr >> 2;
          for (let i = 0; i < GUITAR_TWEAK_FLOATS; i++) view[off + i] = +values[i] || 0;
          this.wasm[ABI.setGuitarTweaks](this.guitarPtr);
          break;
        }
        case "meters":
          this.postMeterSnapshot(!!message.levels);
          break;
        case "dispose":
          this.playing = false;
          this.freeSong();
          if (this.keyboard) this.wasm[ABI.freeSource](this.keyboard);
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
    const written = this.wasm[ABI.render](source, this.scratchPtr, n, n);
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
      if (written === 0 && this.wasm[ABI.samplesLeft](this.song) === 0) {
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
