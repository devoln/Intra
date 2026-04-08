---
title: "Range Design Decision Trace"
status: "active"
created: 2026-04-07
updated: 2026-04-07
task: "docs/tasks/active/20260407-RangeDesign.md"
tags:
  - range
  - lifetime
  - ownership
  - view
  - matchers
supersedes:
  - "docs/decisions/active/20250405-RangeOfOwningRange.md"
  - "docs/decisions/active/20260405-RangeMapFunctorOwnership.md"
  - "docs/decisions/active/20260406-MatcherAdvancesRange.md"
superseded_by: []
---

# Decision Trace

This document consolidates range-related decision traces into a single trace.

## Terminology

- **View list** (`TagViewList` / `CViewList`): may reference elements it does not own; element lifetime depends on referenced objects.
- **View range** (`CViewRange`): `CViewList && CRange`.
- **Owning list** (`COwningList` / `TagOwningList`): may own elements it returns from `First/Last`.
- **Predicate**: callable used as `bool(pred(value))`.
- **Matcher**: callable used as `bool(matcher(range))` and may advance/mutate the range on success.
- **Rewrite** (future): streaming rewrite / replacement producing 0..N output elements per match.

## D-1: `RangeOf` must lift rvalue owning lists into `OwningRange`

### Context

`RangeOf` is used pervasively to normalize user inputs into a `CRange`. For rvalue owning containers, returning a view produces dangling references.

Key constraints:

- Zero-cost abstractions.
- No standard library headers.
- Lvalue containers must stay view-like (no unexpected ownership changes).
- Rvalue owning containers must not produce dangling references.

Observed failure mode:

- `RangeOf(Vector{}).First()` (or used inside an adaptor) may return references into a destroyed temporary if `RangeOf` returns a view for rvalues.

### Decision

- If `RangeOf` is called with `CRValueReference<L> && COwningList<L>`, it returns `OwningRange<TRemoveReference<L>>{...}`.
- Otherwise `RangeOf` delegates to an internal implementation (`RangeOfImpl`) to avoid dependency cycles.

### Rationale

- Avoids dangling by default.
- Preserves zero-overhead views for lvalues.

### Consequences

- Some algorithms must be aware that owning has been introduced by `RangeOf`.

### Options considered

- `A:` Always return a view (fastest, but unsafe for rvalue owning containers).
- `B:` Always return an owning wrapper (safe, but breaks view semantics for lvalues and can be expensive).
- `C:` Owning only for `CRValueReference<L> && COwningList<L>` (chosen).

### Notes

- The owning wrapper is expressed as a range (`OwningRange<C>`) rather than special casing every adaptor.

## D-2: Break the `COwningList` / `RangeOf` dependency cycle via `RangeOfImpl`

### Context

`COwningList` depends on list concepts that depend on `RangeOf`.

Concrete cycle (simplified):

- `COwningList` needs `CGrowingList`
- `CGrowingList` needs `CList`
- `CList` needs `TRangeOfRef`
- `TRangeOfRef` needs `RangeOf`
- `RangeOf` needs `COwningList`

### Decision

- Define `z_D::RangeOfImpl` (owning-agnostic) and define `CList` in terms of it.
- Define public `RangeOf` on top of `RangeOfImpl`.

### Rationale

- Breaks the cycle without weakening concepts.
- Keeps owning logic in exactly one public place (`RangeOf`).

### Consequences

- Two functions exist (`RangeOfImpl` and `RangeOf`), but `RangeOfImpl` is internal and owning-agnostic.
- `CList`/list concepts are defined using `RangeOfImpl` to keep compile-time dependency order sane.

## D-3: Adaptors store functors by value by default; explicit reference wrapper is required

### Decision

- `RMap` stores functors by value.
- `FunctorOf` returns an owning callable object, selecting move vs copy based on constructibility.
- Explicit escape hatch for non-movable / non-copyable functors: `FRef(f)`.

### Rationale

- Safety by default (no implicit refs to temporaries).
- Zero-cost: no allocation/type-erasure.

### Options considered

- `A:` Store functors by reference by default.
  - Rejected: unsafe for temporaries; pushes lifetime management onto every call site.
- `B:` Always perfect-forward into stored value.
  - Rejected: fails for split usage patterns and for copy-only / no-move-no-copy functors.
- `C:` Store by value by default + explicit reference wrapper (chosen).

### Consequences

- Default pipeline usage is safe: `list | Map(f)`.
- Advanced use cases must opt in explicitly: `Map(FRef(f))`.

## D-4: Matcher model is "advances range on success"

### Decision

- A matcher is callable as `bool(matcher(range))` and may advance/mutate the range on success.
- Lookahead/backtracking require forward/copyable ranges or explicit buffering.
- Repetition requires a progress guarantee.

### Rationale

- Supports streaming/input patterns.
- Avoids fragile `{Matched, Advance}` protocols.

### Options considered

- `A:` Matcher returns `{Matched, Advance}` and does not mutate the range.
  - Rejected: introduces a fragile progress protocol (`Advance == 0` can lead to infinite loops under repetition); complicates input-range support.
- `B:` Matcher mutates/advances a range on success (chosen).

### Consequences

- Lookahead/backtracking must be explicit (copy/commit or buffering).
- Search/consume algorithms can share a small set of primitives built on the matcher contract.

## D-5: View/owning tagging

### Decision

Introduce type-level tags and conservative propagation across pipelines:

- `TagViewList`: the list/range may reference elements it does not own.
- `TagOwningList`: the range/list may own elements it returns.

`TagViewList` and `TagOwningList` are orthogonal and may both apply to a type.

### Rationale

- View-ness is a property of the type (not runtime state) and should be trackable across long pipelines.
- Ownership can be introduced (e.g. buffering, owning wrappers) without removing view-ness.

### Consequences

- Adaptors must conservatively propagate these properties.
- Properties should generally become stricter through lazy pipelines unless an eager materialization explicitly changes the model.

## D-6: `ViewRangeOf` as the primitive for temporary navigation

### Decision

Introduce `ViewRangeOf` to obtain a local view used for lookahead/look-behind in algorithms.

### Context

Some algorithms need temporary navigation without committing state (e.g. `Filter::Last()` scanning backwards for the last element satisfying a predicate).

Naively copying a range (`auto copy = range;`) is problematic when:

- the range is owning-like (copy can be expensive or change lifetime expectations)
- reference validity depends on the range object's lifetime

### Rationale

`ViewRangeOf` gives a single explicit mechanism for "temporary navigation" that can be specialized per list/range category without rewriting algorithms.

### Consequences

- Algorithms that previously copied ranges for lookahead/look-behind should prefer `ViewRangeOf`.

Implemented behavior:

- Prefer `Span` for lvalue `CConvertibleToSpan` inputs.
- Otherwise, prefer `RIndexedRef` for lvalue `CRandomAccessList` inputs (index-based independent iteration state).
- Otherwise, fallback to `RangeOf(INTRA_FWD(list))`.

## D-7: `Filter::Last()` policy (selected)

### Decision

Policy `[C]`:

- For `!COwningList<R>`: `Last()` returns a reference (via `decltype(auto)`), using `ViewRangeOf` rather than copying the range.
- For `COwningList<R>`: `Last()` returns `TRangeValue<R>` (by value).

### Rationale

- Zero-overhead reference access for view-like ranges.
- Correctness for owning-like ranges.
- Diagnostics and explicit contracts instead of hidden safety costs.

### Options considered

- `A:` Always return by value (safe but can pessimize view ranges).
- `B:` Always return by reference (fast but not correct for owning-like ranges and look-behind implementations that require a temporary view).
- `C:` Conditional return type based on ownership properties (chosen).

### Consequences

- View-like ranges keep zero-overhead reference access.
- Owning-like ranges avoid returning references tied to a temporary owning wrapper.
- Requires explicit tagging/constraints to keep the rule checkable.

## D-8: MatcherOf fallback classification must accept already-a-matcher inputs

### Decision

When classifying needles passed into search/consume algorithms, `MatcherOf` must first accept "ready" matchers directly:

- If the input is callable as `bool(x(range))`, it is treated as a matcher as-is.

Only if it is not already a matcher, fallback classification proceeds with:

- Equality-comparable with element type -> equality matcher.
- Callable predicate on element -> element predicate matcher.
- Otherwise treat as subrange -> subrange matcher.

### Rationale

This makes "subrange" just one possible way to build a matcher. Users (and internal algorithms) can provide explicit matchers that are more general than subrange matching.

### Options considered

- `A:` Only accept values/predicates/subranges and always wrap into matchers.
  - Rejected: blocks composition and forces needless wrapping for already-correct matchers.
- `B:` Accept ready matchers first, then fallback classify other inputs (chosen).

### Consequences

- Fallback classification is strictly more general.
- Subrange matching is just one adapter among many possible matcher adapters.

## What Should Change Downstream

- Introduce `TagViewList`/`CViewList`/`CViewRange` and `ViewRangeOf` in `Concepts.h` and propagate tags through range adaptors.
- Update algorithms that currently copy ranges for temporary navigation to use `ViewRangeOf`.
- Keep matcher/search work built on the "matcher advances range" contract and the updated `MatcherOf` fallback classification.

## Review Notes

- `RISK:` Tag propagation mistakes can cause subtle lifetime bugs. Prefer conservative propagation and add constexpr tests.
- `RISK:` `COwningList` is an approximation; hybrids exist. The design must remain explicit about trade-offs.
- `NOTE:` Keep user-facing APIs pipe-friendly; type-parameter-heavy utilities like `MatcherOf<R>` should remain internal to algorithm implementations.
