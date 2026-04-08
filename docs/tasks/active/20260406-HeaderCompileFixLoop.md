---
title: "Header Compile Fix Loop"
status: "active"
created: 2026-04-06
started: 2026-04-06
updated: 2026-04-06
risk_level: low
related_files:
  - src/Intra
related_decisions: []
related_skills:
  - project-intra-compile-fix-loop
---

## Goal

Iterate through `./run test-header <Header>` (and small batches via `./run test-headers`) and fix compilation errors header-by-header until the default set compiles.

## Constraints

- Prefer minimal local fixes.
- Do not redesign APIs unless explicitly required.
- If a header requires a large refactor or unresolved design work, skip it for now and record it in `Skipped Headers`.

## Deterministic Checks

- `./run test-headers` succeeds (default set).

## Skipped Headers

- `StringUtils.h` — incomplete type `String` in `StringSprintf` (needs larger refactor / include order decisions).
- `Range/ByLine.h` — includes missing `Intra/Range/Concepts.h` (broken include path / file moved).
- `Range/TakeUntilAny.h` — includes missing `Intra/Range/Concepts.h` (broken include path / file moved).
- `Range/String/Ascii.h` — includes missing `Intra/Range/Span.h` (broken include path / file moved).
- `Range/String/Escape.h` — includes missing `Intra/Range/Concepts.h` (broken include path / file moved).
- `Range/Search/Subrange.h` — includes missing `Intra/Range/Concepts.h` (broken include path / file moved).
- `Range/Search/Distance.h` — includes missing `Intra/Range/Concepts.h` (broken include path / file moved).
- `Range/Sort/Quick.h` — includes missing `Intra/CompoundTypes.h` (broken include path / file moved).
- `Range/Mutation/Remove.h` — includes missing `Intra/Assert.h` (broken include path / file moved).
- `Range/Stream/Spaces.h` — includes missing `Intra/Range/Concepts.h` (broken include path / file moved).
- `Range/Split.h` — missing symbols (`TTakeResult`, `Take`, `IsLineSeparator`) and `Split` name collision; likely needs a larger refactor.

## Session Log

### Session 1 (2026-04-06)

- Done:
  - Restored compilation of `Range/StringView.h`.
  - Fixed unicode fast-path handling in `Range.h` / `StringView.h` without changing existing concepts.
- Skipped:
  - `StringUtils.h` (see above).
