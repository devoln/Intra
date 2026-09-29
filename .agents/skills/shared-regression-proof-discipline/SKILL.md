---
name: shared-regression-proof-discipline
description: Use when fixing a reported bug or regression that must be proven by a failing reproduction before claiming a fix. Enforces a red-green-proof loop instead of speculative patching.
---

# Regression Proof Discipline

Use this skill when the user reports a concrete bug, regression, or broken interaction and expects a reliable fix.

## Primary rule

Do not return claiming the issue is fixed until all of these are true:
- the bug has been reproduced deterministically;
- there is a concrete check that fails on the bad behavior;
- the implementation changes make that same check pass;
- the check is kept as regression coverage when practical.

## Required loop

1. Reproduce first.
2. Convert the reproduction into the smallest stable failing check you can.
3. Run it and confirm it fails for the reported reason.
4. Only then change code.
5. Re-run the same check and make it pass.
6. Re-run neighboring checks that protect previously good behavior.

## What counts as acceptable proof

Prefer, in this order:
- an automated test that fails before the fix and passes after;
- an existing test tightened so it demonstrably fails before the fix;
- a deterministic scripted reproduction with machine-verifiable output;
- only if none of the above is possible, an explicit human-verification item with a clear statement that no deterministic proof was possible.

## Forbidden shortcuts

Do not:
- guess at a fix and only add a permissive test afterward;
- write a test that asserts internal state unrelated to the user-visible bug;
- replace the reported behavior with weaker assertions just to get green;
- say the bug is fixed if you did not see the failing reproduction first.

## For frontend and interaction bugs

Prefer browser-level proof for:
- scrolling, zooming, dragging, resizing, focus, overlays, markers, and chart behavior;
- timing-sensitive interactions where unit tests can miss the real failure.

If debug state is needed:
- expose the minimum state needed to prove the user-visible invariant;
- verify that the debug field actually changes when the UI changes;
- do not substitute debug-only success for a still-broken visible interaction.

## Communication rule

Before editing, tell the user:
- how you reproduced it;
- what failing check now captures it.

At closeout, report:
- which check failed before;
- which code changed;
- which check now passes;
- any remaining gap that still needs human eyes.

## If you cannot reproduce

Stop claiming progress on the fix.
Instead:
- report that the bug is not yet reproduced;
- show what you tried;
- add instrumentation or a better reproduction path first;
- only return to “fixed” language after a deterministic repro exists.
