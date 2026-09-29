---
name: shared-agent-process-maintenance
description: Use when improving the agent-development process itself, when repeated user corrections suggest a missing rule, or when docs/skills/worklogs are drifting, bloating, or failing to shape runtime behavior.
---

# Agent Process Maintenance

This skill is for maintaining the process system itself.

## When this skill applies

Use it when:
- the user explicitly asks to improve the agent workflow;
- the same correction keeps repeating across sessions;
- task docs are bloating or not closing cleanly;
- runtime behavior still depends on large maintainer docs;
- skills are missing, stale, duplicated, or too broad;
- bootstrap files are too heavy or too vague.

## Primary goal

Move repeated operational behavior into thin reusable skills.
Keep large process documents for maintainers, not as default runtime context.

## Preferred direction

- `AGENTS.md` should stay thin.
- shared runtime habits should live in shared skills.
- project-specific runtime habits should live in project skills.
- large process canon should be used mainly to evolve the system, not to drive every task directly.

## Typical actions

1. Identify where runtime still depends on bulky docs.
2. Extract the operational subset into one or more thin skills.
3. Remove duplicated guidance from bootstrap files.
4. Compress oversized task/worklog docs.
5. Archive finished notes instead of keeping them active forever.
6. Update maintainer docs only when the process model itself changes.

## Keep process docs small in runtime

If a rule must be followed by default in normal work, it should probably live in:
- a thin skill;
- a compact template;
- or a small bootstrap note.

If a document mainly explains why the process exists or how it evolved, it is maintainer-facing, not runtime-facing.

## When to read deeper process docs

Read `../../core/Flow.md` and `../../core/ResearchBase.md` only when:
- evolving the process model itself;
- resolving ambiguity between competing process rules;
- deciding whether something belongs in a skill, template, or maintainer doc.

## References

- `../../core/Flow.md`
- `../../core/ResearchBase.md`
- `../shared-decision-discipline/SKILL.md`
