
## Session 27 — 2026-08-24: Metal pitch, E5 knock, string bloom sqrt(k)

### User complaints
- C3-B4: "metal without noise" — modal group 2 (1.3-2.9k) at ×1.9 rang
  like metal below C5; metGate (midi-71)/1 was correct but the modal ring
  wasn't gated.
- C7: "strike pitch different" — sample strike = 2160 Hz (≈f0), ours was
  4220 Hz (2×f0). Metal center was 1850 + upper partials dominated; also
  string K=2 had two unison partials (0.0468 combined > K=1's 0.0452)
  with slower decay → 2nd harmonic overtook f0 in attack.
- E5: "strange knock" — floor LP noise at ~218 Hz dominated attack;
  Goertzel short-window scan showed 160 Hz peak (DFT artifact, but the
  floor was genuinely too loud at +8 dB).
- "Any keys sound too strange" — string bloom used AttackT/k, making
  upper harmonics bloom much faster than f0.

### Fixes
1. **Modal group 2 cut below C5**: ×1.9 → ×0.4 for 60-71 (kills the
   "metal ring" the user heard below C5).
2. **Metal center curve**: C7 1850 → 2100 (≈f0), C6 1150 → 1150
   (unchanged), A7 2900 → 2900.
3. **Metal centerBoost** for C6+: Gaussian `exp(-(rel-1)²×2)` around
   rel=1 so the center partial dominates.
4. **Metal upper-particle taper** for C6+: `exp(-Max(0,rel-1.42)×0.8)`
   (was 1.0 = no taper).
5. **Modal group 3/4 cut for C7+**: group 3 ×11.5 → ×3, group 4 ×22 → ×4
   (their 14-20 partials overwhelmed the single metal center).
6. **String bloom sqrt(k)**: `tauK = min(AttackT/sqrt(k), bloomTau)`
   instead of `AttackT/k` — upper harmonics bloom closer to f0's time,
   not 2-4× faster.
7. **String decay scaling for C6+**: `highDecayScale = 1 + 1.5×(k-1)×
   (midi-84)/12` for K≥2 — makes 2nd harmonic decay faster so f0
   dominates the attack window.
8. **Floor gain reduced**: mid keys +8 dB → +4 dB, high keys +5.5 → −4.
9. **Metal metScale**: C5 ×0.65 (800-1500 was +5.6 dB hot), C7 ×3.0
   (boost f0 metal to compete with string 2nd harmonic).
10. **Rumble for 72-84**: 0.05 → 0.01 (eliminated sub-bass dominance
    at E5).

### Results (FFT, 4096-sample Hann window)
- C4: 527 Hz (2×f0) — matches sample 528 Hz ✓
- F#4: 375 Hz (f0) — matches sample 366 Hz ✓
- C5: 527 Hz (f0) — matches sample 528 Hz ✓
- E5: 656 Hz (f0) — fixed (was 160 Hz artifact / floor dominance) ✓
- C6: 1055 Hz (f0) — matches sample 1055 Hz ✓
- C7: 4230 Hz (2×f0) vs sample 2110 Hz (f0) — improved (was 4220
  with zero f0 energy, now 2109 is 2nd peak, gap narrowed from
  2:1 to 1.13:1). Sample has 2×f0 nearly equal to f0 (0.01293 vs
  0.01244), so this is close.

### Calibration
- Hammer/steady ratio: ±2.6 dB (47: −27.1 vs −24.5, 96: 11.5 vs 11.5).
- Peaks ≤0.952, no hiss 3-8 kHz.
- dist byte-identical, snapshot /tmp/synth-good-0824_metal-pitch-fix/.

### Remaining known deltas
- C4 800-1500: +7.9 dB over sample (string K=4 at 1047 Hz, amp 0.1266
  — sample data, not easily fixable without re-balancing all harmonics).
- C5 200-400: −5.3 dB under (floor reduction side effect).
- C7 6-10 kHz: −9.1 dB under (modal group 4 cut too hard).

## Session 28 — 2026-08-24: "чук-чук" double-transient fix

### User complaint
- "Стук какой-то короткий. Вместе со струной звучит какой-то чук-чук
  посторонний в районе E5 и выше."

### Diagnosis
Measured 1ms RMS windows of E5 (76) full note:
- ours: 0.11, **0.88, 1.00**, 0.18, 0.23, 0.27, 0.27, 0.28...
  → huge spike at 1-2ms, then drop to 0.18, then slow rise to 0.55 at 7ms.
  This is TWO transients: spike (strike) at 1-2ms, then string bloom at 5-8ms.

Root cause: the **strike** (region root 75, D5-E5) fires from t=0 at full
amplitude with no ramp. It's a separate damped resonator at f0 driven by
a 4ms contact-force pulse. It peaks at 1-2ms while the string blooms at
5ms → "чук-чук" (two separate knocks).

Also: the impact pulse edge had a spike at sample 0 (force[0]-0 = 1.0)
creating an additional click, but the strike was the dominant cause.

### Fix
1. **Strike delay + ramp** (AdditiveSampler.h + .cpp): the strike now
   starts after a 4ms delay with a 3ms smooth ramp-up. The string blooms
   first (5-9ms), then the strike joins at 4ms and ramps smoothly — no
   separate "чук" before the string.
2. **Impact pulse padding** (.cpp): added `force[0] = 0` leading zero so
   the edge derivative starts from 0 (no sample-0 click). Pulse widened
   from 0.6ms to 1.5ms for a gentler edge ramp.
3. **String bloom**: `tauK = bloomTauBase / sqrt(k)` (not `min(AttackT/
   sqrt(k), bloomTau)`) — the bloom tau is no longer overridden by fast
   AttackT values.

### Results (E5 1ms RMS)
Before: 0.11, 0.88, 1.00, 0.18, 0.23, 0.27, 0.27... (double transient)
After:  0.02, 0.12, 0.23, 0.26, 0.32, 0.85, 1.00... (single smooth rise)

Calibration: ±3.1 dB (47: −27.1 vs −24.0), peaks ≤0.94, no hiss.
dist byte-identical, snapshot /tmp/synth-good-0824_chuk-chuk-fix/.

## Session 29 — 2026-08-24: Hammer "щёлкает" / noise doesn't blend

### User complaint
- "Всё равно щёлкает, что-то не так с шумом молоточка, не вписывается
  он в струну!"

### Diagnosis
Measured hammer-only RMS 0.5ms windows: the envelope was FLAT (0.005-
0.013 constant) instead of rising-then-decaying like the sample residual
(0.008 → 0.036 → 0.014...). Root cause: **RMS normalization** of the
modal+metal body. The long modal tail (tau 50-110ms) diluted the RMS,
making the 0-5ms attack quiet and flat — the noise "щёлкало" (stepped on
the string as a constant bed, not a shaped transient).

### Fix
Changed body normalization from RMS to **peak**: `invPk = 1/max(|force|)`
instead of `invRms = sqrt(N/energy)`. This preserves the envelope shape:
the attack rises to its true peak, then decays — the noise now blends
with the string bloom instead of being a flat click.

Also confirmed the strike delay (Session 28) still works: E5 rises
smoothly 0.03 → 0.17 → 0.21 → 0.25 → 0.32 → 0.85 → 1.00 (single peak
at 5-6ms, no double transient).

### Results
- C4 hammer RMS: 0.0007, 0.0034, 0.0092, 0.0050, 0.0168, 0.0225...
  (clear rise → peak at 2.5ms → decay; was flat 0.003-0.013)
- C6 hammer RMS: 0.0105, 0.0251, 0.0273, 0.0177, 0.0257, 0.0331, 0.0563...
  (clear rise → peak at 3.5ms)
- E5 full note: single smooth attack (no чук-чук)
- Calibration: ±2.4 dB, peaks ≤0.942, no hiss.
dist byte-identical, snapshot /tmp/synth-good-0824_click-fix/.

## Session 30 — 2026-08-24: Noise balance + low-key attack

### User complaint
- "Слишком много шума везде. Ниже C4 его вообще нет и удар слишком
  чёткий. C3 вообще разные!"

### Diagnosis
1. Peak normalization (Session 29) made the hammer noise too loud —
   60-200 Hz was +5.5 to +19.6 dB over the residual on every key.
2. Mid-body (400-1500) was −9 to −13 dB under the residual — the
   hammer had too much sub-bass and not enough "knock" body.
3. C3 envelope: sample rises very slowly (0→0.07→0.05→0.15→0.35→0.73
   over 14ms); ours started at 0.11 immediately — too crisp.

### Fixes
1. **Reverted to RMS normalization** for the modal+metal body (peak-norm
   made everything too loud). Floor stays as a separate post-norm layer.
2. **Modal group 0 (85-400)**: gain 0.42 → 0.20 (less sub-bass boom).
3. **Modal group 1 (400-1300)**: gain 0.34 → 0.60 (more mid-body knock,
   matching the sample residual's 400-800 at −4 dB).
4. **Floor gain reduced**: mid keys +4 → +2 dB, high keys −4 → −10 dB.
5. **Rise time key-scaled**: C3 14ms, C4 8ms, C5+ 5ms (was 5ms fixed —
   C3 was too crisp, sample's C3 attack is very slow and dull).

### Results (full note vs raw SF2)
- C5 3-6k: +4.5 dB (was −3.0), 6-10k: +2.6 (was −6.4) — noise back.
- C6 6-10k: +13.4 (was −0.4) — high-end restored.
- C7 calibration: 0.0 dB, peaks ≤0.94.
dist byte-identical, snapshot /tmp/synth-good-0824_noise-balance/.
