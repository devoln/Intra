---
name: shared-task-init-discipline
description: Use for non-trivial tasks to decide whether a worklog is needed, set risk level, record key invariants, and start from a lean task frame instead of ad hoc execution.
---

# Task Init Discipline

Use this skill at the start of non-trivial work.

## When this skill applies

Use it when any of these are true:
- the task is medium/high risk;
- the task will likely take multiple steps;
- the task touches multiple subsystems;
- the task may need handoff;
- the task may end with human verification;
- the task is vague enough that scope could drift.

Skip it for clearly local, low-risk, one-shot edits that are fully provable with fast checks.

## What to do

1. Classify the task as `Low`, `Medium`, or `High`.
2. Load only the relevant skills for the task.
3. If the task is non-trivial, create or update a worklog in `docs/tasks/active/`.
4. Record 1-3 invariants for `Medium/High` tasks.
5. Write a short `Out Of Scope` section if scope creep is plausible.
6. Break the work into small milestones.

## Minimum task frame

For most non-trivial tasks, the task doc should quickly answer:
- What are we changing?
- How risky is it?
- What must not break?
- What checks will prove it?
- What still may require a human?

## Keep it light

- Default to a lean worklog, not a thesis.
- For most tasks, a compact document is enough.
- If the task doc starts growing faster than the implementation, compress it.

## Output expectations

At the start of meaningful work, there should usually be:
- `Risk Level`
- `Goal`
- `Out Of Scope`
- `Skills Loaded`
- `Milestones`
- `Invariants` when relevant
- `Deterministic Checks`

## References

- `../shared-worklog-discipline/SKILL.md`
- `../shared-decision-discipline/SKILL.md`
- `../../templates/WorklogTemplate.md`
