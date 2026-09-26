---
name: shared-repo-automation-discipline
description: Use when deciding whether repeated work should become a repo-native script or command, and how to make that automation easy for both humans and agents without hiding scripts inside skills.
---

# Repo Automation Discipline

Prefer repo-native automation.
Do not treat skills as the primary home for useful project scripts.

## Use this skill when

- a repeated action should probably become a short command or script;
- both the user and the agent may want to run it;
- you are deciding where automation should live.

## Default rule

Put reusable automation in the repository:
- `package.json`
- `tools/`
- `scripts/`
- `Makefile`
- `CMake`
- `justfile`
- or stack-native equivalents

Skills may point to those entrypoints, but should stay thin.

## Promote an action when

- it repeats often;
- it should work non-interactively;
- stable exit codes matter;
- a short command would reduce friction.

## Keep entrypoints

- short;
- non-interactive by default;
- explicit about side effects;
- quiet on success;
- clear on failure.

## References

- `../../../docs/Automation.md`
- `../shared-decision-discipline/SKILL.md`
