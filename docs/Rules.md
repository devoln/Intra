# Intra Project Rules

This document contains the non-negotiable core rules and invariants for the Intra project. All contributors must read and follow these rules. Failure to follow just one rule will result in rejection of the contribution.

## A: Core Philosophy

1. **Zero-Cost Abstractions**: Every feature must be usable without runtime overhead compared to hand-written C code.
2. **No Standard Library Dependency**: Intra must not include ANY standard library or system headers like `<string.h>` or `<Windows.h>`. Instead, we wrap compiler builtins and write our own declarations in `Toolchain.h` in a way that avoids conflicts with system headers. Exceptions are allowed only for ABI-specific code for OS other than Windows, Linux, Android, FreeBSD, and all Apple OSes.
3. **Compile-Time First**: Everything that can be `constexpr`, must be. Static checks are preferred over runtime checks. There may be optimizations under `if(!IsConstantEvaluated(...))` that do the same at runtime but faster than in constant expressions or using less memory (e.g. agressive packing and tagged pointers).
4. **Explicit Unsafe**: Unsafe operations require a `TUnsafe` parameter (or `Unsafe` constant) passed explicitly. No implicit unsafe operations in public interfaces.
5. **Fix Compiler Warnings**: Whenever you see a warning, fix it. If not possible, suppress it locally. See `Core.h` how the warnings are enabled and how they can be suppressed in `INTRA_BEGIN`/`INTRA_END` section.

## B: Code Style & Structure

1. **Concepts Over SFINAE**: Use C++20 concepts for all template constraints. Avoid SFINAE.
2. **Implementation Namespace**: All implementation details go in `Intra::z_D` namespace. Public API must be clean.
3. **Macro Hygiene**: Public macros use `INTRA_` prefix. Internal macros use `INTRAZ_D_` prefix.
4. **No Exceptions for Control Flow**: Error handling uses `Result<T>`, `Optional<T>`, or `ErrorCode`. Exceptions are for fatal errors only.
5. **Unify Similar Code Paths**: If two branches/paths contain identical or near-identical code, factor the common part into a shared block (helper, lambda adapter, local function object, etc.) and keep only the minimum selection logic in the branches. Avoid copy-pasting similar blocks (they tend to diverge and break asymmetrically).

## C: Architecture Invariants

1. **Platform Abstraction**: Direct OS API calls only in `Platform/` directory with consistent cross-platform interface.

## D: Testing

1. **Test Coverage**: All public constexpr APIs must have `#if INTRA_CONSTEXPR_TEST` block in the end of file with `static_assert` cases. Prefer red-green testing approach for bug-fixes and new features.

## Rule Violation Policy

If you think that you can't follow the rule, you must:
1. Research the codebase for similar cases when it could follow the rule.
2. Search online the way to follow the rule. The way may not be obvious, just keep trying different things even if some of them are platform/compiler-specific.
3. If nothing helps, discuss the issue with the project owner and get his approval.

## TODO: possible future rules (not enforced now)

1. Make sure that each function's ASM output is short and efficient in -O1/-Os/-O2 levels for g++/clang/MSVC. Hot functions may enforce higher optimization levels through `INTRA_OPTIMIZE_FUNCTION` and `INTRA_OPT_UNROLL_LOOP`.
2. Write Doxygen-style comments for public APIs (the exact format is TBD).
3. Test each public API with unit tests (using own unit-test framework that is TBD). Prefer red-green testing approach for bug-fixes and new features.
4. Add even more diagnostics with clang-tidy that must be followed.
