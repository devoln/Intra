# Piano sample cache

Дата: 2026-09-22..26

## Цель

Убрать повторный дорогой additive render начала акустических piano notes, не меняя принятую модель длинного хвоста и не создавая отдельный sample set для каждой MIDI-ноты.

## Что сработало

- Один raw stereo prefix хранится на физический acoustic sample region, а не на MIDI key.
- Ноты внутри региона читают общий prefix с pitch resampling; AGP/BAP могут делить cache только когда underlying source recipe действительно одинаков.
- Cache находится до velocity filter, CC7/CC10 и release, поэтому один PCM можно безопасно переиспользовать для разных velocity/control states.
- Prewarm выполняется вне audio callback.
- Playback объединяет resampling, velocity filter, pan и release в одном проходе.
- Mono API проходит через общий stereo hot path, чтобы не дублировать большой additive kernel.
- Prefix увеличен до 1000 ms source time и хранится компактно как `short`; после границы длинная нота продолжает live additive state.
- Ранний NoteOff внутри cached prefix сохраняет release semantics.

## Что пробовали и не оставили

- Cache на каждую MIDI-ноту: больше памяти и дублирование одинакового физического source.
- Отдельный generic resampler: лишние проходы и буферы без выигрыша для этого hot path.
- Sharing по названию/GM family: недостаточно безопасно; sharing разрешён только при одинаковом pre-cache source recipe.

## Проверка

Cache/live handoff и early release проверялись на разных block sizes и коротких повторных нотах. После изменения ранней string physics формат cache менять не понадобилось. Финальный owner listening подтвердил корректное звучание; дальнейшая оптимизация synth после этого признана нецелесообразной.
