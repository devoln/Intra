---
name: shared-task-closeout-discipline
description: Use before finishing non-trivial tasks to record concise persona review, explicit human-verification gaps, and safe handoff/closeout state.
---

# Task Closeout Discipline

Use this skill before declaring a non-trivial task done.

## When this skill applies

Use it for:
- medium/high-risk tasks;
- multi-step tasks;
- tasks with visual, UX, mobile, or subjective quality aspects;
- tasks where automatic checks do not prove everything;
- tasks likely to be resumed later.

## What to do

1. Run the most relevant deterministic checks.
2. Add a short `Persona Review` block using:
   - `BLOCKER`
   - `RISK`
   - `NOTE`
3. Add `Needs Human Verification` if anything remains unproven automatically.
4. Update session log with what was done and verified.
5. Leave a safe `Next-Step Handoff`.

## Review rule

- unresolved `BLOCKER` means the task is not really done;
- `RISK` should be either mitigated or explicitly surfaced;
- `NOTE` is non-blocking but still worth preserving.

## Human verification rule

Do not bury verification gaps in prose.
Keep them in one compact checklist.

Required fields:
- `Priority`
- `Area`
- `Why human`
- `Steps`
- `Expected`
- `Observed by agent`
- `Devices / environments` when relevant

## Keep it concise

- The closeout should be short and scannable.
- Prefer a compact checklist over long narrative.

## References

- `../shared-human-verification-queue/SKILL.md`
- `../../core/Personas.md`
- `../../templates/WorklogTemplate.md`
