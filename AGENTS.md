Use relevant skills from `.agents/skills/`.

For non-trivial work, the default runtime path should come from thin skills, especially shared task-init / worklog / closeout discipline, not from large maintainer docs.

Whatever you do, from designing a system to tiny bugfix, you MUST always keep in mind and follow `docs/Rules.md`.

Task/history documents live in `docs/tasks/`: `active/`, `planned/`, `archived/`.

Architecture and trade-off records live in `docs/decisions/`: `active/`, `accepted/`, `archived/`.

Task and decision documents should be entirely in English.

Task and decision file names should use the format `YYYYMMDD-UpperCamelCase.md`.

If work continues for a while in one direction without an assigned task, and decisions or trade-offs are being made, create or attach a task/worklog before continuing further.

High-level process artifacts and shared skills in `.agents/` must not be changed without explicit owner agreement.


Temporary experiment output (ad-hoc build trees, probes, scratch renders) must go under `.scratch/` and stay gitignored; never leave untracked `build-*/` or `probe-*` dirs in the repo root (`/.scratch/`, `/build-*/`, `/probe-tmp/` are in `.gitignore`).