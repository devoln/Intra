# Web Audio delivery и underrun

Дата: 2026-09-26

## Проблема

При небольшой фоновой нагрузке UI начинал хрипеть даже после полного offline render. Причина оказалась не в synth CPU: готовый PCM всё равно подавался через `ScriptProcessorNode(256)` и `onaudioprocess` на main thread. При 44.1 кГц это оставляло около 5.8 ms на callback, а внутри callback дополнительно выполнялось UI/progress обслуживание.

## Принятая архитектура

### Полный pre-render

Готовый stereo PCM превращается в `AudioBuffer` и воспроизводится нативным `AudioBufferSourceNode`. JS больше не кормит аудио блоками после завершения render.

### Realtime MIDI и live keyboard

`AudioWorkletProcessor` держит собственный экземпляр того же WASM и вызывает synth ABI непосредственно на Web Audio rendering thread. Между main thread и worklet идут только MIDI/control сообщения и небольшие meter snapshots. PCM между потоками не пересылается, `SharedArrayBuffer` не требуется.

## Что пришлось исправить при миграции

- boot ждёт готовность worklet до включения live keyboard;
- A/B switch пересоздаёт realtime WASM вместе с main-thread module;
- track NoteOn/CC/Program feedback приходит часто, а levels остаются редким UI snapshot;
- heap views в worklet кэшируются до `memory.grow()`, без per-quantum `TypedArray`/`subarray` мусора;
- `ScriptProcessorNode`/`onaudioprocess` полностью удалены.

## Что не использовали

- Worker → Worklet PCM bridge: лишний producer/consumer boundary и риск underrun.
- Обязательный SAB/COOP/COEP: текущий raw-WASM ABI достаточно мал, чтобы инстанцировать модуль прямо в worklet.
- Generated Emscripten loader внутри worklet: он рассчитан на `window`/обычный Worker, поэтому используется прямой `WebAssembly.Instance`.

## Проверки

Production `synth-worklet.js` прогнан с exact owner-approved 226799-B WASM: live keyboard и MIDI-file path дают ненулевой stereo PCM, feedback и note levels приходят. Статический audit подтверждает отсутствие `ScriptProcessorNode`.

Владелец проверил вариант в реальном UI: базовое воспроизведение работает. Browser-specific проблемы, если появятся позже, исправляются поверх этой архитектуры без возврата audio delivery на main thread.
