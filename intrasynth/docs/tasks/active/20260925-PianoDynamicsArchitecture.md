# Piano dynamics и runtime architecture

Дата: 2026-09-25..26

## Цель

Отделить MIDI velocity/CC7 от низкоуровневого sampler state и упростить горячий piano path без изменения принятого velocity law.

## Принятая архитектура

- На NoteOn вычисляется компактный `NoteOnParams {Strike, Gain}`; sampler не хранит raw MIDI velocity.
- CC7 остаётся output/pruning gain канала и может меняться после NoteOn без пересоздания голоса.
- Опциональный `VelocityModulator` компилируется только для специальных runtime/minexe профилей; canonical fast build использует принятый velocity law напрямую.
- x8 sustain kernel используется на wasm32/x86-64; i386 остаётся на x4 из-за register pressure.
- Stereo recurrence state хранит уже включённый left gain, поэтому отдельный `mStereoPartL` не нужен.
- После DecayOnset attack state считается завершённым и не участвует в sustain arithmetic.

## Что пробовали и отбросили

- Per-block нормализация правого stereo state: уменьшала состояние, но добавляла деления и заметно замедляла render.
- Несколько вариантов lifetime packing: максимум давали небольшой/шумный выигрыш и меняли PCM сильнее, чем оправдывает результат.
- Более агрессивный pruning не принят без отдельного качества-gate.

## Проверки

- CC7=0/low с последующим подъёмом не убивает голос преждевременно.
- Длинные ноты не показывают растущую ошибку recurrence.
- x86-64 register audit после stereo-state cleanup показал существенно меньше spills в аналогичном x8 kernel.

## Итог

Owner listening финального варианта пройден. На пользовательском workload последние изменения дают около 1–5% ускорения; дальнейшее усложнение runtime прекращено.
