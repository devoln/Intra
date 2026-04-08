---
name: project-intra-compile-fix-loop
description: Compile Intra headers one by one, identify failures, classify fixes, and either fix immediately or create documented decisions for complex refactoring.
---

# Intra Compile-Fix Loop

Use this skill when working on fixing Intra library compilation errors.

## Overview

Intra core must be compilable header-by-header without CMake. This skill guides the process of:
1. Selecting the next header to compile (simplest first, dependency-ordered)
2. Attempting compilation
3. Classifying errors
4. Making decisions: immediate fix OR create task + decision document

## When to Use

Use when:
- Intra headers fail to compile
- Working on core refactoring
- Need to unblock container development

Stop when:
- All headers in `src/Intra/` compile cleanly
- Ready for CMake integration

## Compilation Command

```bash
# For single header test, create minimal test file:
echo '#include <Intra/HeaderName.h>' > /tmp/test.cpp

# Compile with Clang (preferred for error clarity):
clang++ -std=c++20 -I src -c /tmp/test.cpp -o /tmp/test.o 2>&1

# Or GCC:
g++ -std=c++20 -I src -c /tmp/test.cpp -o /tmp/test.o 2>&1
```

## Header Selection Order

Prefer headers in this order:
1. **Leaf headers** - Few dependencies, few includes
2. **Foundation headers** - `Concepts.h`, `Meta.h`, `Preprocessor.h`
3. **Utility headers** - `LifeCycle.h`, `Ownership.h`
4. **Core abstractions** - `Range.h`, `Container.h`
5. **Concrete containers** - `Container/Sequential/*.h`
6. **Complex systems** - `Allocator.h`, `Core.h`

Check dependencies with:
```bash
grep -h "^#include.*Intra/" src/Intra/*.h src/Intra/**/*.h | sort | uniq -c | sort -rn
```

## Error Classification

### Type A: Quick Fix (< 5 minutes)
- Missing typename keyword
- Wrong template syntax
- Simple renaming (e.g., ArrayRange → Span)
- Missing include guard
- **Action:** Fix immediately, commit

### Type B: Missing Implementation (5-30 minutes)
- Incomplete function body
- Missing concept definition
- Unfinished template specialization
- **Action:** Fix with minimal viable implementation, commit

### Type C: Design Decision Required (> 30 minutes or architectural)
- API redesign needed
- Template parameter strategy unclear
- Integration between multiple systems (e.g., Allocator ↔ Container)
- Breaking change to existing code
- **Action:** Create task document + possibly decision document, discuss with owner

## Immediate Fix Workflow (Type A/B)

1. Identify error from compiler output
2. Locate in source file
3. Apply minimal fix
4. Recompile to verify
5. If fixed: commit with message pattern: `Fix: <header> - <brief description>`
6. If more errors: classify and repeat

## Decision Required Workflow (Type C)

1. Create task document in `docs/tasks/active/YYYYMMDD-TaskName.md`
2. Document in task:
   - Current error
   - What needs to be decided
   - Options considered (if obvious)
   - Why this is Type C (complexity, dependencies)
3. If architectural decision needed: create `docs/decisions/active/YYYYMMDD-DecisionName.md`
4. **STOP** and discuss with owner
5. After decision: update task, proceed with implementation

## Decision Trigger Checklist

Create a decision document if ANY of these apply:
- [ ] Changes public API (visible to users of library)
- [ ] Affects multiple subsystems (Allocator + Container + Range)
- [ ] Introduces new concept or policy
- [ ] Has performance implications (needs Compiler Explorer check)
- [ ] Owner has strong opinions on approach

Just fix immediately if ALL of these apply:
- [ ] Internal implementation detail only
- [ ] No API change
- [ ] Follows existing pattern
- [ ] Straightforward correctness

## Progress Tracking

Update the main worklog at end of session:
```markdown
## Session Log

### Session N (YYYY-MM-DD)

- Done: Fixed <header1>, <header2> (Type A)
- Done: Created task <task-name> for <complex-issue> (Type C)
- Next: <next header or task>
```

## Success Criteria

- All `src/Intra/*.h` compile standalone
- No standard headers transitively included
- No CRT dependencies in core
- Ready for CMake integration
