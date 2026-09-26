# Ускорение WASM-сборки

Дата: 2026-09-26

## Цель

Сократить время canonical Emscripten build без изменения результата синтеза и без смешивания этой работы с оптимизацией размера/рантайма WASM.

## Что проверяли

- Реальную CPU quota контейнера вместо слепого использования `nproc`.
- Старый giant handcrafted unity build против штатного CMake unity batching.
- Полный Intra source set против минимального closure, реально требуемого web-synth.
- Дублирующую сборку `IntraSynthCore`, когда executable и так компилировал те же synth sources.
- Prebuilt `libIntra.a` как дополнительный seed/cache для handoff.

## Что сработало

- Для доступного runtime оптимален `-j4`, соответствующий cgroup quota.
- Synth sources быстрее собираются несколькими CMake unity TU с batch size 8, а не одним гигантским TU.
- Web target компилирует только фактически используемый Intra implementation closure.
- Для Emscripten убрана дублирующая сборка `IntraSynthCore`.
- В closure явно оставлены allocator implementation files, чтобы fresh build не зависел от случайно более полного старого archive.

Fresh configure+build на измеренной конфигурации сократился примерно с 21.8 с до 9.3 с, а собственно build примерно с 11.0 с до 4.8 с. Более поздняя fresh-проверка без prebuilt archive дала около 5 с configure + 5 с build.

## Что не стало основой

- Один giant unity TU: хуже использует доступные CPU и даёт длинный одиночный compile step.
- Prebuilt `libIntra.a` как единственная стратегия: полезен как second-level cache/handoff seed, но после минимизации closure уже не является главным выигрышем.

## Инвариант

Этот слой меняет только скорость получения canonical build. Acoustic DSP и принятый render path не должны от него зависеть.
