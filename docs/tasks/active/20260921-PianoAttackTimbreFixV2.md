# PianoAttackTimbreFix V2 — повторное измерение sustain и подготовка к новой физической модели

Дата начала: 2026-09-20/21

Продолжение: `docs/tasks/active/20260820-PianoAttackTimbreFix.md`.

## Цель

После длинной первой серии экспериментов стало ясно, что задача — не максимальное
сжатие таблиц само по себе. Главные требования владельца:

1. сохранить или повысить слышимое качество относительно текущего baseline;
2. отделить реальные свойства исходного Clavinova/SF2 от ошибок старых измерений;
3. по возможности перейти к физически интерпретируемой модели;
4. если часть тембра не сворачивается в физическую формулу, хранить её честно как
   понятный voicing/calibration layer, а не скрывать в PCA/rank-коэффициентах;
5. размер важен для 64K intro, но текущие сотни байт piano-таблицы уже достаточно
   малы, поэтому размер не должен ухудшать интерпретируемость или звук.

V1 содержит историю старой атаки, молоточка, unison и первого baseline. Этот файл
фиксирует ВСЕ новые эксперименты после handoff в новый чат, включая отрицательные.

---

## Исходное состояние V2

Передано из прошлого чата:

- принятый additive piano baseline;
- packed partial table: 544 partial × 11 B;
- отдельные `Amp`, `Decay1..4`, `Phase`, `FreqRatio`;
- smooth19 как предыдущая компактная макро-модель Amp;
- source-derived p90/early/low16 targets;
- текущий быстрый two-string collapse в `AdditiveSampler`;
- вывод старого исследования: частоты/decay хорошо поддаются процедурному
  описанию, Amp гораздо чувствительнее к локальному рисунку гармоник.

Первые тесты V2 использовали первые ~17 с пользовательского MIDI, две piano
дорожки, reverb = 0.

---

## Эксперимент 1: 2/3/4-bit residual поверх smooth19

### Идея

Проверить, можно ли заменить source-derived low-harmonic fingerprint очень
маленькой квантованной поправкой к smooth19.

Кандидаты:

- smooth19;
- 2-bit corrections для h1..h16;
- 3-bit;
- 4-bit;
- exact low16;
- full robust p90.

### Первое прослушивание

Владелец сначала счёл даже 2-bit вариант эквивалентным baseline.

### Повторное прослушивание

После более внимательного сравнения выяснилось:

- у 2/3/4-bit проявляется одна и та же небольшая потеря мягкости на отдельных
  нотах;
- эффект почти не зависит от числа бит;
- exact/source-derived варианты не дают той же явной деградации.

Это указало, что длинный 17-секундный фрагмент плох для тонкого A/B: слуховая
память замыливается при перемотке.

### Blind triangle test

Сделаны короткие локальные тройки вокруг участков с наибольшей численной
разницей. В каждой тройке два файла одинаковы, один отличается.

Результат владельца:

- 6 определённых ответов из 6 попали в odd-one-out;
- ещё 2 ответа были `?`, где разница действительно не слышалась.

Вывод: 2-bit residual уже слегка переходит индивидуальный слуховой порог,
хотя на длинном фрагменте кажется эквивалентным.

### Решение

Не использовать 2-bit fingerprint как финальную модель. Размер уже не настолько
важен, чтобы оправдывать слабую, но воспроизводимую деградацию.

---

## Эксперимент 2: повторное измерение исходного SF2 и подозрение на p90

В новый чат сначала не был заново загружен SF2. Первые compact-тесты поэтому
строились из сохранённого старого source-derived state, а не из нового
измерения исходника. После загрузки `Titanic 200 GM-GS v1.2.sf2` измерение
повторено.

### Найденные проблемы старого измерительного подхода

1. p90 по независимым временным окнам может выбирать максимум каждой гармоники
   в РАЗНЫЙ момент биения. В результате получается спектр «Франкенштейна»,
   которого физически не было одновременно.
2. Для отдельных верхних partial p90 был на 12–15 dB выше более устойчивого p75.
3. Некоторые найденные пики сидели у края частотного поискового окна и гуляли,
   что похоже на резонанс/соседний пик/биение, а не устойчивый partial.
4. Старый анализ местами смешивал L/R waveform до спектрального измерения.
   Из-за фазового вычитания это может искажать partial levels. В новой ветке
   L/R должны объединяться по ЭНЕРГИИ.

### Диагностический listening round

Сравнивались baseline, saved full p90, low16 exact, 2-bit, fresh p75,
early-median и positive-boost capped вариант.

Результат владельца:

- кроме слегка подозрительного 2-bit, остальные звучали эквивалентно baseline;
- p75/median/capped не ухудшали звук.

Вывод: точные огромные положительные p90-пики не являются обязательным
носителем характера.

---

## Эксперимент 3: новая физическая Amp-модель (physical7 / physical10)

### Цель

Вместо fingerprint построить небольшой набор понятных физических параметров:

- hammer spectral cutoff;
- изменение cutoff по регистру;
- slope/modal rolloff;
- bridge/soundboard broad response;
- дополнительный широкий body resonance.

Отдельно проверялся strike-position comb `sin(pi*k*x)`.

### Результаты fitting

- оптимизатор почти выключил strike-position comb;
- physical7 и physical10 автоматически приблизились к smooth19 по ошибке;
- body resonance улучшал численную метрику лишь немного.

### Прослушивание

Владелец не слышал разницы между smooth19, physical7 и physical10: все три
звучали одинаково плохо относительно baseline, то есть более обобщённо и
менее живо.

### Вывод

Проблема не в выборе одной более красивой гладкой spectral envelope. Нужна
локальная структура первых гармоник, которую smooth/physical family теряет.

---

## Эксперимент 4: common core трёх «хороших» спектров

Вместо попытки объяснить один baseline взяты три spectra, которые уже проходили
на слух:

- baseline;
- robust p90;
- early target.

Строился consensus residual относительно smooth19:

- оставлялась поправка только там, где хорошие варианты согласны по знаку;
- величина бралась консервативно.

### Наблюдения

- около 75% low16 ячеек имели одинаковое направление поправки относительно
  smooth19;
- region-wide mean gain объяснял только ~24% энергии consensus;
- после удаления region gain оставшийся relative spectral shape всё равно
  содержал основную слышимую информацию.

### Прослушивание

Владелец не отличил от baseline:

- full robust p90;
- common core low16;
- common core + region gain;
- common spectral shape WITHOUT region gain.

Smooth19 при этом оставался явно плохим.

### Вывод

Характер не сидит в общей громкости sample-zone. Он сидит в относительном
рисунке низких гармоник.

---

## Эксперимент 5: harmonic cutoff и locality по sample-zone

### Harmonic cutoff

Проверялись common-shape только до h8 / h12 / h14 / h16.

Первый результат:

- h1..h8 заметно менее звонкий, хотя не режет слух;
- h1..h12 и выше не отличались от baseline.

На следующее утро владелец уже не был уверен, что отличает даже h1..h8. Это
показало, что такой cutoff находится около плавающего слухового порога и не
годится как главный архитектурный критерий.

### Register locality

Соседние source-zones объединялись и делили один shape.

Результат:

- даже объединение двух соседних зон (`pair averaged`) звучало резко хуже,
  приблизительно как старые rank-модели;
- точный local shape каждой зоны проходил как baseline.

### Вывод

Локальность sample-zone критична. Нельзя просто плавно интерполировать один
общий spectral shape по клавиатуре.

---

## Эксперимент 6: интерпретируемые параметры внутри каждой зоны

Поскольку зоны нельзя объединять, была проверена идея оставить каждой зоне
маленький набор понятных параметров:

- brightness tilt;
- curvature;
- odd/even balance;
- low-mid hump;
- upper shelf;
- полосовые группы 1–2 / 3–4 / ...;
- N strongest explicit harmonic corrections.

### Прослушивание

- exact K1..12 local shape совпадает с baseline;
- все компактные параметризации хуже;
- 5-параметрический smooth voicing вариант худший среди них;
- strongest-corrections и banded варианты ближе, чем smooth19/rank1, но всё
  равно явно теряют характер.

### Вывод

Даже при сохранении отдельной настройки каждой зоны несколько гладких ручек
не заменяют необходимый low-harmonic рисунок.

---

## Эксперимент 7: slow register trend vs sample-zone calibration

Consensus shape разложен на:

- медленную зависимость каждой гармоники от регистра;
- локальный residual конкретной sample-zone.

Также проверялись wrong-neighbour residual и half-local residual.

### Прослушивание

- exact common shape = baseline;
- slow trend only — сильно хуже;
- local residual only — промежуточно;
- trend + wrong neighbour и trend + half local тоже промежуточны.

### Вывод

Ни «универсальная физика», ни «только sample calibration» по отдельности не
восстанавливают характер. Хороший результат требует достаточно точного
совместного рисунка.

---

## Эксперимент 8: strike-comb и voicing islands/boxes

В consensus map обнаружены крупные острова, например:

- подавление h8 в нескольких нижне-средних root zones;
- отдельные области h9..h12;
- широкие усиления в других регистрах.

Проверено:

1. per-zone strike-position comb;
2. 8/12/20 явно подписанных register×harmonic boxes;
3. strike + boxes.

Strike fit объяснял лишь около 40% энергии карты, а найденный strike position
скакал между соседними зонами и часто упирался в ограничения.

### Прослушивание

Все strike / boxes / hybrid варианты заметно хуже exact common shape.

### Вывод

Крупные «острова» реальны как описание таблицы, но их недостаточно как
акустическая модель. Не принято.

---

## Эксперимент 9: entanglement Amp и Decay

Гипотеза: irregular Amp может компенсировать irregular per-harmonic decay.
Тогда сглаживание Amp отдельно было бы заведомо ошибочным.

Сделан причинный тест:

- smooth19 Amp + baseline decay;
- smooth19 Amp + сглаженные Decay1..4;
- exact common Amp + smooth decay;
- baseline Amp + smooth decay.

### Прослушивание

- smooth19 + smooth decay всё равно = плохой smooth19;
- exact/common Amp остаётся baseline-like даже со smooth decay;
- baseline Amp со smooth decay тоже не даёт явной деградации.

### Вывод

Неровность decay НЕ объясняет потерю характера smooth19. Amp и decay можно
в значительной степени упрощать независимо.

---

## Эксперимент 10: joint temporal Amp+Decay remeasurement — ПРИНЯТО

Это главный положительный результат V2.

### Старый дефект

Amp и decay ранее могли измеряться разными методами/окнами, а p90 мог ловить
разные фазы биений разных partial.

### Новый метод

Для каждого partial одной stereo sample-zone:

1. L/R НЕ суммируются по waveform; складывается спектральная энергия;
2. по устойчивому раннему спектру выбирается ОДНА фиксированная частота partial;
3. эта линия отслеживается по всей ноте;
4. fitting начинается только с `PianoRegionData::DecayOnset`, атака исключена;
5. одна непрерывная piecewise-exponential траектория даёт одновременно:
   - Amp в `DecayOnset`;
   - Decay1;
   - Decay2;
   - Decay3;
   - Decay4;
6. границы decay segments остаются текущими региональными;
7. слабые/шумовые хвосты не заставляют fitter создавать абсурдные rates.

### Прослушивание

Владелец отметил:

- первая нота стала лучше;
- C7 теперь затухает ближе к исходному SF2; в старом baseline верхний sustain
  оставался слишком громким/свистящим;
- ноты в целом стали более собранными, старый baseline местами воспринимался
  слегка раздвоенным;
- явного ухудшения тембра нет;
- новый звук чуть мягче, часть старой «особенности Clavinova» могла быть не
  реальной особенностью банка, а ошибкой первого измерительного агента.

Это важное изменение цели: старый baseline больше НЕ считается абсолютной
истиной. Новый joint target может быть лучше исходного baseline именно потому,
что устраняет его измерительные ошибки.

### Решение

Amp + Decay1..4 из joint temporal fit приняты как новый sustain baseline.

Частоты и phase НЕ меняются этим коммитом.

---

## Эксперимент 11: unison depth/rate/initial phase

После joint fit отдельно исследовались биения двух струн.

### Найдено в старом коде

Текущий быстрый two-string collapse представляет две струны одним carrier и
медленной AM-огибающей. Он сохраняет основную глубину биений, но выбрасывает
малый carrier phase wobble.

В V1 уже был exact complex-unison эксперимент: он стоил около 1.9× горячего
цикла, а владелец разницы не услышал, поэтому был откатан.

### Новые измерения

Проверялись:

- remeasured beat rate;
- rate + depth + initial beat phase;
- phase-aware collapse без второго oscillator lane.

Для rate estimator введено физическое ограничение: h1..h4 одной ноты должны
давать согласованный fundamental beat rate, а не независимые случайные maxima.
Дополнительно временная модуляция сверялась со spectral splitting.

### Прослушивание

Владелец сравнил control и все fast-unison варианты на нескольких мелодиях и
long-note diagnostics и не смог услышать разницу вообще.

### Phase-aware negative result

Экспериментальный phase-aware collapse снова испортил начало ноты: атака
воспринималась исчезнувшей/ослабленной. Похожий дефект уже происходил в V1 при
работе с phase.

Причина важна архитектурно: phase нельзя бездумно добавлять поверх существующей
инициализации carrier/attack. Нулевая/общая стартовая фаза двух unison strings
в текущем production path была выбрана не случайно: старый per-voice delay
создавал comb-filter атаки.

### Решение

- новые rate/depth/initial-phase таблицы НЕ принимаются в runtime;
- phase-aware collapse НЕ принимается;
- текущий быстрый collapse оставляется;
- результаты измерений сохраняются только как исследовательская история.

---

## Эксперимент 12: SIMD regression при сборке Emscripten — ИСПРАВЛЕНО

При первой сборке новых WASM Emscripten 6.0.9 пользователь заметил, что они
работают в 2.5–3 раза медленнее baseline.

### Причина

`-msimd128` сам по себе включает wasm SIMD ISA, но НЕ переключает ручные SIMD
ядра Intra. В `Intra/Simd/Simd.h` library default для этого target оставался
`INTRA_SIMD_NONE`.

Нужны ОБА условия:

- compiler ISA: `-msimd128` / compat `-msse2`;
- `INTRA_SIMD_SUPPORT=INTRA_SIMD_SSE2`.

Без второго условия командная строка выглядит «SIMD», но `AdditiveSampler`
компилирует scalar path.

### Проверка

После правильной сборки в WASM реально присутствуют `v128` / `f32x4`
инструкции, а производительность вернулась примерно к baseline.

### Изменение в коде

В Emscripten branch `intrasynth/CMakeLists.txt` default теперь сразу задаёт:

- `INTRA_SIMD_SUPPORT=INTRA_SIMD_SSE2`;
- `-msse2` для compat intrinsics headers;
- существующий `-msimd128` остаётся.

Explicit CMake override всё ещё работает.

---

## Размер WASM и Emscripten 6.0.9

Пользователь заметил рост экспериментальных WASM примерно с 220 КБ до 270 КБ.
Разложение причины:

1. Исторический baseline (старый SDK): 219800 B.
2. Чистый текущий `synth-wip`, Emscripten 6.0.9, `-Os`, правильный SIMD,
   `INTRA_UI_METERS=ON`, `INTRA_PIANO_ALL_TABLES=OFF`, без новых piano
   экспериментов: 242380 B.
3. Значит около +22.6 КБ даёт новый SDK/codegen/runtime при тех же исходниках.
4. Остальной рост до ~270 КБ был от временных unison experiment tables/code.
5. После удаления неуспешного unison-кода и принятия ТОЛЬКО joint Amp+Decay
   latest-SDK acoustic-only build (`INTRA_PIANO_ALL_TABLES=OFF`, UI meters ON)
   снова ровно 242380 B.
6. Каноническая owner-tracked конфигурация по `AGENTS.md`: `-Os`, SIMD,
   `INTRA_UI_METERS=ON`, `INTRA_PIANO_ALL_TABLES=ON` — **254542 B** на
   Emscripten 6.0.9.

Важно: joint Amp+Decay заменяет значения в существующей 5984-byte packed table
и сам по себе размер WASM НЕ увеличивает.

Не использовать `-Oz`: владелец отслеживает `-Os`, а -O2/-O3 раньше не давали
полезного прироста на этом synth.

---

## Unified fitter

Добавлен `intrasynth/tools/analysis/fit-piano.mjs`.

Назначение: заменить россыпь одноразовых Amp/Decay-зондов одним воспроизводимым
pipeline.

Он:

- парсит исходный SF2 напрямую;
- читает regions/packed table из `PianoRegions.h`;
- использует общий проверенный FFT `intrasynth/tools/analysis/lib/fft.mjs`;
- детектирует sample onset;
- складывает L/R по spectral energy;
- выбирает один fixed peak на partial;
- строит временную partial trajectory;
- robust-IRLS фитит одну continuous 4-stage decay curve;
- получает Amp и Decay1..4 из ОДНОЙ траектории;
- сохраняет per-zone Loudness через спектральную нормировку;
- по умолчанию НЕ меняет Phase/FreqRatio;
- `--fit-frequency` позволяет отдельно обновить FreqRatio;
- пишет JSON с диагностикой и, по запросу, отдельный candidate header.

Скрипт не имеет npm-зависимостей и использует существующую analysis library.
Полный тестовый прогон всех 25 зон на Titanic SF2 занимает около 3 секунд в
текущем контейнере.

Принятая packed table была получена предыдущим scipy prototype joint fitter;
новый Node script — консолидированный successor метода, а не обещание
бит-в-бит повторить старый prototype.

---

## SF2 reference «без молоточка» для диагностики

Чтобы сравнивать sustain без доминирующего hammer transient, сделан отдельный
диагностический SoundFont/render.

Не использовался произвольный global shift. Для КАЖДОЙ из 25 Clavinova zones
sample start сдвинут ровно на ту временную точку, которая является t=0 для
joint Amp/Decay fit:

`detect_onset(sample) + PianoRegionData::DecayOnset`.

На Titanic Clavinova samples `detect_onset == 0`, потому что samples уже
trimmed. Поэтому фактические shifts:

- 115 ms почти везде;
- root 75: 50 ms;
- root 81: 75 ms;
- root 84: 80 ms;
- root 87: 70 ms;
- root 90: 50 ms;
- root 93: 50 ms;
- root 96: 40 ms.

Это НЕ математическое вычитание молоточка: к этому времени удар уже успел
возбудить струну и корпус. Но это гораздо более честный reference sustain для
сравнения с нашей гармонической моделью.

---

## Текущее принятое состояние после V2

В runtime принимается:

1. прежняя архитектура additive piano;
2. прежние `FreqRatio` и `Phase`;
3. прежний fast unison collapse;
4. НОВЫЕ joint-temporal `Amp + Decay1..4`;
5. исправленный default WASM SIMD build.

НЕ принимаются:

- smooth19/physical7/physical10 как финальная Amp-модель;
- rank/PCA models;
- 2-bit fingerprint;
- strike-position comb;
- box/island voicing compression;
- sample-zone averaging;
- smooth local 5-parameter voicing;
- Amp/Decay entanglement hypothesis;
- remeasured unison rate/depth/phase (неслышимо);
- phase-aware collapse (ломает атаку).

---

## Следующий этап

Теперь есть смысл ВЕРНУТЬСЯ К ФИЗИКЕ заново, но фитить её не к старому
baseline, а к более чистому joint temporal target / raw SF2 trajectories.

Приоритет:

1. проверить, стала ли residual Amp-map после нового temporal measurement более
   гладкой/физически объяснимой;
2. отдельно исследовать hammer/contact transient:
   - первые ~0..DecayOnset;
   - вычесть предсказанный harmonic modal response;
   - анализировать residual как broadband contact + inharmonic body modes;
3. не смешивать hammer residual с sustain Amp;
4. velocity в этом SF2 не даёт независимой sample-layer информации для
   Clavinova: один и тот же sample переиспользуется, поэтому будущую velocity
   dependence придётся задавать как отдельную физическую модель, а не
   «измерять» из разных sample layers;
5. длинные ноты использовать для decay/unison, короткие локальные A/B — для
   тонких timbre differences.

## Human verification status

Принятый joint temporal sustain пользователь слушал на нескольких мелодиях и
длинных нотах. Ухудшений относительно baseline не обнаружено; C7 decay явно
стал ближе к исходному SF2. Unison refinements ниже слухового порога и поэтому
не включены.