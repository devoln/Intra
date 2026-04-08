---
description: Compile-fix loop for Intra core - identify failing headers, classify errors, and fix or escalate to decision documents
---

# Intra Compile-Fix Workflow

## Purpose

Systematically fix Intra library compilation errors by:
1. Testing headers one at a time
2. Classifying errors (quick fix vs needs decision)
3. Either fixing immediately or creating task+decision documents

## Prerequisites

- Clang or GCC installed
- Intra repo at `/Users/devoln/Dev/Intra`

## Steps

### 1. Select Next Header

Prefer in dependency order (leaf first):
```bash
# Check include counts
grep -h "^#include.*Intra/" src/Intra/*.h src/Intra/**/*.h 2>/dev/null | sort | uniq -c | sort -rn | tail -20
```

Start with headers that have fewest includes:
- `Preprocessor.h` (usually safe)
- `Meta.h`
- `Concepts.h` (may need fixes)

### 2. Test Compilation

```bash
echo '#include <Intra/HeaderName.h>' > /tmp/test.cpp
clang++ -std=c++20 -I src -c /tmp/test.cpp -o /tmp/test.o 2>&1
```

### 3. Classify Errors

**Type A - Quick Fix (< 5 min):**
- Missing `typename`/`template` keywords
- Syntax errors (missing comma, wrong bracket)
- Simple naming mismatches
- Missing include guards
- **Action:** Fix immediately, commit with `Fix: <header> - <description>`

**Type B - Missing Implementation (5-30 min):**
- Incomplete function body
- Missing concept definition
- Unfinished template
- **Action:** Add minimal implementation, commit

**Type C - Design Decision Required:**
- API redesign needed
- Breaking change to existing code
- Multiple subsystem integration (Allocator ↔ Container)
- Complex template strategy
- **Action:** Create task document, discuss with owner

### 4. Error Classification Checklist

Create decision document if ANY apply:
- [ ] Changes public API
- [ ] Affects multiple subsystems
- [ ] New concept/policy introduced
- [ ] Performance implications (needs Compiler Explorer)

Fix immediately if ALL apply:
- [ ] Internal implementation only
- [ ] No API change
- [ ] Follows existing pattern
- [ ] Straightforward correctness

### 5. Fix and Verify

```bash
# After fix, recompile
echo '#include <Intra/HeaderName.h>' > /tmp/test.cpp
clang++ -std=c++20 -I src -c /tmp/test.cpp -o /tmp/test.o 2>&1
```

If clean → commit.
If more errors → classify and repeat.

### 6. Document Progress

Update main worklog (`docs/tasks/active/20250404-DocumentActiveTasksAndDecisions.md`):
```markdown
### Session N (YYYY-MM-DD)

- Done: Fixed <header> (Type A: <description>)
- Done: Created task <task-name> for <complex-issue> (Type C)
- Next: <next-header>
```

## Platform-Specific Notes

### macOS
- System headers may be included implicitly by toolchain
- Use `struct tm*` not `tm*` to match system declarations
- Forward declarations for structs must be in global namespace

### Linux
- GCC may have different implicit includes than Clang
- Test with both compilers if possible

### Windows (MSVC)
- Use `/std:c++20` flag
- Different CRT function signatures

## Success Criteria

- All `src/Intra/*.h` compile standalone
- No standard headers transitively included
- Ready for CMake integration

## Common Issues

**Conflict with system headers:**
- Symptom: `conflicting types for 'gmtime'`
- Solution: Match system declaration exactly (e.g., `struct tm*`)

**Missing struct forward declaration:**
- Symptom: `unknown type name 'timespec'`
- Solution: Add `struct timespec;` in global namespace before use

**Namespace confusion:**
- Symptom: `no type named 'X' in 'Intra::z_D'`
- Solution: Check if declaration is inside `namespace Intra { INTRA_BEGIN }`
