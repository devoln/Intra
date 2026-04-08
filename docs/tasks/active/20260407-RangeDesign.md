---
title: "Range Design: Lifetime, Ownership, Views, and Matchers"
status: "active"
created: 2026-04-07
started: 2026-04-05
updated: 2026-04-07
risk_level: high
related_files:
  - src/Intra/Concepts.h
  - src/Intra/Range.h
  - src/Intra/LifetimeTests.h
related_decisions:
  - docs/decisions/active/20260407-RangeDesign.md
related_skills:
  - shared-decision-discipline
  - project-intra-compile-fix-loop
---

# Task

## Goal

Unify and formalize the Intra Range design so that lifetime/ownership/view semantics are explicit, conservative across pipelines, and usable by range algorithms (including matcher-based search).

## Out of Scope

- Large refactors of unrelated headers.
- Parser refactors (`TextDeserializer`) beyond defining reusable matcher primitives.

## Constraints / Invariants

- `INV-1:` Zero-cost abstractions (wrappers must inline; no dynamic allocation).
- `INV-2:` No standard library headers.
- `INV-3:` Safety via explicit contracts, compile-time constraints, diagnostics, and clear semantics.
- `INV-4:` Tagging must be composable across pipelines and should be conservative (properties may only become stricter unless the pipeline explicitly materializes into an owning container).

## Milestones

- [x] `M0:` `RangeOf` lifts rvalue owning lists into `OwningRange` (implemented).
- [x] `M0:` `RangeOfImpl` delegation to break dependency cycles (implemented).
- [x] `M0:` `COwningList` concept defined (implemented).
- [x] `M0:` `Map` / `FunctorOf` default owning semantics with explicit `FRef` escape hatch (implemented).
- [x] `M0:` Matcher model decision: matcher advances range on success (decided).

- [x] `M1:` Add view tagging and concepts (`TagViewList`, `CViewList`, `CViewRange`).
- [x] `M2:` Implement `ViewRangeOf` (Span -> `RIndexedRef` -> `RangeOf`).
- [x] `M3:` Re-implement `Filter::Last()` via policy `[C]` using `ViewRangeOf`.
- [x] `M4:` Ensure adaptors conservatively propagate view/owning tags (`TagViewList` + `TagOwningList`).
- [ ] `M5:` Continue matcher/search unification work on top of these contracts.

## Deterministic Checks

- [x] `./run test-headers ...` for `Concepts.h`, `Range.h` and affected headers.

## Reality Verification

- [x] Affected headers compile.
- [ ] `#if INTRA_CONSTEXPR_TEST` blocks contain `static_assert` coverage for the important cases introduced/changed by this task, and those asserts pass.

## Persona Review

- `BLOCKER:` none
- `RISK:` This task changes type-level lifetime/ownership contracts; missing a propagation edge can introduce dangling references or accidental copies.
- `NOTE:` Prefer proving behavior via targeted constexpr tests (in existing test headers) rather than broad refactors.

## Needs Human Verification

- `Priority:` medium
  `Area:` Integration with real IntraX containers and long pipelines
  `Why human:` Compile-time checks do not fully cover runtime performance/copy behavior for all concrete container types.
  `Steps:`
  1. Run a minimal demo that pipes an owning container rvalue through adaptors that use `ViewRangeOf`.
  2. Confirm no unexpected deep copies (where observable) and no dangling references.
  3. Confirm behavior on both view-like and owning-like inputs.
  `Expected:` `ViewRangeOf` prevents accidental owning copies in lookahead/look-behind; `Filter::Last()` policy matches `COwningList`.
  `Observed by agent:` Header compile verified via `./run test-headers`.
  `Devices / environments:` Any supported build environment

## Session Log

### Consolidated history

- Implemented `OwningRange` and `RangeOfImpl` delegation to break dependency cycles.
- Defined `COwningList` to detect owning containers.
- Established owning-by-default functor storage in `Map`/`RMap`, with explicit reference wrapper `FRef`.
- Began design for matcher-based range search, including matcher-advances-range contract.

## Surprises

- `RangeOf` / `COwningList` introduced a dependency cycle that required `RangeOfImpl`.
- Look-behind APIs (e.g. `Filter::Last()`) surface hidden lifetime/ownership assumptions and must be handled via explicit tagging + `ViewRangeOf`.

### Implementation notes (2026-04-07)

- Implemented `TagViewList`/`CViewList`/`CViewRange`.
- Implemented `RIndexedRef` (for `CRandomAccessList`) and integrated it into `ViewRangeOf`.
- Implemented `ViewRangeOf` as a unified entry point: prefer `Span` (lvalue), fallback to `RIndexedRef` (lvalue random-access list), otherwise fallback to `RangeOf`.
- Updated range adaptors in `Range.h` to propagate `TagViewList` and `TagOwningList`.
- Updated `RFilter::Last()` to policy `[C]` using `ViewRangeOf`.

### Matcher notes (2026-04-07)

- `PopWhile` is the base primitive that gains matcher support.
- Matchers are required to advance the range on success.
- Input-range-safe matchers are explicitly tagged (`TagInputSafe`) and are the only matchers allowed in `PopWhile` matcher branch.
- `PopWhile` matcher branch is single-pass: no lookahead/backtracking/copying; it counts popped elements by wrapping the range in `RCounter` over `RRef`.

## Next Safe Step

- Start matcher-based unification:
  - Introduce matcher concepts (`CMatcher`, `CMatcherInput`) and `MatcherOf` fallback classification.
  - Refactor `Contains`/`Count`/`DropUntil`/`TakeUntil` to accept matchers and remove duplicate subrange search loops.
