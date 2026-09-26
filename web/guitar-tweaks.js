// Ручки живого эксперимента гитар 29 Overdriven / 30 Distortion (Updates 125-127).
//
// Update 127 (слово владельца): «Как бы я ни крутил ручки, звук неприятный…
// Выставь дефолты в guitar steel, чтобы звучало именно так, а я уже буду крутить
// и смотреть, что как влияет». Поэтому ДЕФОЛТ панели — не канон перегруза, а
// пресет «GUITAR STEEL»: множители подобраны так, что струна 29/30 звучит как
// программа 25 (AcousticGuitarSteel), а перегруз накатывается ручкой Drive.
//
// Почему это достижимо МНОЖИТЕЛЯМИ. В конструкторе SpectralStringSampler полоса
// PickupHz, наклон TiltExp и комб Pickup считаются ПО МОДАМ в раскладке спектра
// — то есть входят в саму струну и действуют НЕЗАВИСИМО от Drive. Поэтому при
// Drive = 0 «чистая струна» всё равно окрашена каноническими pickupHz/tilt/pickup
// (это и слышал владелец: «дребезг есть даже без клипера»). Пресет steel снимает
// всю окраску: pickupHz = 0 (ФНЧ 4-го порядка выключен), pickup = 0 (комба нет),
// tilt = 0 (множитель 0 даёт tiltExp = 0, а k^0 = 1, то есть наклона нет — ровно
// как у Karplus-Strong), inharm = 0, beatDepth = 0, sfBase/expCoeff пересчитаны в
// числа steel (0.25/1.0 против канонических 0.08/1.35 у 29 — Update 131; после
// этой правки множители steel другие, чем в Updates 127-130: изменился канон).
//
// Update 131: у 30 канонический damping 0.15 (сглаженная атака — просьба владельца
// «атака слишком колючая, у Титаника она сглажена»), а у steel 0.05. Пресет steel
// damping НЕ задаёт (иначе поехали бы числа 29), поэтому на 30 он даёт ту же струну
// с более гладким щипком — это ожидаемое расхождение, а не ошибка.
//
// Здесь только UI и вызов C-API: множители живут в WASM (GuitarTweaks в
// SpectralStringSampler.h, SynthSetGuitarTweaks в EmscriptenInterface.cpp), и
// голос читает их при создании. Изменение слышно на НОВЫХ нотах.
(() => {
  "use strict";

  // Зеркало порядка значений SynthSetGuitarTweaks. def — значение по умолчанию
  // (дефолт панели = пресет guitar steel).
  const GUITAR_TWEAKS = [
    { key: "drive",       label: "Drive — клипер",          min: 0,    max: 2,   step: 0.01, def: 0    },
    { key: "bias",        label: "Bias — асимметрия",       min: 0,    max: 3,   step: 0.01, def: 1    },
    { key: "tone",        label: "Tone — срез верха",       min: 0.25, max: 3,   step: 0.01, def: 1    },
    { key: "pickupHz",    label: "PickupHz — полоса",       min: 0,    max: 3,   step: 0.01, def: 0    },
    { key: "pickup",      label: "Pickup — точка датчика",  min: 0,    max: 2,   step: 0.01, def: 0    },
    { key: "pickupDepth", label: "PickupDepth — нули",      min: 0,    max: 3,   step: 0.01, def: 1    },
    { key: "sustainMax",  label: "SustainMax — сустейн",    min: 1,    max: 2,   step: 0.01, def: 1    },
    { key: "tilt",        label: "Tilt — наклон струны",    min: 0,    max: 2,   step: 0.01, def: 0    },
    { key: "damping",     label: "Damping — шум медиатора", min: 0,    max: 8,   step: 0.01, def: 1    },
    { key: "beatDepth",   label: "BeatDepth — биения",      min: 0,    max: 4,   step: 0.01, def: 0    },
    { key: "cabQ",        label: "CabQ — кабинет",          min: 0.5,  max: 2,   step: 0.01, def: 1    },
    { key: "sfBase",      label: "SfBase — длина ноты",     min: 0.25, max: 6,   step: 0.01, def: 3.13 },
    { key: "expCoeff",    label: "ExpCoeff — спад",         min: 0,    max: 2,   step: 0.01, def: 0.74 },
    { key: "inharm",      label: "Inharm — жёсткость",      min: 0,    max: 5,   step: 0.01, def: 0    },
    { key: "scale",       label: "Scale — уровень",         min: 0,    max: 3,   step: 0.01, def: 0.82 },
    // Последние две — НЕ множители, а абсолютные значения (Гц и дБ).
    { key: "presenceHz",  label: "PresenceHz — полка, Гц",  min: 500,  max: 9000, step: 50,   def: 2500, canon: 2500, dec: 0 },
    { key: "presenceDb",  label: "PresenceDb — полка, дБ",  min: 0,    max: 18,   step: 0.5,  def: 0,    canon: 12,  dec: 1 },
    // Update 134: трение струны (потери гармоник ∝ их частоте ВЫШЕ основного тона).
    // Множитель к каноническим 1.8: больше — верхние гармоники уходят быстрее
    // (сустейн «глаже», как у банка), 0 — трение материала выключено, остаётся
    // только петля KS (как у guitar steel). Прессеты: steel 0, канон 1, вдвое 2.
    { key: "dampSlope",   label: "DampSlope — трение",      min: 0,    max: 4,   step: 0.05, def: 0    },
    // Update 150: ТЕМБР-СТЕК ТРАКТА — три секции с ФИКСИРОВАННЫМИ (по Гц)
    // частотами: полка низа, резонансный пик присутствия, полка верха. Числа в
    // каноне подобраны замером (.scratch/u152-fit.mjs) — см. toneStack в
    // guitar-modal.json. toneStack — МНОЖИТЕЛЬ усилений всех трёх секций (0 —
    // звено выключено целиком, то есть прежняя окраска; 1 — канон; 2 — вдвое),
    // остальные — АБСОЛЮТНЫЕ частота/добротность пика (0 — «как в каноне») и
    // добавки к полкам в дБ. canon — значение для пресета «канон».
    { key: "toneStack",       label: "Тембр-стек (множитель)",   min: 0,   max: 2,    step: 0.05, def: 0, canon: 1 },
    { key: "toneStackLowDb",  label: "Тембр-стек: низ, дБ",      min: -15, max: 8,    step: 0.5,  def: 0, canon: 0, dec: 1 },
    { key: "toneStackPeakHz", label: "Тембр-стек: пик, Гц",      min: 0,   max: 5000, step: 50,   def: 0, canon: 0, dec: 0 },
    { key: "toneStackPeakQ",  label: "Тембр-стек: пик, Q",       min: 0,   max: 3,    step: 0.05, def: 0, canon: 0, dec: 2 },
    { key: "toneStackHighDb", label: "Тембр-стек: верх, дБ",     min: -20, max: 8,    step: 0.5,  def: 0, canon: 0, dec: 1 },
  ];
  const COUNT = GUITAR_TWEAKS.length + 1; // +1 — флаг «включено»
  const showVal = (p, v) => Number(v).toFixed(p.dec ?? 2);

  // Пресеты: множители, которых нет в объекте, равны 1 (канон).
  //   steel  — струна как программа 25 (AcousticGuitarSteel), усилитель выключен;
  //   canon  — канон перегруза 29/30 бит-в-бит;
  //   rock   — БОГАТЫЙ ВХОД + МЯГКИЙ КЛИП (Update 129, замер .scratch/u129-cand.mjs);
  //   shelf  — канон + полка ВЧ после кабинета (+12 дБ @2.5 кГц).
  //
  // Почему rock выглядит именно так. Замер собранной сборки в окне атаки: у банка
  // Overdriven crest 1.76-2.36 и flat 4-9 % (доля периода в 15 % от пика), то есть
  // банк — СЛАБО клиппованная богатая струна. У нашего канона crest 1.27-1.80 при
  // flat 18-52 %: глубина клипа больше банковской, а входа в клипер меньше
  // (полоса PickupHz 3.2-4.2 кГц — это ФНЧ 24 дБ/окт ДО нелинейности). Отсюда
  // «дребезжащая прямоугольность» вместо драйва.
  // Пресет расширяет полосу ДО клипа вдвое и снижает глубину клипа вдвое:
  // сумма |flat − банк| по клавишам 52/64/76/88 падает 90.2 → 38.3, RMSE полос
  // 32.81 → 28.34 дБ, мусор между гармониками НЕ растёт (−108…−131 дБ, как в
  // каноне), уровень ниже на 3-4 дБ — отсюда Scale 1.3.
  // `_canon: true` — база пресета каноническая (все множители 1, полки нет),
  // иначе — дефолт панели (то есть steel).
  const PRESETS = {
    // Update 131: sfBase/expCoeff/scale пересчитаны под новый канон 29 (0.08/1.35/
    // 0.12168): 0.25/0.08 = 3.125, 1.0/1.35 = 0.74, а scale подобран под тот же
    // абсолютный уровень, что и прежде (0.64 x 0.156 = 0.0998 → 0.0998/0.12168 = 0.82).
    // Update 150: steel без тембр-стека (у «чистой струны» формант тракта нет).
    steel: { drive: 0, pickupHz: 0, pickup: 0, tilt: 0, dampSlope: 0, inharm: 0, beatDepth: 0, sfBase: 3.13, expCoeff: 0.74, scale: 0.82, toneStack: 0 },
    canon: { _canon: true },
    rock: { _canon: true, pickupHz: 2, drive: 0.5, scale: 1.3 },
    shelf: { _canon: true, presenceHz: 2500, presenceDb: 18 },
    // Update 134: канон с полкой ПОНИЖЕ. Замер (.scratch/u134-level2.mjs): полка
    // +12 дБ кладёт уровень сустейна ВЫШЕ банка на +2…+5 дБ в диапазоне C5…G6
    // (в самой середине — паритет), и это единственный рычаг для «C6 оглушает»;
    // при 9 дБ верх возвращается к уровню банка (на C6 +1.3 вместо +3.2), теряя
    // примерно треть присутствия. Полка 12 остаётся каноном — её владелец
    // принял на слух, и она не тронута.
    shelf9: { _canon: true, presenceHz: 2500, presenceDb: 9 },
    // Трение струны (DampSlope) — от канонических 1.8: 0 — без трения (струна
    // как у guitar steel), 2 — вдвое больше (верх уходит ещё быстрее банка).
    // Update 150: канон БЕЗ нового звена — прежняя окраска бит-в-бит (сравнить
    // «с тембр-стеком / без него» можно прямо на слух, не пересобирая WASM).
    nostack: { _canon: true, toneStack: 0 },
    lowfric: { _canon: true, dampSlope: 0 },
    highfric: { _canon: true, dampSlope: 2 },
  };

  let ptr = 0;
  let ptrModule = null; // инстанс, которому принадлежит ptr (сбрасывается при A/B)
  let sliders = [];

  function getModule() {
    const hook = window.__intraGuitarTweaks;
    return hook && hook.getModule ? hook.getModule() : null;
  }

  // Значение слайдера = множитель из пресета, если он там есть, иначе база пресета.
  // База пресета «канон»: множители 1, а полка тембр-стека — СВОИ канонические
  // числа (Update 130: +12 дБ @2.5 кГц запечено в канон, см. guitar-modal.json).
  // Пресет steel — база дефолтов панели (струна без усилителя, полки нет).
  // База канона у каждого слайдера своя (canon): множители — 1, канонические
  // числа полки — свои (2500 Гц / 12 дБ, Update 130), у тембр-стека — 1 и нули
  // (Update 150: 0 в дельтах и в частоте пика означает «как в каноне»).
  function presetValue(index, preset) {
    const p = GUITAR_TWEAKS[index];
    const base = preset._canon ? (p.canon ?? 1) : p.def;
    return p.key in preset ? preset[p.key] : base;
  }

  function applyPreset(name) {
    const preset = PRESETS[name] || PRESETS.steel;
    sliders.forEach((s, i) => {
      const v = presetValue(i, preset);
      s.input.value = String(v);
      s.val.textContent = showVal(GUITAR_TWEAKS[i], v);
    });
    const head = document.getElementById("gt_enable");
    if (head) head.checked = true;
    push();
  }

  // Отправляет текущие значения в WASM. Лениво выделяет буфер под текущий
  // инстанс: после A/B-переключения WASM другой, и указатель прежнего невалиден.
  function push() {
    const M = getModule();
    if (!M) return false;
    if (ptrModule !== M) {
      ptr = M._malloc(COUNT * 4);
      ptrModule = M;
    }
    const head = document.getElementById("gt_enable");
    const view = M.HEAPF32;
    const off = ptr >> 2;
    view[off] = head && head.checked ? 1 : 0;
    sliders.forEach((s, i) => { view[off + 1 + i] = parseFloat(s.input.value); });
    if (typeof M._SynthSetGuitarTweaks !== "function") return false;
    M._SynthSetGuitarTweaks(ptr);
    const hook = window.__intraGuitarTweaks;
    if (hook && hook.pushRealtime) {
      hook.pushRealtime(Array.from(view.subarray(off, off + COUNT)));
    }
    return true;
  }

  function build() {
    const grid = document.getElementById("gtweakGrid");
    const head = document.getElementById("gt_enable");
    if (!grid || sliders.length) return;

    sliders = GUITAR_TWEAKS.map((p, i) => {
      const row = document.createElement("div");
      row.className = "gtweak-row";
      const label = document.createElement("label");
      label.textContent = p.label;
      label.htmlFor = "gt_" + p.key;
      const input = document.createElement("input");
      input.type = "range";
      input.id = "gt_" + p.key;
      input.min = String(p.min);
      input.max = String(p.max);
      input.step = String(p.step);
      input.value = String(p.def);
      input.dataset.index = String(i);
      const val = document.createElement("span");
      val.className = "gtweak-val";
      val.textContent = showVal(p, input.value);
      row.append(label, input, val);
      grid.append(row);
      return { input, val };
    });

    grid.addEventListener("input", (e) => {
      const input = e.target.closest("input[type=range]");
      if (!input) return;
      const i = Number(input.dataset.index);
      sliders[i].val.textContent = showVal(GUITAR_TWEAKS[i], input.value);
      if (head) head.checked = true;
      push();
    });
    if (head) head.addEventListener("change", push);
    document.querySelectorAll("[data-gtpreset]").forEach((btn) => {
      btn.addEventListener("click", () => applyPreset(btn.dataset.gtpreset));
    });
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", build);
  } else {
    build();
  }
})();
