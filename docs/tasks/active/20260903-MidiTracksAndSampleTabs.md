---
title: "MIDI track list with live per-channel instrument switching + per-instrument raw sample tabs"
status: "active"
created: 2026-09-03
started: 2026-09-03
updated: 2026-09-14
risk_level: medium
related_files:
  - web/index.html
  - web/synth.js
  - scripts/generate-sf2-samples.js
  - web/generated/samples/manifest.json
related_tasks:
  - 20260828-LoudnessBalanceFix
related_decisions:
  - 20260914-SeekKillAndSkip
  - 20260914-MidiTextEncodingHeuristic
  - 20260914-SeekReviveHeldNotes
---

# Task

User request: show every MIDI file track and its instrument, with the ability to
switch each one in real time ("В celine звучит ужасно. Там наверное какое-то
electric piano из трёх, хочу проверить какое"), and add raw SF2 samples for the
other instruments too, with tabs to pick which instrument's samples to listen to
("чтобы я мог послушать без fluidsynth и без внешней MIDI клавиатуры").

# Celine diagnosis (answer to "что не так")

Parsed `celine_dion-my_heart_will_go_on.mid` (format 1, 19 tracks, 23208 bytes):

| track | name | channel | GM prog | instrument |
|---|---|---|---|---|
| 1 | Piano (1201 notes) | ch0 | **5** | **Electric Piano 2 (Yamaha DX7)** |
| 2 | Strings | ch1 | 49 | String Ensemble 2 |
| 3 | Pan Flute | ch2 | 75 | Pan Flute |
| 4 | Sweep | ch3 | 95 | Pad 7 (halo) |
| 5 | Bass | ch4 | 35 | Fretless Bass |
| 6 | Drums | ch9 | — | drums (1231 notes) |
| 7 | Voice | ch5 | 54 | Synth Voice |
| 8 | French Horn | ch6 | 60 | French Horn |
| 9 | Bass Hit | ch7 | 116 | (unmapped → Sine2Exp) |

So the "ужасное" piano is exactly the instrument the user suspected: **EP2
(prog 5)**. Context from the 2026-08-31 worklog: our EP2 table models the DX7
FM layer's timbre while its volume was calibrated to the preset's *audible*
Soft layer (−5.8 dB vs AGP). If EP2 still reads wrong vs the soundbank, the new
"Electric Piano 2" sample tab (which extracts the Soft-layer samples) is the
A/B reference.

# Changes

## 1. Track list panel (web/index.html + web/synth.js)

- New `#tracksPanel` section between the URL panel and the player; hidden until
  a MIDI file loads.
- `parseMidiTracks(bytes)` — a ~70-line SMF parser in synth.js (track names via
  FF 03, channels + last Program Change per channel, note counts). The heavy
  parse stays in the WASM `MidiFileParser`; this is only for the UI.
  - Bug caught during development: the SMF header fields are
    format→nTracks→division; reading nTracks from the format field made every
    format-1 file parse as a single track ("Дорожек: 1"). Fixed + commented.
- One row per **channel** (MIDI instruments are channel-scoped; several tracks
  may share a channel). Row shows channel, joined track names, and a GM
  program `<select>` (reuses `GM_GROUPS`) defaulting to the file's program.
  Channel 10 rows are read-only ("канал ударных — инструмент фиксирован").
- Changing a row sends `0xC0|ch, prog` straight into the current WASM source
  (`SourceSendMidiEvent` → `MidiSynth::SetChannelProgram`), so subsequent notes
  of that channel use the new instrument immediately — no reload, no re-render.
  Option "Исходный из файла" sends 255 (0xFF), which clears the override
  (`mChannelProgramOverride[ch] = 0xFF`).
- The override map lives in JS (`trackOverrides`), because every source
  recreation (seek, stop, A/B swap, pregen toggle, generateAll) discards the
  C++ channel state. `createSource()` is the single choke point: it calls
  `applyTrackOverrides(srcPtr)` right after `SourceCreateFromMidiFileData`, so
  every recreation path re-applies the overrides automatically (including the
  ones inside functions that otherwise couldn't be patched individually).
  `loadFromBytesInner` clears the map before creating the source for a new file.
- Switching a track's instrument invalidates the pre-generated offline buffer
  (it contains the old render), same as the reverb slider.

## 2. Per-instrument raw sample tabs (web/index.html + web/synth.js)

- The hardcoded acoustic-only `.debug-rows` were replaced by a tab strip
  (`#sampleTabs`) + rows container (`#sampleRows`), filled from
  `samples/manifest.json` (built artifact, gitignored).
- Tabs: Acoustic Piano, Electric Grand, Electric Piano 1, Electric Piano 2,
  Harpsichord, Clavinet, String Ensemble, Flute. Bright (1) and Honky-Tonk (3)
  share the Clavinova sample pool with Acoustic (0) — noted in the panel
  description, no separate tabs.
- Switching a tab also switches the main instrument selector to the same GM
  program (Acoustic→0, EG→2, EP1→4, EP2→5, Harpsi→6, Clav→7, Strings→48,
  Flute→73), so the note-test buttons next to the rows play OUR synth of the
  same instrument — a true A/B against the raw soundbank sample.
- Samples are preloaded lazily per tab (fetch → ArrayBuffer → blob URL) so they
  keep playing if the server goes away; WAV header duration feeds the
  test-note hold time (`sampleDurations[tabDir][note]`), keyed per tab.

## 3. scripts/generate-sf2-samples.js (local build tool)

- Resolves instruments by **GM program number** (bank 0) instead of preset
  name. Preset names in this bank are unreliable ("Rhodes Piano" is a substring
  of "XP50EPianoRhodes"; "Yamaha DX7" has an inaudible FM layer that must not
  win the zone pick). Matches the synth's own mapping (prog 2=EG, 4=EP1,
  5=EP2, 6=Harpsi, 7=Clav) and `.scratch/gen-instrument-tables.js`.
- Zone selection = loudest velocity layer at vel 100, then least attenuation —
  exactly how the SF2 player itself picks (there is no "untransposed" concept
  in the spec). This also avoids noise layers like the harpsichord's "Key
  Noise" sample whose originalPitch accidentally matches mid keys (C4 used to
  resolve to it).
- Corrected sample sets (previous tabs had EP1 and EP2 BOTH showing XP50
  samples; now): EG→XP50 G2L..F5L, EP1→Rhodes EVP73 c2-90/c3-90/c4-90,
  EP2→Yamaha DX7 Soft layer (the audible one), Harpsi→H8'I-B..I, Clav→Clavinet
  C3..G7, Strings→XP50 strings, Flute unchanged.
- Regenerate with `node scripts/generate-sf2-samples.js` (needs
  /tmp/sf2extract/Titanic 200 GM-GS v1.2.sf2), then `node scripts/build-web.js`
  to refresh dist/.

# Verification

`node .scratch/smoke-tracks.mjs` (serves dist/, playwright):
- 8 tabs render; acoustic tab = 9 rows; EP1 tab rows preload to blob URLs and
  the instrument selector flips to 4.
- celine loads: 19 tracks parsed, 9 with notes; panel shows 9 channel rows;
  ch1 "Piano" defaults to EP2 (5); drums row read-only.
- Rendering ch1 before/after switching to EP1: RMS 0.0325 → 0.0818 (151%
  change) — the override audibly changes the file render.
- Override survives source recreation (re-seek after switching: select still 4).
- No console/page errors; `scripts/smoke-live-midi.js` and
  `scripts/smoke-test-wasm.mjs` still pass.

# Revert / notes

- Track overrides are JS-only UI state; nothing in the WASM ABI changed (no
  rebuild needed — the C++ `SetChannelProgram` path already existed).
- The old flat `dist/*_sample.wav` files are no longer referenced by the UI but
  were left in place (dist is gitignored build output).
- `.scratch/midi/analyze.js` + `.scratch/smoke-tracks.mjs` hold the probe
  copies used during development.

## 2026-09-04 (evening) — Sample-tab WAVs sounded at the WRONG note (label vs actual pitch audit)

User: "проверь все тестовые семплы, что их пометка соответствует ноте и октаве.
Часто попадается вообще не та нота, и это во всех инструментах."

### Root cause

The SF2 sample-header `originalPitch` is unreliable in this bank (every
Clavinova sample says 60) and sample NAMES use different octave conventions per
manufacturer (Roland/DX7/Clavinet: C4 = middle C; the Clavinova pool: C3 =
middle C). The tab generator therefore labeled each WAV with the REQUESTED key
while the audio played at the sample's NATURAL pitch — off by up to ±2 octaves
on many tabs (e.g. EP1 C3 sounded C2, EP2 C5 sounded C4, Flute C5 sounded C4,
Harpsichord C4 sounded C3).

### Fix (`scripts/generate-sf2-samples.js`)

- New `TRUE_PITCH` table: per (instrument dir, requested key) → the ACTUAL MIDI
  pitch of the raw SF2 sample, measured from the extracted audio
  (`.scratch/probe-root-pitch.js` + FFT peak cross-checks, 2026-09-04) — e.g.
  "38(L)" really is D2, "XP50 G2L" really G2, the Rhodes "c2-90" layer is a C3
  recording used by keys 36..53, the DX7 "C1 Soft" sample is a C2 recording
  used by key 36, "Clavinet C3" is a C2 recording.
- The generator now transposes the raw sample to the LABELED note
  (`shiftSemis = key - trueMidi`), exactly as an SF2 player would transpose by
  key offset — the tab WAV now sounds AT the label. Manifest records
  `sourcePitch` / `shiftSemis` / `transposed`. Keys outside the table fall back
  to a spectral estimator (weak fundamentals can lock an octave up — noted).

### Verification

- `.scratch/probe-sample-pitch.js` (harmonic-sum F0, robust to octave locks) on
  the regenerated `web/generated/samples`: ALL 40 manifest WAVs measure within
  ±0.35 st of their labels (was up to ±24 st off on some tabs).
- `.scratch/pitch-audit.js` (autocorrelation) misreads sine-like samples
  (EP1/EP2/Flute/Harpsichord upper notes lock an octave DOWN — pure tones
  correlate at 2× period) and flagged 7 stale leftover WAVs (`E4.wav` × 6,
  `Flute/G5.wav`) from an older note list — deleted from web/generated + dist.
  Directory audit is now clean; the manifest is the source of truth.
- dist rebuilt (2026-09-04 20:29 + 21:11) and byte-identical to
  web/generated/ (md5 checked); sample tabs + note-test buttons unchanged.
## 2026-09-13 — Per-track volume/mute + «последняя нота» с яркостью по громкости (Update 76)

User: «1. слайдер громкости для каждой дорожки MIDI файла и возможность её
замьютить, не меняя громкость (анмьют возвращает как было). 2. прямоугольник,
в котором будет отображаться последняя проигрываемая нота инструмента … и её
яркость фона будет меняться в зависимости от текущей абсолютной громкости
ноты (с учётом громкости дорожки, инструмента и его огибающей)… зелёным фон
разной яркости и белым название ноты».

### C++ (intrasynth)

- `MidiSynth`: массивы `mChannelGain[16]` (1.0) и `mChannelMeters[16][4]`
  (rms, peak, последняя нота, гейн; нота = −1). Публичные `SetChannelGain`,
  `SetChannelMetering(mask)`, `GetChannelMeters(dst)`.
- Мьют (gain ≤ 0): в `OnNoteOn` ноты мьютенного канала игнорируются; уже
  играющие голоса канала получают NoteRelease и удаляются из карты — нота без
  NoteOff не копится вечно. Метры канала обнуляются (в JS это выглядит как «—»).
- Измеряемые каналы (`mChannelMeteringMask`) и каналы с гейном ≠ 1 рендерятся
  отдельным «tap-проходом»: голоса пишутся в изолированный буфер, измеряются
  (RMS/пик стерео), масштабируются гейном дорожки и только потом подмешиваются
  в общий фрейм — точная громкость «с учётом дорожки, инструмента и огибающей»
  для всех типов слоёв (wave/wavetable/noise/generic/пиано). Остальные каналы —
  прежний путь без изменений. Измерение можно выключать (`mask=0`), тогда
  tap-проход есть только у каналов с гейном ≠ 1.
- Проверено node-пробой (`.scratch/meter-probe.mjs`): гейн 0.25 → rms ровно
  ×0.25; мьют → rms 0 и голосов нет; tap on/off меняет выход не более чем на
  3e-8 (−150 дБFS, последний ulp сложения float — ниже 16-битного квантования;
  бит-в-бит недостижим без измерения внутри общего микса).
- Экспорты: `_SourceSetChannelGain`, `_SourceSetChannelMetering`,
  `_SourceGetChannelMeters` (EmscriptenInterface.cpp) + CMake
  `EXPORTED_FUNCTIONS`.
- Клик-детектор `.scratch/click-lr-any.mjs`: 74:60/72, 43:60, 75:60, 115:60 —
  0 кликов (у 74 попадания детектора на onset существовали и до правки,
  проверено на сборке v75).

### JS (web/synth.js + web/index.html)

- Состояние `trackGains[ch]` / `muted[ch]` в JS (состояние C++ теряется при
  пересоздании источника). `createSource()` — единая точка: после неё
  `applyTrackMix(srcPtr)` прогоняет гейн/мьют всех 16 каналов; слайдер и мьют
  дергают `sendChannelMix()` на живом источнике. Все пути пересоздания
  (перемотка, стоп, A/B, полная генерация) идут через `createSource` —
  переопределения не теряются.
- В строку дорожки: `<input type=range>` 0…1.5 (шаг 0.01) + подпись «N%»,
  кнопка «M». Мьют не меняет ползунок; анмьют возвращает прежний гейн.
  Ползунок в 0 снимает мьют. Активный мьют — класс `track-muted` (полузатемнённая
  строка).
- Прямоугольник `.track-note`: имя последней ноты канала (C4-формат), фон —
  зелёный с яркостью `--lum` по RMS канала (лог-шкала −40…−6 дБFS; во время
  ноты 0.25+0.75·t, в паузе между нотами имя остаётся приглушённо 0.12, до
  первой ноты — «—» 0.06). Опрос `SourceGetChannelMeters` rAF-циклом
  `pollTrackMeters`: работает только когда панель видима и идёт
  воспроизведение; при паузе/скрытии измерение в C++ выключается (`mask=0`),
  цикл останавливается.
- Маска измерения — только каналы с дорожками файла (живая клавиатура не
  мерится). CSS: `.track-row` grid, `.track-note` (var(--lum) → фон),
  `.track-vol`, `.track-mute`, `.track-muted`.

### Verification

- `node .scratch/meter-probe.mjs` — 3/3 (гейн линейность, мьют, tap-инвариант).
- `node .scratch/ui-check76.mjs` (playwright, демо-MIDI из 2 дорожек):
  элементы строки 2/2/2/2; при воспроизведении прямоугольники показывают D4/G4
  с яркостью; мьют гасит прямоугольник и звук, `aria-pressed`; анмьют
  возвращает громкость; ошибок страницы нет.
- `node .scratch/ui-check74.mjs` — регрессия прежних панелей: вкладки сырых
  семплов 12/12, возврат на вкладку blob n/n, A/B 21/21 плееров, ошибок нет.
- `scripts/smoke-test-wasm.mjs` — PASSED (0 non-finite).
- dist пересобран, `web/generated` = `dist` (md5 wasm `ca56f088…`).

### WASM

Канон (plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`): **IntraSynth.wasm
216 856 байт**, IntraSynth.js 14 367 байт. Рост +2 842 байта к v75
(214 037) — tap-проход и метры.

### Откаты

`.scratch/IL-v76-pre-meter.bak` (InstrumentLibrary.cpp не менялся в 76 —
бэкап MidiSynth: `.scratch/MidiSynth-v76.bak` нет: правки точечные, откат —
git diff). Известное ограничение: при мьюте прямоугольник показывает «—»
(канал убит целиком, включая память о ноте) — выбрано намеренно, чтобы
мьют читался мгновенно.

## 2026-09-14 — CC7-громкость дорожек, фидбек MIDI, мгновенная перемотка (Update 77)

User: «Слишком громоздко в 2 строки. Надо всё в 1 строку вместить… Вместо
буквы М лучше слева поставить просто чекбокс… И надо показывать MIDI
громкость, а изменение громкости из UI должно показывать и посылать MIDI
событие изменения громкости. Так что откатывай изменения API, у нас уже всё
для этого было… Если в процессе файл меняет громкость дорожки, это должно
отражаться в UI — собирать MIDI события колбеком по подписке и слать в
браузер. При оффлайн рендере во время проигрывания все элементы управления
должны быть неактивными. Почини перемотку, чтобы она всегда работала и
работала мгновенно». Дополнение: события фиксировать в синтезаторе (не в
парсере), UI-фишки почти бесплатны для WASM (допустимый рост ~200 байт),
большая часть логики — в UI-коде.

### C++ (в синтезаторе, парсер фактически не тронут)

- Точка фиксации канальных событий — MidiSynth: new IDevice-хуки
  OnChannelControlChange/OnProgramChange (Messages.h, вызовы из
  TrackParser.cpp — по 2 строки, как у уже существовавшего NoteOn),
  живой ввод идёт через ту же точку в SendMidiEvent.
- CC7 канала хранится в mLiveVolume[16] и используется в NoteOn.Volume —
  отдельный гейн-механизм Update 76 удалён полностью (откат по ТЗ).
- Кольцо фидбека MidiSynth::PushFeedback (256×3 байта, перезапись старых) —
  NoteOn (0x90), CC7 (0xB0/07), ProgramChange (0xC0). Экспорт
  SourceDrainMidiFeedback(src, ptr, cap) отдаёт события браузеру.
  Де-факто «колбек по подписке»: JS осушает кольцо раз в кадр.
- SourceFastForward(src, samples): мгновенная перемотка вперёд без звука
  (обработка событий без рендера); назад — пересоздание источника +
  FastForward до позиции. Аудио-инвариант: выход источника не меняется.
- Тап-метринг Update 76 (отдельный проход рендера голосов) удалён —
  индикаторы нот теперь из NoteOn-событий кольца: ~3 стора на событие,
  нулевая цена на сэмпл.

### JS (web/synth.js + index.html)

- Строка дорожки в 1 линию: чекбокс-мьют → компактный слайдер CC7
  (0..127, значение под ним) → имя дорожки → индикатор ноты, приклеенный
  слева к комбобоксу инструмента. Порог переноса на 2 строки уменьшен вдвое.
- Громкость дорожки = MIDI CC7: слайдер шлёт SourceSendMidiEvent CC7
  в реальном времени; начальные значения — из парсинга CC7 в SMF (JS);
  чекбокс-мьют шлёт CC7=0 и запоминает прежнее значение, анмьют возвращает.
- Фидбек: файл сам сменил CC7/инструмент → слайдер/чекбокс/комбобокс
  обновляются из кольца; во время генерации события копятся в очередь и
  применяются после (pregen-проигрывание тоже живое, индикаторы из очереди).
- Контролы дорожек (и seek) блокируются при генерации и при pregen-проигрывании.
- Перемотка: вперёд — SourceFastForward (мгновенно), назад — пересоздание;
  seek работает и во время генерации (позиция pregen-буфера).
- Фикс «прочерков после нескольких секунд»: виды WASM-кучи пересоздаются,
  если куча выросла и отвязала ArrayBuffer (ensureMetersViews).
- Фикс затирания начального CC7 файла: applyTrackMix шлёт CC7 только для
  каналов с известным JS-состоянием (иначе дефолтные 127 возвращались
  фидбеком и перезаписывали громкость из файла).

### Проверка

- node-проба (.scratch/cc7-probe.mjs): CC7 из файла масштабирует ноту
  (127 vs 20 → ×6.2 по RMS), фидбек отдаёт CC7 обоих каналов + NoteOn,
  живой CC7 и живой NoteOn в кольце, FastForward отматывает источник
  без потери работоспособности — все ОК.
- Playwright (.scratch/ui-check77b.mjs, 17 проверок): все ОК — начальный
  CC7 из файла (100/30) до воспроизведения, чекбокс-мьют, индикатор ноты
  с живой яркостью и без «смерти» со временем, эхо собственного CC7 не
  глушит дорожку, файловый CC7=0 гасит чекбокс, анмьют возвращает,
  мгновенная перемотка, блокировка контролов при генерации и pregen,
  индикатор жив из очереди фидбека, ошибок страницы нет.
- Регрессия: smoke-test-wasm PASSED; клик-детектор 74/43/75/115 — 0 кликов;
  вкладки сырых семплов и A/B — без изменений (21/21, 304-кэш работает).
- ui-check76 частично устарел намеренно: он проверяет семантику удалённого
  tap-метринга Update 76 (мьют гасит прямоугольник и т.п.) — заменён
  ui-check77b.

### Размер

WASM (канон: plain -Os, INTRA_PIANO_ALL_TABLES=ON) = 215 009 байт
(было 216 856 в Update 76, 214 014 до UI-изменений: тап-метринг -1 847,
фидбек/CC7/FastForward +995 — в пределах запрошенного «~200 байт на фичу»,
итог даже меньше исходной версии).

## 2026-09-14 — Мьют дорожек = маска каналов, а не CC7 (Update 78)

User: «Я выключил все галочки, а музыка всё равно играет. Проверял на
tous les garçons.»

### Причина (замер, не догадка)

Файл сам присылает CC7: у `touslesgarcons.mid` это 7 событий CC7 в самом
начале (85/100/88/127/95/100/127 по каналам 0-5 и 9). Мьют в Update 77 был
сделан через CC7=0 — то есть через ту же MIDI-величину, которой управляет сам
файл, и файловый CC7 его отменял. Отсюда «музыка играет при снятых галочках».

### Решение

Мьют перестал быть громкостью и стал отдельным слоем микшера:

- C++ `MidiSynth::SetChannelMuteMask(ushort)` — битовая маска каналов
  (1 = замьючен). Новомьюченные каналы сразу гасят звучащие ноты
  (`OnAllNotesOff`), ноты замьюченного канала не создаются вовсе (нулевая
  цена), фидбек NoteOn для индикаторов по-прежнему уходит.
- Громкость канала осталась чистой MIDI-величиной CC7 (`mLiveVolume`) —
  её меняет и файл, и ползунок UI, мьют её не трогает («анмьют возвращает
  как было» — теперь буквально: значение не менялось).
- JS: чекбокс ставит бит маски (`SourceSetChannelMute`, `applyTrackMix` —
  единая точка для всех пересозданий источника). Убраны мьют через CC7=0,
  CC123-хак, `preMuteCC7` и «CC7=0 из файла = мьют»: файловый CC7 теперь
  только обновляет слайдер (в т.ч. у замьюченной дорожки), но не включает и
  не снимает галочку. Подпись под слайдером всегда показывает MIDI-громкость.

### Проверка

- node-проба `.scratch/mute-mask-probe.mjs`: файл с собственным CC7=100 —
  мьют глушит (peak 0.00000 против 0.110), CC7 канала остаётся 100 (в фидбеке
  тоже), мьют всех 16 каналов = абсолютная тишина, после анмьюта ноты звучат.
- Playwright `.scratch/ui-check78.mjs` (12/12): громкости файла в слайдерах
  после мьюта не изменились, все галочки сняты → тишина по замеру выходного
  сигнала (`outputStats`), файловый CC7 не включает галочки обратно, анмьют
  возвращает звук и ту же громкость, блокировка контролов при оффлайн-рендере
  и перемотка — без изменений.
- Playwright на реальном файле `.scratch/ui78-tous.mjs` (7/7): tous les
  garçons, 7 дорожек — без мьюта peak 0.566, все галочки сняты → 0.000000,
  громкости 85/100/88/127/95/100/127 сохранились, одна возвращённая дорожка
  снова звучит (0.260).
- Регрессии: `smoke-test-wasm` PASSED, проба CC7/фидбека/FastForward (6/6),
  A/B-панель 21/21 + кэш 304, вкладки сырых семплов, клик-детектор на
  43/74/75/115 — 0 кликов. Прогнаны также попутно исправленные предупреждения
  компилятора (`-Wunused-parameter` в `IDevice`-заглушках, Update 77).

WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = 214 995 байт
(было 215 009 в Update 77: маска каналов вместо мьюта через CC7 плюс единый
PushFeedback — −14 байт).

## 2026-09-14 — Яркость = огибающая звучащей ноты; компактная строка (Update 79)

User: «Слайдер громкости можно покомпактнее, число текущей громкости ближе к
слайдеру. Имя дорожки более мелким шрифтом, да и имя инструмента тоже. И
уменьшить ещё порог на четверть, когда каждая дорожка в 2 строки переходит. И
мне кажется, это просто анимация с зелёным фоном? Яркость должна отображать
именно громкость играющей ноты с учётом её огибающей… Можно обновлять раз в
200 мс и по событию отпускания. Но хотелось бы, чтобы это было дёшево в плане
размера WASM или спрятать за #if, чтобы минимальная сборка этого не тянула.
Полностью завершённые ноты не должны висеть, а должны заменяться на прочерки».

### C++ (уровни нот для индикаторов, за новой опцией сборки)

- Новая опция CMake `INTRA_UI_METERS` (по умолчанию OFF) → `-DINTRA_UI_METERS`.
  Канонический WASM собирается с ней ON (`scripts/build-wasm.sh`), минимальные
  сборки — как раньше, **без единого байта** этой функциональности: под #if
  лежат и виртуальная функция, и её реализации, и тело экспорта.
- `Sampler::GetLevel()` — уровень огибающей ноты 0..1 (по умолчанию 1: семплеры
  без огибающей звучат ровно). Реализации: `NoteSampler` (своя ADSR × самая
  громкая огибающая тела — у флейты атака и релиз живут именно в огибающей
  волновой таблицы, верхней ADSR у неё нет) и `WaveTableSampler` (собственная
  огибающая, покрывает и WaveFormSampler).
- `MidiSynth::GetChannelNoteLevels(byte* dst)` — 16 байт, на канал уровень самой
  громкой звучащей ноты (0 = канал молчит). Номер ноты не отдаётся: его UI знает
  из кольца фидбека. Экспорт `SourceGetNoteLevels`. Расходов на семпл нет —
  запрос редкий (см. ниже), обход списка голосов один раз за запрос.
- `OnNoteOff` теперь тоже кладёт событие (0x80) в кольцо фидбека: UI обновляет
  яркость сразу по отпусканию, не дожидаясь опроса.
- Парсер файла не тронут.

### JS: яркость идёт за огибающей, а не за таймером

- Состояние индикатора: `{ note, vel, level, live }`. Яркость = уровень
  огибающей (из синтезатора) × velocity ноты × CC7 дорожки; мьют даёт 0.
- Уровни спрашиваются раз в **200 мс** и **сразу по событию отпускания**
  (`levelsDirty` из кольца). Между опросами яркость держится — прежняя схема
  (затухание на каждом кадре) гасила прямоугольник за ~0.6 с независимо от звука,
  это и было «просто анимацией зелёного фона».
- Полностью завершённая нота → «—» и фон тишины (0.06); раньше имя висело вечно.
- Когда живого источника нет (офлайн-рендер в pregen-проигрывании; там
  источника в WASM уже не существует) — мягкий fallback: яркость вспышки
  затухает, и по затухании тоже ставится прочерк. В A/B-сборке без экспорта
  уровней (`typeof Module._SourceGetNoteLevels !== "function"`) поведение то же —
  страница не падает.

### Раскладка строки дорожки

- Слайдер 58 px (было 74) при высоте 14 px, значение — вплотную под ним (зазор
  0, `margin-top: -1px`), имя дорожки 0.72rem, имя инструмента 0.78rem с
  padding 5/8, отступ строки 3/8, gap 6.
- Порог перехода в 2 строки уменьшен ещё на четверть: `@media (min-width: 420px)`
  (было 560).

### Проверка

- node-проба `.scratch/levels-probe.mjs` (10/10): тишина → все каналы 0; флейта
  после NoteOn растёт 0 → 21 → 74 → 127 (шаг 5 мс); сустейн держится; NoteOff
  приходит в кольцо; после отпускания уровень падает и завершённая нота даёт 0;
  замьюченный канал → 0, анмьют снова звучит.
- Playwright `.scratch/ui-check79.mjs` (19/19): строка в одну линию, высота 38 px,
  слайдер 58 px, значение вплотную (−1 px), шрифты 11.5/12.5 px, индикатор
  приклеен к комбобоксу, на 400 px строка переносится; до игры «—»; во время
  ноты «C4» с яркостью 0.78; **яркость не гаснет сама в сустейне** (поздняя
  0.78 против ранней 0.78 — этим и отличается от прежней анимации); после
  отпускания нота сменяется прочерком, фон 0.06; мьют → прочерк, анмьют не
  висит; перемотка не оставляет зависшей ноты; ошибок страницы нет.
- Регрессии: `smoke-test-wasm` PASSED, проба CC7/фидбека/FastForward, проба
  мьюта маской (4/4), A/B-панель 21/21 + кэш 304, вкладки сырых семплов,
  клик-детектор 43/74/75/115 — 0 кликов.
- `.scratch/mute-probe.mjs` намеренно устарел: он проверяет мьют через CC7=0 —
  семантику, удалённую в Update 78 (её заменила маска каналов, проба
  `mute-mask-probe.mjs`).

### Размер

WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = **215 582 байта**
(было 214 995 в Update 78, +587). Из них: сама функция уровней (виртуальная
`GetLevel` в базовом классе семплеров, её реализации, обход голосов, экспорт)
= **474 байта** — ровно эти байты и уходят под `INTRA_UI_METERS`; сборка с
`-DINTRA_UI_METERS=OFF` даёт 215 108 байт (тот же код без неё) и не тянет
ничего из этой функциональности. Остальные ~113 байт — событие отпускания в
кольце фидбека и заглушка экспорта, они нужны всегда.

## 2026-09-14 — Яркость без скачков, пауза гасит, офлайн-рендер без индикаторов (Update 80)

User: «Для оффлайн рендера вообще эти все фичи не нужны. Почему-то ноты иногда
залипают и висят яркими десятки секунд, пока их не заменит другая. И мне
кажется, что яркость как-то уменьшается скачкообразно и часто не гаснет. После
note off она что ли теряется? И наверное опрос стоит участить до 100 мс и
сделать интерполяцию в UI.»

### Что было не так (измерено, не догадка)

1. **Подскок яркости назад к максимуму.** `NoteSampler::GetLevel()` при
   отсутствии измеримых слоёв возвращал 1.0 («огибающая неизвестна, считаем
   ровной»). Трасса уровней с шагом 10 мс (`.scratch/level-trace.mjs`) показала
   вспышку на полную яркость **после** релиза: Ocarina `…1,0,127,0`, PanFlute
   `…0,127,127,127,127,127,127,127,0` (70 мс на полной яркости), Recorder
   `…1,0,127,0`. Причина — сэмплер ноты, у которого тело уже отзвучало, но
   остались неизмеримые слои (у PanFlute — generic-слой выдоха).
2. **«Залипают и висят яркими».** Здесь два независимых механизма: (а) та же
   вспышка 1.0; (б) rAF-цикл метров останавливался на паузе и стопе
   (`metersStop()`), поэтому прямоугольники замирали на последней яркости и
   жили до следующего NoteOn в том же канале. Синтезатор тут не виноват: проба
   `.scratch/stuck-probe.mjs` (все 128 GM-инструментов, 20 с тишины после
   ноты) не нашла ни одного зависшего уровня.
3. **Скачкообразная яркость.** Уровень спрашивался раз в 200 мс и присваивался
   напрямую — шаги опроса были видны как ступеньки, а на отпускании яркость
   «пропадала» между опросами.

### Что сделано

- **C++** (`NoteSampler`, под `#ifdef INTRA_UI_METERS`): у пустого сэмплера
  (`Empty()`) и у ноты, у которой измеримое тело уже было (`mBodySeen`),
  уровень теперь 0 — индикатор гаснет, а не вспыхивает. Инструменты, у которых
  измеримого тела нет вовсе (ударные, шум, generic-семплеры), по-прежнему
  показывают ровный уровень: их поведение не изменилось. Огибающая,
  доработавшая до конца (`AdsrAttenuator::Done`), тоже даёт 0 — раньше
  `Active == false` после релиза читалось как «огибающей нет».
- **JS** (`web/synth.js`): опрос уровней **раз в 100 мс** (было 200) и
  **интерполяция** показанной яркости к цели в rAF (постоянная 55 мс), поэтому
  ступенек нет; пауза/стоп/перемотка больше не останавливают цикл, а переводят
  цель в 0 — индикаторы гаснут сами; номер ноты при гашении не забывается, так
  что после пауза-возобновления та же нота возвращается без нового NoteOn;
  **офлайн-рендер (полная генерация и проигрывание готового буфера) фич
  индикаторов не использует вовсе** — события фидбека осушаются и
  выбрасываются, уровни не опрашиваются, прямоугольники гаснут (очередь
  `pendingFeedback` удалена как ненужная).
- В `window.__synthDebug` добавлен `noteGlowState()` для автотестов панели
  (цель/показанный уровень/состояние цикла) — это JS-хук, размер WASM не трогает.

### Проверка

- `.scratch/level-trace.mjs` (шаг 10 мс, 5 инструментов): подскока яркости нет
  ни у одного (шаг вверх = 0), после отпускания уровень уходит в 0.
- `.scratch/level-shape.mjs`: форма уровня по 50 мс — удержание ровное,
  отпускание гаснет.
- `.scratch/drum-probe.mjs`: ударные по-прежнему показывают уровень, пока
  звучат (поведение не изменилось).
- Playwright `.scratch/ui-check80.mjs` (11/11): до игры «—»; после Play «C4»,
  рост яркости занимает ~230 мс (интерполяция, а не скачок), максимальный шаг
  0.33 (мгновенного прыжка нет); сустейн держит яркость 0.82; **пауза → все
  индикаторы погасли**, после возобновления нота снова видна; стоп → погасли;
  во время генерации и при проигрывании офлайн-рендера индикаторы погашены;
  ошибок страницы нет.
- Регрессии: `smoke-test-wasm` PASSED; проба CC7/фидбека/FastForward;
  `levels-probe` 10/10; проба мьюта маской 4/4; Playwright по «tous les
  garçons» 7/7 (мьют тишиной, файловые CC7 не включают галочки); A/B-панель
  21/21 + кэш 304; вкладки сырых семплов; клик-детектор 43/115/74/75 — 0 кликов
  (код рендера не тронут: правка C++ живёт только в измерительном API).

### Размер

WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = **215 678 байт**
(было 215 582 в Update 79, **+96**). Сборка с `-DINTRA_UI_METERS=OFF` = 215 108
байт, то есть вся измерительная функциональность по-прежнему целиком живёт под
`INTRA_UI_METERS` и в минимальную сборку не попадает.

### Известное ограничение

У инструментов, тело которых синтезатор не умеет измерять (ударные, шумовые и
generic-слои — например, выдох PanFlute или sustain флейты), яркость означает
«нота звучит», а не её текущую громкость: затухания такой ноты индикатор не
показывает. Чтобы показывать и его, измерительный слой нужно распространить на
generic-семплеры (`Synth`/`AdditiveSampler`/струнные и т.д.) — это уже
заметно дороже по размеру, поэтому в этот апдейт не входило.

## 2026-09-14 — Громкость пиано в индикаторах дорожек (Update 81)

User: «Для piano надо тоже громкость показывать. Я так понимаю, у них вместо
огибающей затухание зашивается в партиалы, поэтому это не отражается. Если
основная партиала всегда самая громкая, можно брать только её громкость
наверное.»

### Причина (подтверждена замером)

У аддитивного пиано (`AdditiveSampler`) общей огибающей нет: каждый партиал
затухает СВОИМ шагом (`mDecay1..4` из таблицы, измерены из семплов
Clavinova/CP-80/Rhodes). `NoteSampler::GetLevel` умел измерять только
волновые таблицы и формы, generic-слои (а пиано — это generic) не измерялись
вовсе, поэтому возвращался ровный `adsr = 1` («огибающей нет»): прямоугольник
дорожки горел постоянным максимумом до самого конца ноты, сколько бы нота ни
затухала. Именно это и было видно в UI.

### Что сделано

- `IGenericSampler::GetLevel()` (под `#ifdef INTRA_UI_METERS`, по умолчанию
  **-1** = «уровень не измеряю»): ударные, шумовые и физические модели
  отвечают -1 и ведут себя ровно как раньше.
- `NoteSampler::GetLevel()` — в максимум по телу добавлены generic-слои
  (значение -1 на результат не влияет). Опрос тот же: ~100 мс +
  событие отпускания, интерполяция яркости в UI (Update 80).
- `AdditiveSampler::GetLevel()` — максимум огибающей по лейнам, то есть
  уровень самой громкой гармоники: в атаке и большей части сустейна это
  фундаментал, он и задаёт воспринимаемую громкость (предложение владельца).
  Плюс фейд последних 20 мс конца региона, чтобы индикатор не обрывался
  скачком. Уровень относительный (velocity и CC7 дорожки домножает UI).

Пиано-семейство целиком аддитивное, поэтому покрыты Acoustic Grand, Bright,
Electric Grand, Honky-Tonk, Electric Piano 1/2, Harpsichord и Clavinet.

### Проверка

- `.scratch/piano-level-probe.mjs` (18/18): уровень растёт на NoteOn и
  затухает монотонно (AGP: 127 → 85 @0.5 с → 45 @1.5 с → 22 @3 с), после
  отпускания падает и завершённая нота даёт 0; все семь пиано-пресетов
  затухают; ударные по-прежнему дают ровный ненулевой уровень (поведение не
  изменилось); флейта (волновая таблица) не сломана.
- Playwright `.scratch/ui-check81.mjs` (7/7): в браузере яркость пиано
  реально падает (0.63 @0.7 с → 0.34 @2 с → 0.20 @4 с, монотонно после
  пика), нота остаётся видимой пока звучит, стоп → прочерк; пан-флейта
  сустейн держит как раньше.
- `.scratch/audio-ident81.mjs`: рендер микса (пиано ×2, флейта, пан-флейта,
  рекордер, вибрафон, барабаны, 6 с) одной и той же сборкой с
  `-DINTRA_UI_METERS=ON` и `OFF` — **sha256 совпал, 0 отличий байт**:
  измерительный код (включая новую ветку пиано) не меняет ни одного сэмпла.
- Регрессии: `smoke-test-wasm` PASSED; `levels-probe` 10/10; проба
  CC7/фидбека/FastForward; проба мьюта маской; `ui-check80` 11/11; вкладки
  сырых семплов; A/B-панель 21/21 + кэш 304; клик-детектор 43/75/115/74 —
  без изменений (код рендера не тронут).

### Размер

WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`, `INTRA_UI_METERS=ON`) =
**215 940 байт** (было 215 678 в Update 80, **+262**). Сборка с
`-DINTRA_UI_METERS=OFF` = **215 108 байт** — байт-в-байт как до правки, то
есть в минимальной сборке новая функциональность не занимает ничего.

### Остаётся

Яркость по-прежнему «нота звучит» (а не уровень) у ударных, шума и
физических моделей — они уровня не отдают принципиально (у них нет
огибающей, которую можно спросить). Струнные Karplus-Strong и
Gaussian/Spectral-модели могли бы отдавать свой уровень так же, как пиано,
если это понадобится.

## 2026-09-14 — Живой слой громкости канала (CC7/мьют), перемотка без «взрыва» (Update 82)

User: «А нужна ли вообще эта виртуальность? Может каждый инструмент будет просто
в поле своего абстрактного родителя класть текущую громкость каждый фрейм, а он
её просто отдавать? Это уменьшит размер? Если КС ещё не реализован, там тоже надо
сделать. Ну и в остальном можно расследовать, может есть другие примитивные
виртуальные методы, которые можно. Кстати, заметил баг синтезатора. Если дорожка
изначально нулевой громкости, то нота не создаётся, и потом когда дорожка
становится громче, её не слышно. А нота может быть очень длинная. Перемотка
странно работает, каждый раз оглушающий взрыв какой-то. Она что ли разом
воспроизводит все ноты, которые должна была пропустить?»

### 1. Виртуальность против поля — вопрос закрыт замером

Сделал оба варианта одного и того же (уровень ноты для индикаторов) и измерил
канонической сборкой:

| вариант | канон (`-Os`, `INTRA_UI_METERS=ON`) | `INTRA_UI_METERS=OFF` |
| --- | --- | --- |
| как было (виртуальные `GetLevel`) | 216 997 | 216 014 |
| поля (`Level`, `EnvelopeRender`, записи из рендера) | 217 072 | 216 032 |

**Поля дороже на 75 байт.** Слот в таблице виртуалов стоит 4 байта на класс, а
поле требует записи уровня из горячего цикла рендера — у струн эти циклы
шаблонные и инстанцируются по нескольку раз на класс. Поэтому виртуальные
геттеры оставлены (в `Sampler::GetLevel` и `IGenericSampler::GetLevel` — заметка
с замером, чтобы вопрос не поднимался заново). Решение записано в
`docs/decisions/active/20260914-SamplerLevelGetterVsField.md`.

Остальные «примитивные» виртуальные методы трогать не стали по той же причине:
`MultiplyVolume`/`MultiplyPitch`/`SetPan`/`SetRenderParams`/`NoteRelease` — это
команды с разными реализациями (поле их не заменяет), а `SupportsEnvelopeRender`
заменён на поле и вернулся обратно в составе того же замера (экономии нет).

### 2. Баг: нота, рождённая на нулевой громкости дорожки, молчала навсегда

CC7 канала запекался прямо в стартовую громкость ноты (`totalStartVolume`), а
`sampler * 0` мёртв: после подъёма CC7 умножать было нечего — длинная нота не
звучала. Теперь громкость канала — **живой слой поверх тела ноты**:

- в `Sampler` появились `ChannelGain` (1 — как родилась, 0 — мьют, иначе
  CC7/CC7-при-рождении) и `BornCC7` (CC7 в момент рождения, не менее 1: нота
  всегда рождается живой);
- доля канала по-прежнему запекается в стартовую громкость (тембр не меняется —
  проверено бит-в-бит), но не ниже 1/127, а текущую громкость несёт `ChannelGain`;
- `OnChannelControlChange` (CC7) и `SetChannelMuteMask` пересчитывают
  `ChannelGain` у **звучащих** нот канала — как в MIDI: смена CC7 и мьют слышны
  на доигрывающей ноте;
- множитель применяет сама нота в `NoteSamplerTask`: при `ChannelGain == 1`
  (обычный случай — ничего не менялось) это прежний путь без лишних проходов,
  иначе нота рендерится в отдельный буфер кадра (`SamplerTaskContext::ScratchL/R`,
  буферы кадра общие для всех нот) и в микс попадает домноженной. При нуле
  сигнал в микс не идёт, но огибающие идут — голос завершается сам, а не
  копится, и оживает при снятии мьюта.

Побочный эффект (осознанный): ноты замьюченной дорожки теперь создаются и
считаются (как если бы играли) — это цена того, что снятие мьюта возвращает
длинную ноту в том же месте огибающей. Раньше мьют экономил CPU, но «съедал»
ноты.

### 3. Баг: перемотка «выстреливала» пропущенными нотами

`FastForward` прогонял события пропущенного участка через обычный `OnNoteOn`, а
эти ноты стартуют не в своих файловых временах, а сразу на целевой позиции —
все разом («оглушающий взрыв»). Теперь на время прогона стоит `mSkippingEvents`:
состояние каналов (CC7, инструмент, педаль) читается, а ноты не создаются, и
фидбек NoteOn/NoteOff в кольцо не пишется (чтобы UI не подсвечивал пропущенное).
Итог: после перемотки звучат ноты, начинающиеся ПОСЛЕ цели, а не весь пропущенный
участок.

### 4. Уровень струн (Karplus-Strong) — «КС ещё не реализован»

Раньше струнные не отдавали уровень (по умолчанию `-1` = «не измеряю»), и
индикатор дорожки горел ровно. Теперь `KarplusStrongSampler`,
`SpectralStringSampler` и `GaussianStringSampler` возвращают своё затухание
(`mVolume` относительно стартовой громкости): у гитар индикатор идёт за
затуханием струны. Ударные и шум по-прежнему «нота звучит» — им мерить нечего.

### Проверка

- `.scratch/update82-probe.mjs` (11/11): нота при рождении с CC7=0 молчит,
  оживает при подъёме и продолжает звучать; мьют до рождения ноты глушит, снятие
  мьюта возвращает её; CC7 на звучащей ноте меняет громкость сразу (отношение
  3.97 при ожидаемых 4.0); после перемотки нет ни всплеска, ни клиппинга, а ноты
  после цели звучат; струна отдаёт уровень и гаснет (118 → 65 → 31 → 0).
- Playwright `.scratch/ui-check82.mjs` (8/8): перемотка на плотном файле — пик
  после неё 0.139 против 0.455 при обычном воспроизведении (взрыва нет), 0
  клиппингов, музыка продолжается; слайдер громкости дорожки в нуль глушит
  звучащую ноту (пик 0.00000), подъём возвращает **ту же** ноту (rms 0.0694 →
  0.0692), индикатор показывает ноту.
- `.scratch/audio-ident82.mjs`: рендер 9 инструментов (флейты 43/115, рекордер,
  пан-флейта, пиано, гитара, скрипка, барабаны) новой сборкой, сборкой с
  `INTRA_UI_METERS=OFF` и **сборкой до правок** (Update 81,
  `dist/IntraSynth.wasm`, md5 `3cf05d52…`) — **все хэши совпали**: там, где нет
  изменений CC7/мьюта, звук не изменился ни на байт (живой слой включается только
  при `ChannelGain != 1`).
- Регрессии: `smoke-test-wasm` PASSED; `levels-probe` (обновлено под новую
  семантику мьюта: нота жива, яркость обнуляет UI) — ВСЁ ПРОШЛО;
  `piano-level-probe` ВСЁ ПРОШЛО; проба мьюта маской — ВСЕ ПРОВЕРКИ;
  `cc7-probe` — ВСЕ ПРОВЕРКИ; `ui-check80` 11/11; `ui-check81` 7/7;
  `ui78-tous` на реальном файле 7/7 (мьют = тишина, файловые CC7 сохранены);
  A/B-рендеры пересобраны; клик-детектор 43/74/75/115 — как до правок (числа
  совпадают, рендер не тронут).

### Размер

WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`, `INTRA_UI_METERS=ON`) =
**216 997 байт** (было 215 940 в Update 81, **+1057**). Из них:
- живой слой громкости канала + фикс перемотки (видно и в OFF-сборке) — +906;
- уровень струн (новое в измерительной части) + остальное в ON — +151;
- вариант «поля вместо виртуальных геттеров» дал бы ещё +75 (откатан).

Сборка с `-DINTRA_UI_METERS=OFF` = **216 014 байт** (`-DINTRA_UI_METERS`-`ON`
разница 983 — вся измерительная функциональность по-прежнему целиком под флагом).
`web/generated/` и `dist/` синхронизированы.

### Остаётся (закрыто в Update 83)

- ~~После перемотки ноты, начавшиеся ДО цели, не досоздаются~~ — сделано в
  Update 83: удерживаемые ноты дозвучивают на целевой позиции.
- Мьют дорожки больше не экономит CPU (ноты живут) — цена за оживление ноты при
  снятии мьюта.

## 2026-09-14 — Дозвучивание удерживаемых нот после перемотки (Update 83)

User: «Продолжи» — это продолжение пункта «Остаётся» из Update 82: после
мгновенной перемотки должны дозвучивать удерживаемые ноты (пады, начавшиеся за
минуты до цели), при этом «взрыва» быть не должно.

### Что сделано (C++, только `MidiSynth`; JS и парсер не тронуты)

При перемотке (`FastForward`, флаг `mSkippingEvents`) теперь различаются **две
судьбы ноты**, и обе соответствуют тому, что было бы при рендере участка:

1. **Нота звучала ДО перемотки.** Её голос уже существует, поэтому он
   сохраняется как есть — с ТОЧНОЙ позицией огибающей (никакой перезапуска
   атаки). Если в пропущенном участке пришёл её NoteOff, голос гасится прямо
   во время прогона событий (`NoteRelease`), а с педалью — уходит в
   `SustainHold` и отпускается снятием педали: это ровно та же логика, что и в
   обычном воспроизведении, только без рендера. Раньше на этом месте стоял
   `OnAllNotesOffAllChannels()` — он убивал вообще все голоса, включая живые
   пады.
2. **Нота РОДИЛАСЬ в пропущенном участке и удерживается на цели.** Голоса у неё
   нет, поэтому она запоминается (`mSkipHeld`: канал, нота, velocity,
   инструмент с учётом переопределения UI, признак «держится педалью») и
   создаётся настоящим `OnNoteOn` уже на целевой позиции — с фидбеком для
   индикаторов дорожек. Педальные записи при снятии педали снимаются
   (`SkipHeldReleaseSustainedOnChannel`), All Notes Off снимает все записи
   канала, повторный удар той же клавиши обновляет velocity и гасит прежний
   голос, как в обычном MIDI.

Ёмкость записей — 96. Обоснование замером (`.scratch/held-polyphony.mjs`,
`.scratch/held-split.mjs`): максимум одновременно удерживаемых нот по реальным
файлам владельца — 28 (Chopin с педалью), 15-16 (tous les garçons, celine),
14 (Merry Christmas). Массив обнуляемый, то есть живёт в BSS и **в размере
файла не занимает ничего** (проверено `llvm-size -A`: секция DATA не меняется).

Парсер MIDI-файла и веб-UI не менялись вовсе: вся логика — в синтезаторе (там
же, где состояние каналов), в браузер по-прежнему уходят только события кольца
фидбека.

### Формат записей: список, а не таблица «канал × нота» (замерено)

Сделаны и измерены три варианта одного и того же поведения:

| вариант | канон (`-Os`, ALL_TABLES=ON, UI_METERS=ON) | DATA-секция |
| --- | --- | --- |
| список 96×6 байт (принят) | **218 402** | 42 297 |
| список 64×5 байт + общий хелпер снятия | 218 412 | 42 297 |
| таблица `byte[16][128]` состояния и velocity | 218 678 | 42 297 |

Таблица «канал × нота» проигрывает 267 байт кода: обнуляемые массивы бесплатны
в файле, но индексация по двум измерениям и обход 2048 ячеек на каждой
перемотке стоят дороже, чем поиск по плотному списку. Убрать дублирование
поисков голоса через общий хелпер тоже не помогло (+10 байт — инлайн в двух
местах дешевле). Все три варианта зафиксированы в прогоне, чтобы вопрос не
поднимался заново.

### Проверка

- `.scratch/update83-probe.mjs` (17/17):
  - нота, звучавшая до перемотки, после seek продолжает звучать, и её уровень
    совпадает с тем же моментом при обычном воспроизведении (отношение 1.00 —
    огибающая НЕ сброшена на атаку);
  - нота, родившаяся в участке и удерживаемая, дозвучивает после перемотки
    (это и путь «назад»: свежий источник + FastForward);
  - нота, рождённая И отпущенная в участке, не звучит; отпущенная в участке
    (без педали) не атакует заново — доигрывается только её релиз, ниже уровня
    сустейна, и гаснет;
  - педаль: и запись (рождённая в участке под педалью), и живой голос
    дозвучивают до снятия педали, а снятие педали после цели их отпускает;
  - мьют дорожки сильнее дозвучивания: возрождённая нота мьюта не слышна, а
    снятие мьюта её возвращает (значит, она действительно создана);
  - «взрыв»: на сценарии «30 коротких нот в участке + одна длинная» после
    перемотки звучит только удерживаемая нота (пик 0.08 против 0.25 у порога);
  - перемотка «в то же место» ничего не рвёт.
- Громкость после перемотки на реальных файлах (`.scratch/update83-realfile.mjs`,
  сравнение с обычным воспроизведением той же точки): tous les garçons —
  0.777 против 0.798, celine — 0.182 против 0.133, Chopin на 120 c — 0.510
  против 0.157 (короткий транзиент переатакованных педальных нот, см. ниже).
  Клиппинга нет нигде (пик < 1.0).
- Регрессии: `smoke-test-wasm` PASSED; `update82-probe` (проверка 4 обновлена
  намеренно: раньше после перемотки не звучало ничего, теперь удерживаемые ноты
  дозвучивают — инвариант «звучат только они, а не весь участок» сохранён),
  `levels-probe`, `piano-level-probe`, `mute-mask-probe`, `cc7-probe` — все
  прошли; Playwright `ui-check82` 8/8, `ui-check80` 11/11, `ui-check81` 7/7,
  `ui78-tous` на реальном файле 7/7; клик-детектор 43/74/75/115 — 0 кликов
  (код рендера не тронут: правки живут только в путях перемотки).

### Размер

WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`, `INTRA_UI_METERS=ON`) =
**218 402 байта** (было 216 997 в Update 82, **+1 405**). Сборка с
`-DINTRA_UI_METERS=OFF` = **217 419 байт** (было 216 014, тоже +1 405) — то
есть новая функциональность целиком в общей части и под флагом ничего не
прячет; разница ON/OFF осталась прежней, 983 байта. `web/generated/` и `dist/`
синхронизированы (md5 `de3dd32b…`).

### Известные артефакты мгновенной перемотки (осознанные, замерены)

- Ноты, рождённые в пропущенном участке, воссоздаются с НАЧАЛА — их атака
  звучит на целевой позиции, а не в своей фазе огибающей. Для падов и струнных
  это слышно как естественный вдох, для педального пиано — как короткий
  транзиент: на Chopin с 28 нотами под педалью пик первых 0.2 с после
  перемотки доходит до 0.510 против 0.157 при обычном воспроизведении
  (замер выше). Полностью убрать это можно только пред-рендером участка перед
  целью (не мгновенно) или отказом от воссоздания педальных нот — тогда пады,
  удерживаемые педалью, пропадали бы. Оставлено как есть: MIDI-семантика
  (педальная нота звучит) важнее, а транзиент короткий и без клиппинга.
- Ноты, ОТПУЩЕННЫЕ в пропущенном участке, гасятся при перемотке, поэтому их
  релиз доигрывается уже после цели (уровень не выше сустейна, затухает за
  ~0.5 с). Это поведение было и в Update 82 (`OnAllNotesOffAllChannels` гасил
  все звучащие голоса) — новое только то, что удерживаемые голоса больше не
  гасятся вместе с ними.
- Записи ограничены 96 одновременными удержаниями: при переполнении лишние
  ноты просто не дозвучат (в реальных файлах порог не достигается — 28 максимум).

### Откаты

Правки точечные и целиком в `MidiSynth.h`/`MidiSynth.cpp` (+ одна строка
комментария в `EmscriptenInterface.cpp`), откат — `git diff` по этим файлам.
Замеры и пробы: `.scratch/update83-probe.mjs`, `.scratch/update83-realfile.mjs`,
`.scratch/held-polyphony.mjs`, `.scratch/held-split.mjs`; сборка OFF для
сравнения размеров — `.scratch/build-off/`.

## 2026-09-14 — Перемотка «гасим всё и пропускаем события»; кодировка имён дорожек (Update 84)

User: «Как-то сложно и размер сильно вырос. Нельзя было просто убить все
семплеры и мотать? Запускать только ноты, которые идут после перемотки, это норм
для MIDI. Или размер вырос не от этого, а от нот нулевой громкости? Я видел
scratch буфер, зачем он нужен? Чтобы не зашивать 0 в огибающую и потом не
восстанавливать её делением на 0? Кстати, некоторые мелодии выводят кракозябры
как имя дорожки: ñêðèïêè. Может можно как-то распознавать кодировку в UI?»

### 1. Дозвучивание удерживаемых нот (Update 83) откатано целиком

`FastForward` снова простой: **отпустить всё звучащее** (по 16 каналам
`OnAllNotesOff` → `NoteRelease`: релиз доигрывается сам, без щелчка) и
**пропустить события участка**, читая состояние каналов, но не создавая нот
(`mSkippingEvents`). После перемотки играют только ноты, начинающиеся ПОСЛЕ
цели — обычное поведение MIDI-секвенсера. Удалены: `SkipHeldNote`, `mSkipHeld`,
`SKIP_HELD_CAP`, `MaterializeSkipHeldNotes`, `SkipHeldRemove`,
`SkipHeldRemoveChannel`, `SkipHeldReleaseSustainedOnChannel` и ветки пропуска в
`OnNoteOn`/`OnNoteOff`/`OnAllNotesOff`/`OnSustain` — теперь это
`if(mSkippingEvents) return;`.

Что теряется осознанно: нота, удерживаемая на целевой позиции (пады, педальное
пиано), больше не дозвучивает — её услышит только следующий note-on; релиз уже
звучавших нот доигрывается ≈0.5 с после цели. Взамен ушёл транзиент
переатакованных педальных нот (замер ниже).

### 2. Откуда вырос размер (замер, а не догадка)

| состояние | канон (`-Os`, ALL_TABLES=ON, UI_METERS=ON) | `INTRA_UI_METERS=OFF` |
| --- | --- | --- |
| Update 82 | 216 997 | 216 014 |
| Update 83 (дозвучивание) | 218 402 (+1 405) | 217 419 (+1 405) |
| Update 84 (откат) | **217 008** (−1 394) | **216 025** (−1 394) |

Рост дало именно дозвучивание (1 405), а не фикс нулевой громкости: отдельным
замером живой слой CC7 временно отключён (зажим `BornCC7For` убран,
`ChannelGain` всегда 1, пустой `UpdateChannelGain`) и собран тот же канон —
**216 744**, то есть весь живой слой громкости канала (поле `ChannelGain`,
`BornCC7`, `UpdateChannelGain`, `scratch`-путь, `BornCC7For`/`ChannelGainFor`)
стоит **≈264 байта** (0.12 % файла). Замер разовый, исходники восстановлены,
финальная сборка — 217 008 (md5 `819c075a…`).

### 3. Зачем `scratch`-буфер в `SamplerTaskContext` (ответ на вопрос)

Он не про «зашивание нуля в огибающую» и не про деление на 0. Буферы кадра
ОБЩИЕ для всех нот: голоса смешиваются прямо в них. Громкость канала
(`ChannelGain` = CC7/CC7-при-рождении, либо 0 при мьюте) нужна ЖИВОЙ — её меняет
и файл (CC7), и ползунок UI, и мьют, причём у уже звучащей ноты. Чтобы домножить
вклад ОДНОЙ ноты, её надо отрендерить отдельно (частные буферы `ScratchL/R`,
один на контекст: задачи идут последовательно, и ноте нужен один буфер за
задачу) и потом добавить в микс с множителем. При `ChannelGain == 1` (обычный
случай: громкость канала не менялась) работает прежний прямой путь без лишних
проходов, и сигнал там бит-в-бит прежний (п. 4). Альтернатива «запекать CC7 в
стартовую громкость» — это ровно тот баг, который чинили в Update 82:
`sampler * 0` мёртв, и длинная нота на нулевой дорожке молчала навсегда.

### 4. Обычный рендер не изменился (замер)

`.scratch/render-ident84.mjs`: сырой PCM реального файла (tous les garçons, 12 с)
и живого источника (6 инструментов на разных каналах) — **md5 совпал с прошлой
сборкой** (`0c6cf88eea87` и `af98f04103f9`). Правки Update 84 живут только в
пути перемотки, поэтому клик-детектор не трогался: вывод
`.scratch/click-probe.mjs 43:83 74:83 75:83 115:83` — 43/75/115 по 0 кликов,
74 (рекордер, шумовой выдох) — как и раньше.

### 5. Перемотка после упрощения (замеры, `.scratch/update84-probe.mjs` 17/17)

- Нота, звучавшая до перемотки, после seek гаснет (сустейна нет: 2.1-2.5 с —
  0.000000); всплеска нет (пик сразу после seek 0.084 против 0.107 до него).
- Ноты, начавшиеся в пропущенном участке, после seek молчат (пик 0.000000),
  включая удерживаемые и педальные — «взрыва» нет ни на синтетике (30 нот в
  участке), ни на реальных файлах.
- Ноты ПОСЛЕ цели играют как обычно; серия перемоток вперёд (в т.ч. «в то же
  место») и путь «назад» (свежий источник + перемотка, как в UI) — без всплесков
  и клиппинга; живой слой CC7/мьют после перемотки работает (CC7 127 → 32:
  пик 0.153 → 0.039).
- Реальные файлы, сразу после перемотки против обычного воспроизведения той же
  точки: tous les garçons 0.761 против 0.798, celine 0.070 против 0.133,
  Chopin на 120 с 0.091 против 0.157 (в Update 83 было 0.510 — транзиент
  педального пиано исчез). Клиппинга нет нигде.
- Браузер (`.scratch/ui-check84.mjs` 12/12): перемотка вперёд/назад мгновенна
  (0.9 и 0.8 мс), без «взрыва» (0.547 → 0.283 сразу), без клиппинга, музыка
  продолжается (rms 0.17), серия перемоток не ломает страницу.

### 6. Кодировка имён дорожек (веб-UI)

Имя дорожки в SMF (FF 03) — просто байты без объявленной кодировки: старые
редакторы писали их в системной кодировке (для русских названий — cp1251:
«скрипки» читалось как ñêðèïêè), новые — в UTF-8. В `parseMidiTracks` добавлен
`decodeMidiText`: строгий UTF-8 (fatal), иначе выбор между `windows-1251` и
`windows-1252` по доле латинских букв с диакритикой в cp1252-декодировании
(> 0.5 → cp1251). Порог и оба исхода замерены: `.scratch/encoding-probe.mjs`
10/10 — cp1251 «скрипки»/«Piano скрипки»/«Дорожка №1 (соло)», UTF-8 «Партия
флейты», cp1252 «Café solo»/«Mélodie» (не «Cafй»), ASCII, три дорожки в разных
кодировках в одном файле, нет U+FFFD, пустое имя. Решение и ограничения —
`docs/decisions/active/20260914-MidiTextEncodingHeuristic.md`. В браузере (та же
проба): «скрипки» вместо «ñêðèïêè», UTF-8 и ASCII — верно.

### 7. Регрессии

- `update84-probe` 17/17, `update82-probe` (проверка 4 намеренно возвращена к
  прежнему инварианту: после перемотки в участке ничего не звучит),
  `smoke-test-wasm` PASSED, `levels-probe`, `piano-level-probe`,
  `mute-mask-probe`, `cc7-probe` — всё прошло; `encoding-probe` 10/10.
- Playwright: `ui-check84` 12/12, `ui-check82` 8/8, `ui-check81` 7/7,
  `ui-check80` 11/11, `ui78-tous` на реальном файле 7/7 — ошибок страницы нет.
- Устаревшие пробы Update 83 (`.scratch/update83-probe.mjs`,
  `.scratch/update83-realfile.mjs`) удалены: их инвариант (дозвучивание) отменён,
  живая проверка — `update84-probe`; замер ёмкости (`.scratch/held-polyphony.mjs`,
  `held-split.mjs`) сохранён как история.

### Размер

WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`, `INTRA_UI_METERS=ON`) =
**217 008 байт** (было 218 402 в Update 83, **−1 394**; к Update 82 +11 — цикл
«гасим всё» и комментарии). Сборка с `INTRA_UI_METERS=OFF` = **216 025 байт**
(разница ON/OFF прежняя, 983). `web/generated/` и `dist/` синхронизированы
(md5 `819c075a…`).

### Замечание по инструменту (важно для дальнейших правок)

Файловые инструменты платформы видят только НАЧАЛО больших файлов: их снапшот
обрезан, поэтому патчи хвоста рапортуют «old string not found» для текста,
который на диске есть (проверено на `web/synth.js`: находится `renderSampleTabs`
из середины файла и не находится `parseMidiTracks`; на самом worklog: находится
`held-split` и не находится последняя строка Update 83). Патчить таким
снапшотом опасно — он записал бы обрезанную ревизию файла целиком. Поэтому
хвосты вносились скриптами с проверкой ровно одного вхождения:
`.scratch/patch-midi-text.mjs` (вставка `decodeMidiText` в `web/synth.js`),
`.scratch/append-worklog84.mjs` (этот раздел). Корректность после правки
проверена: `node --check web/synth.js`, `node scripts/build-web.js` +
`cmp web/synth.js dist/synth.js`, браузерная проба `ui-check84`.

### Откаты

C++ — `git diff` по `MidiSynth.h`/`MidiSynth.cpp` и одна строка комментария в
`EmscriptenInterface.cpp`; JS — единственная вставка `decodeMidiText` + замена
`String.fromCharCode` на `decodeMidiText(data)` в `parseMidiTracks`.
