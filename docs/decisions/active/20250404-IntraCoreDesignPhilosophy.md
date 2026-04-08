---
title: "Intra Core Design Philosophy"
status: "active"
created: 2025-04-04
updated: 2025-04-04
task: "docs/tasks/active/20250404-IntraCoreRefactoring.md"
tags: [philosophy, design-principles, constraints]
supersedes: []
superseded_by: []
---

# Decision

## What Was Decided

Establish the core design philosophy and constraints for the Intra library that differentiate it from standard C++ approaches and guide all implementation decisions.

## Context

Intra is a C++20 library designed for high-performance, minimal binary footprint, and maximum reusability. It targets multiple platforms (Linux, Windows, macOS, Android, FreeBSD) and multiple compilers (GCC 10+, Clang 13+, MSVC 2019.11+). The library must work in constrained environments including bare metal and systems programming contexts.

## Core Principles

### 1. No Standard Library Dependency

- **STL is prohibited** - No use of `std::` containers, algorithms, or utilities
- **No standard headers** - `#include` of standard headers is forbidden
- **Custom implementations** - All fundamental utilities (strings, arrays, algorithms) are implemented internally
- **Rationale**: Full control over binary size, performance characteristics, and behavior

### 2. Minimal C Runtime Dependency

- **Avoid CRT where possible** - Library core usable without C runtime
- **Optional CRT features** - Comfort features like formatted output are optional
- **Custom startup** - Support for bare metal and minimal runtime environments
- **Platform abstraction** - Direct system calls preferred over CRT wrappers
- **Rationale**: Smaller binaries, better portability, reduced attack surface

### 3. Template-First Architecture

- **Compile-time polymorphism** - Templates are the default abstraction mechanism
- **Zero-cost abstractions** - Runtime cost should match hand-written code
- **Automatic runtime wrappers** - Template code can be automatically wrapped for runtime polymorphism
- **Amortized call costs** - Runtime wrappers use batching and buffering to minimize virtual call overhead
- **Rationale**: Maximum performance with flexibility to fall back to runtime polymorphism when needed

### 4. Binary Size Minimization

- **Small code footprint** - Aggressively minimize generated code size
- **Outline large methods** - Force outline (`INTRA_NOINLINE`) for large template instantiations
- **Type-erased implementations** - Common implementations shared via type erasure
- **ExecutionPolicy control** - Template parameter controlling inline vs outline expansion (see ExecutionPolicy decision)
- **Rationale**: Suitable for embedded, demos (demoscene), and size-constrained environments

### 5. Maximum Performance

- **Compiler Explorer verification** - All performance-critical code verified on godbolt.org
- **Assembly inspection** - Generated assembly must be optimal
- **SIMD utilization** - Aggressive use of SIMD where beneficial
- **Branchless algorithms** - Prefer branchless implementations for hot paths
- **Cache-conscious design** - Data structures designed for cache efficiency
- **Rationale**: Performance is a primary design goal, not an afterthought

### 6. Aggressive Generalization

- **DRY at the limit** - Maximum reuse through template metaprogramming
- **Unified abstractions** - Single implementation covers multiple use cases
- **Concept-based design** - C++20 concepts for clean constraints
- **Rationale**: Less code to maintain, more consistent behavior, better optimization opportunities

### 7. Range-Centric API

- **Everything is a range** - Arrays, strings, I/O are all ranges
- **Composability** - Ranges compose naturally
- **Lazy evaluation** - Range operations are lazy where possible
- **Rationale**: Unified interface, better optimization through fusion

### 8. Multi-Compiler Support

- **Minimum versions**: GCC 10+, Clang 13+, MSVC 2019.11+
- **C++20 standard** - Full C++20 support required
- **Feature detection** - Conditional features based on compiler capabilities
- **Workarounds documented** - Known compiler limitations explicitly handled
- **Rationale**: Portability across toolchain ecosystems

### 9. String as UTF-32 Range

- **Universal encoding** - Strings are ranges of UTF-32 code points
- **Internal representation** - Can be UTF-8, UTF-16, or UTF-32
- **Transparent access** - `RawUnicodeUnits()` for internal representation
- **Rationale**: Correct Unicode handling without complexity at usage sites

### 10. Custom Allocator Integration

- **Allocator-aware containers** - All containers accept custom allocators
- **Inplace capacity** - Small Buffer Optimization (SBO) via template parameters
- **External memory** - Containers can work with externally provided memory
- **Zero-allocation paths** - Stack-only usage where possible
- **Rationale**: Memory control critical for performance and embedded use

### 11. Provenance-Aware Memory Handling

- **Strict aliasing compliance** - Correct use of `std::launder` and `start_lifetime_as`
- **Object lifetime tracking** - Respect C++ object lifetime rules
- **Placement new safety** - Safe in-place construction/destruction
- **Rationale**: Undefined behavior avoidance, optimizer cooperation

### 12. No External Headers Without Justification

- **Standard headers forbidden by default** - No `#include <...>` in core library
- **System headers only when essential** - Platform-specific code may need them, but must be isolated
- **Header conflict safety** - Custom declarations must work both with and without system headers included
- **Namespace isolation** - Use `z_D` namespace for platform declarations to avoid conflicts
- **Rationale**: Full control over dependencies, predictable compilation, no implicit includes from toolchain

### 13. Provenance-Aware Memory Handling

- **Strict aliasing compliance** - Correct use of `std::launder` and `start_lifetime_as`
- **Object lifetime tracking** - Respect C++ object lifetime rules
- **Placement new safety** - Safe in-place construction/destruction
- **Rationale**: Undefined behavior avoidance, optimizer cooperation

## Options Considered

- `A:` Use standard library containers - Rejected: loses control over performance and binary size
- `B:` Header-only library - Rejected: compilation time explosion, code bloat
- `C:` Runtime polymorphism by default - Rejected: virtual call overhead unacceptable for hot paths
- `D:` (Selected) Template-first with controlled runtime fallbacks

## Why This Path Was Chosen

The selected approach provides:
- Maximum performance through compile-time optimization
- Minimal binary size through controlled template instantiation
- Flexibility to use runtime polymorphism where appropriate
- Full control over all aspects of the implementation

## Consequences

### What Became Simpler
- Single implementation per algorithm (templated)
- Consistent behavior across all types
- Optimal performance by default

### What Became More Complex
- Cannot use off-the-shelf libraries
- Must implement all fundamental utilities
- Higher expertise required for contributors
- More rigorous testing needed

### New Constraints
- All code must compile with GCC 10+, Clang 13+, MSVC 2019.11+
- All performance-critical code must pass Compiler Explorer review
- No standard headers allowed
- Binary size regressions must be justified

## What Should Change Downstream

- **Skills to create**: 
  - `shared-asm-verification` - Require godbolt.org checks
  - `shared-binary-size-guard` - Check for code bloat
  - `shared-no-stl-check` - Verify no standard headers included
- **Templates to update**: WorklogTemplate.md needs Compiler Explorer verification section
- **CI to configure**: Automated assembly output comparison for critical paths

## Review Notes

- `Architect` approved: Clean separation of concerns
- `SystemsOptimizer` approved: Zero-cost abstraction achievable
- `RISK`: High expertise barrier for contributors
- `RISK`: Longer development time for new features
- `RISK`: Testing burden higher due to custom implementations
