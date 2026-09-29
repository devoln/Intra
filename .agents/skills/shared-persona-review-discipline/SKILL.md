---
name: shared-persona-review-discipline
description: Use for non-trivial tasks when review should reflect real multi-perspective analysis instead of a shallow one-line verdict. Helps select relevant personas, inspect the work through each lens, then compress the result into BLOCKER/RISK/NOTE.
---

# Persona Review Discipline

Use this skill before finishing non-trivial work.

## Purpose

Persona review is not a decorative verdict block.
Force perspective shifts first, then compress the result.

## When this skill applies

Use it when:
- the task is medium/high risk;
- the task changes architecture, UX, reliability, performance, contracts, or recovery behavior;
- the task is hard to validate from one angle only;
- the user explicitly asks for review or audit;
- you feel uncertainty but cannot explain it from a single perspective.

Skip it for clearly tiny, local, low-risk changes.

## Workflow

1. Choose only the relevant personas.
2. For each selected persona, do a short but real pass using its checklist/questions.
3. Record the important finding from that pass.
4. Only after that, compress the outcome into:
   - `BLOCKER`
   - `RISK`
   - `NOTE`

## Recommended depth

- low-risk: usually none, or 1 persona if helpful;
- medium-risk: 2-4 personas;
- high-risk: multi-persona review expected.

## Output format

Keep the final worklog/task doc compact:
- `BLOCKER:` ...
- `RISK:` ...
- `NOTE:` ...

## Important rule

Do not pretend independent subagent review if you did not actually have isolated agents.
This discipline improves one agent's review quality through perspective shifts.
It is not the same as fully independent multi-agent verification.

## References

- `../../core/Personas.md`
- `../shared-task-closeout-discipline/SKILL.md`
