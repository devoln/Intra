# Разбиение накопленных изменений на коммиты

Дата: 2026-09-26

## Цель

Разложить изменения после `synth-wip @ 7911d16f2c1eca9598b7e29a3ac2fd5ffd6fd820` на небольшую линейную историю по смыслу. Каждый слой должен быть отдельно понятен и пригоден для `git bisect`.

## План коммитов

1. `Complete piano sound`
   - принятая модель звучания пианино: measured partials/decays, contact/string reveal, velocity response, true stereo и согласованные piano-program differences;
   - без piano sample cache, hot-loop tuning, build tuning и Web Audio изменений.

2. `Normalize remaining instruments`
   - SF2-нормировка остальных GM-инструментов и shared drum models;
   - без тембральных изменений, не требуемых нормировкой.

3. `Optimize piano runtime`
   - ускорение live additive path без piano sample cache: сокращение состояния, x8 hot loop и устранение лишней работы;
   - acoustic model остаётся той же.

4. `Optimize piano sample cache`
   - region-based cache дорогого начала пианино, reuse между нотами одного физического региона, компактное PCM-хранение и корректный переход в live state.

5. `Optimize WASM build`
   - canonical Release + `-Os`, удаление ненужного runtime/linker хвоста и лишних экспортов/зависимостей без смены acoustic model.

6. `Speed up WASM builds`
   - только скорость компиляции: минимальный source closure, CMake unity batching, исключение дублирующей компиляции и использование реальной CPU quota.

7. `Update synth UI`
   - UI/meter/playback изменения, включая нативное воспроизведение полного pre-render через `AudioBufferSourceNode`.

8. `Move realtime synthesis to AudioWorklet`
   - realtime MIDI и live keyboard рендерятся внутри `AudioWorkletProcessor` собственным экземпляром WASM;
   - `ScriptProcessorNode` отсутствует, PCM между потоками не пересылается.

## Правила

- Принятые изменения описываются как последовательное развитие текущей модели.
- Отклонённые направления фиксируются только как отдельные эксперименты с краткой причиной отказа.
- Повторные замеры, не относящиеся к конкретному изменению, не дублируются.
- Эксперименты по физико-параметрическому представлению пианино ведутся отдельным worklog.
- ChatGPT-only handoff и служебные файлы в git не добавляются.
