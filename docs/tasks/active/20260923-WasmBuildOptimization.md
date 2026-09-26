# Оптимизация canonical WASM build

Дата: 2026-09-23..26

## Цель

Сократить ненужный Emscripten/runtime/linker хвост и сделать canonical web build воспроизводимым без изменения принятой акустической модели.

## Что сработало

- Canonical профиль зафиксирован как Release + plain `-Os`, SIMD128, piano tables и UI meters включены.
- Web target не тянет неиспользуемые IO/Image/System/Concurrency modules.
- `FILESYSTEM=0` и `SUPPORT_LONGJMP=0` убирают ненужный runtime хвост.
- Убраны дублирующие/неиспользуемые зависимости и exports, где это не меняет web ABI.
- Размерные и performance-фичи оформлены как явные build options, чтобы canonical конфигурация не зависела от случайных локальных флагов.

## Что не смешивается с этой задачей

- Ускорение самой компиляции вынесено в `20260926-WasmBuildSpeed.md`.
- Piano runtime/sample-cache оптимизации принадлежат отдельным слоям.
- Экспериментальные size-at-any-cost профили не являются canonical и здесь не используются.

## Итоговая проверка

Owner-approved финальный web binary после последних отключаемых synth-оптимизаций: 226799 B, SHA-256 `0d4f429626033bc9151e5bbf74e9575ff005d56e23906f350bb7a9cd524534cd`.
