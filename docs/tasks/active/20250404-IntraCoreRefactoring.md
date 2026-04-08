---
title: "Intra Core Refactoring - Arrays, Strings, Serialization"
status: "draft"
created: 2025-04-04
started: 2025-04-04
updated: 2025-04-04
risk_level: high
related_files:
  - src/Intra/Container/
  - src/Intra/Range/
  - src/Intra/Core.h
  - src/Intra/Meta.h
  - docs/notes/Первым делом/Починить ядро.md
related_decisions:
  - docs/decisions/active/20250404-IntraCoreDesignPhilosophy.md
related_skills:
  - shared-task-init-discipline
  - shared-worklog-discipline
  - shared-regression-proof-discipline
---

# Task

## Goal

Complete the core Intra library refactoring to achieve a compilable, coherent foundation that unblocks container development and downstream work (tests, IntraX migration). Current state: WIP commit "Core refactoring - Span, optimized ToString, new files" — partial implementation with compilation errors.

Key deliverables:
1. **GenericArray** - Complete array implementation with SBO, inplace capacity, external memory support
2. **GenericString** - UTF-32 range-based string built on GenericArray
3. **Serialization framework** - Binary and text serialization on Span<char> with callbacks
4. **String formatting** - Type-safe printf replacement using serialization infrastructure
5. **String scanning** - Type-safe scanf replacement
6. **Reader/Writer abstractions** - Buffered I/O with error handling

## Out of Scope

- IntraX migration (waits for stable Intra core)
- Full test suite (waits for compilable core)
- Optimized SIMD implementations (can be added later)
- Lock-free containers (future work)
- Hash map implementations (separate task)

## Loaded Skills

- `shared-task-init-discipline` - Task framing for high-risk core work
- `shared-worklog-discipline` - Multi-session coordination
- `shared-regression-proof-discipline` - Verify no silent breakages

## Milestones

- [ ] `M1:` Fix GenericArray compilation errors
  - Complete inplace capacity variant without union (`!D.IsCompact`)
  - Extract large methods to GenericArrayImpl with `INTRA_NOINLINE`
  - Support non-size_t length types (uint32 for string compactness)
  - Verify Compiler Explorer output matches std::vector performance

- [ ] `M2:` Refactor GenericString on GenericArray
  - Replace existing implementation with GenericArray-based
  - Default InplaceCapacity = 16
  - UTF-32 range interface, internal representation via `RawUnicodeUnits()`
  - StringView-like semantics

- [ ] `M3:` Implement serialization core
  - Span<char> with callback returning new Span
  - Low-level functions taking `char*` or `char*&` (no bounds check)
  - 32-byte buffer assumption for most operations
  - Recursive collection/tuple handling with size checks per element

- [ ] `M4:` Implement string formatting (ToString)
  - Based on text serializer
  - LanguageDesc switching per argument
  - Compact argument packing (64-bit value with flags + index + data/pointer)
  - Constexpr support

- [ ] `M5:` Implement string scanning (FromString)
  - Based on text deserializer
  - Symmetric to formatting

- [ ] `M6:` Implement Reader/Writer abstractions
  - `ReaderToRange` adapter
  - `BufferedReader`/`BufferedWriter` with proper I/O error handling
  - Non-blocking I/O support considerations

- [ ] `M7:` Verify clean compilation
  - All headers in src/Intra/ compile standalone
  - CMake Test.cc includes all headers
  - No CRT dependencies in core

- [ ] `M8:` Compiler Explorer verification
  - GenericArray vs std::vector assembly comparison
  - String formatting assembly inspection
  - Document findings in task

## Invariants

- `INV-1:` No standard headers (`#include <...>`) allowed in Intra core
- `INV-2:` All template code must show optimal assembly on godbolt.org
- `INV-3:` GenericArray code size must be <= 1.5x hand-written C array code
- `INV-4:` String operations must handle full Unicode without external libraries
- `INV-5:` Serialization must support memory-mapped file deserialization

## Deterministic Checks

- [ ] Compile with GCC 10+ `-std=c++20` (no errors)
- [ ] Compile with Clang 13+ `-std=c++20` (no errors)
- [ ] Compile with MSVC 2019.11+ `/std:c++20` (no errors)
- [ ] Run existing tests if any compile
- [ ] Verify no CRT calls in core headers (check with `nm` or similar)

## Reality Verification

- [ ] Create minimal test program using GenericArray, GenericString, formatting
- [ ] Test compiles and runs on Linux (GCC/Clang)
- [ ] Test compiles and runs on Windows (MSVC)
- [ ] Measure binary size of test program (< 50KB ideal)

## Persona Review

- `BLOCKER:` None identified yet
- `RISK:` GenericArray complexity may lead to compilation time explosion
- `RISK:` Serialization design may not handle all edge cases
- `NOTE:` Consider separate task for SIMD-optimized string operations

## Needs Human Verification

- `Priority:` high
  `Area:` GenericArray design completeness
  `Why human:` Owner knows exact requirements for SBO variants and use cases
  `Steps:`
  1. Review GenericArray template parameter design
  2. Verify inplace capacity covers all intended use cases
  3. Check alignment with planned hash map requirements
  `Expected:` Design covers all current and near-future use cases
  `Observed by agent:` Partial implementation exists, needs validation
  `Devices / environments:` Local dev, review code only

- `Priority:` medium
  `Area:` Compiler Explorer verification criteria
  `Why human:` Owner defines "optimal" assembly threshold
  `Steps:`
  1. Run godbolt comparisons for GenericArray vs alternatives
  2. Define acceptable code size/complexity thresholds
  3. Document in decision or skill
  `Expected:` Clear criteria for future automated checks
  `Observed by agent:` Not yet performed
  `Devices / environments:` godbolt.org

## Session Log

### Session 1 (2025-04-04)

- Done: Created task document based on notes from docs/notes/Первым делом/Починить ядро.md
- Done: Referenced related decision on Intra Core Design Philosophy
- Next step: Begin M1 - fix GenericArray compilation errors

## Surprises

- (Pending) Compilation errors may reveal unexpected dependencies
- (Pending) GenericArray complexity may exceed initial estimates

## Deferred

- SIMD-optimized algorithms (separate task)
- Hash map implementations (separate task)
- Full Unicode normalization (beyond basic UTF-32 handling)
- Async I/O integration with coroutines (future architecture decision)

## Next Safe Step

Start M1: Analyze current GenericArray compilation errors and fix incrementally. Target: single header compiles standalone.
