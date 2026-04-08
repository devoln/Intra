---
title: "<Short task name>"
status: "draft"
created: <YYYY-MM-DD>
started: <YYYY-MM-DD>
updated: <YYYY-MM-DD>
risk_level: low
related_files: []
related_decisions: []
related_skills: []
---

# Task

## Goal

- What should change after the task is completed.

## Out of Scope

- What explicitly is not part of this task.

## Loaded Skills

- `shared-...` — why this skill is needed
- `project-...` — why this skill is needed

## Milestones

- [ ] `M1:` ...
- [ ] `M2:` ...

## Invariants

Fill for `medium` and `high` level tasks, as well as when there is real risk of breaking important behavior.

- `INV-1:` ...
- `INV-2:` ...

## Deterministic Checks

- [ ] `typecheck / lint / targeted tests`
- [ ] `integration / contract / smoke`, if relevant
- [ ] other automatic checks per task

## Reality Verification

- [ ] What check will prove the change actually works in the target environment
- [ ] What check closes the main risk of the task

## Persona Review

For non-trivial tasks, it is useful to leave a short output in the format:

- `BLOCKER:` ...
- `RISK:` ...
- `NOTE:` ...

If a status is not present, it is better to explicitly write:
- `BLOCKER:` none

## Needs Human Verification

Add only what the agent cannot reliably prove themselves.

- `Priority:` high | medium | low
  `Area:` ...
  `Why human:` ...
  `Steps:` ...
  `Expected:` ...
  `Observed by agent:` ...
  `Devices / environments:` ...

## Session Log

### Session 1 (<YYYY-MM-DD>)

- Done:
- Verified:
- Next step:

## Surprises

- What was not as expected
- What knowledge is worth compressing into skill, checklist, or decision trace later

## Deferred

- What is consciously left for later

## Next Safe Step

- Where the next agent can safely continue
