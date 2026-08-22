# Engineering Flow

## Purpose

This document defines the general workflow for development.

It describes:
- how a developer should approach tasks;
- what artifacts they should create and update;
- how to extract repeatable knowledge into skills and templates;
- how to adapt this process for a specific project.

The document is intentionally not tied to a specific repository, language, stack, or product.
This is a common starting point from which the project-specific layer is then expanded.

## Core Idea

The executor should not work as a free-form code generator.
They should work within a system of constraints, knowledge, memory, and checks.

Base stack:
- `Specs` define what is being built.
- `Invariants` define what must not be broken.
- `Personas` set mandatory review angles.
- `Skills` encode repeatable knowledge.
- `Worklogs` store working memory for the task.
- `Decision traces` store reasons for strategic decisions.
- `Deterministic gates` check whether the result is acceptable.

The decision owner is responsible for priorities, judgment, and final approval.
The executor is responsible for research, drafts, implementation, structuring, and process discipline.

## Base Work Cycle

For any non-trivial task, the developer should go through this cycle:

1. Understand the current state before making changes.
2. Find or create a spec.
3. Assess risk level.
4. Formulate invariants if necessary.
5. Load relevant skills, personas, and reference artifacts.
6. Create or update worklog.
7. Break work into small, verifiable steps.
8. Implement with frequent local verification.
9. Run persona-based review.
10. Close the loop with reality-based verification.
11. Extract reusable lessons into skills, templates, or decision traces.
12. Leave behind a state from which the next developer can safely continue.

## Phase 1. Understand Before Changing

Before changing code, the developer must gather enough context to avoid working blindly.

Default expectations:
- read relevant code;
- read relevant documents;
- look at existing tests;
- look at git history if behavior or reasons for current implementation are unclear;
- do a short walkthrough of a complex area before making changes;
- understand the task type: product, infrastructure, bugfix, migration, refactor, research.

Priority is always:
- first understand the existing system;
- then propose changes;
- only then write code.

## Phase 2. Lock Down the Task

The executor should work from a spec whenever possible, not from a vague chat description.

If a spec exists:
- read it;
- treat it as the primary source of intent;
- record discrepancies between spec and code.

If no spec exists and the task is significant:
- create a lightweight feature spec or system note before implementation.

A good spec should include:
- happy path;
- sad paths;
- edge cases;
- business/system constraints;
- explicit `out of scope` or `deferred`.

## Phase 3. Classify Risk

Before choosing process depth, the developer must determine the risk level.

Recommended levels:
- `Low`: local refactor, text edits, small isolated fix, trivial test update.
- `Medium`: regular feature work, endpoint changes, data flow changes, integration work, noticeable UI changes.
- `High`: auth, payments, PII, security-sensitive logic, migrations, async/recovery logic, long-running workflows, cross-system contracts, complex visual scenarios where automatic correctness proof is difficult.

Risk level affects:
- worklog mandatory status;
- invariants necessity;
- verification depth;
- number of mandatory personas;
- model-diverse review necessity;
- human verification queue necessity.

## Phase 4. Invariants

For medium/high-risk tasks, it is worth formulating a small set of verifiable invariants.

Invariants:
- are not tests;
- are verifiable statements about system correctness;
- later become tests, manual checks, review criteria, and architectural constraints.

A good invariant:
- is concrete;
- is verifiable;
- is tied to real risk;
- helps discard incorrect solutions.

If the task is low-risk and strictly local, invariants may be skipped.

## Phase 5. Load Reusable Knowledge

Before implementation, it is worth looking for ready knowledge rather than reinventing patterns.

This could be:
- shared skills;
- project-specific skills;
- personas;
- worklogs for similar tasks;
- decision traces;
- reusable prompts;
- reference artifacts and exemplars.

General rule:
- thin, focused artifacts are better than giant universal documents;
- knowledge should be split by generality levels;
- do not load the entire AI archive at once if you can connect point-wise.

## Phase 6. Worklog

Worklog is working memory for the task, not a historical chronicle of the project.

It is needed for:
- multi-session tasks;
- multi-step tasks;
- medium/high-risk tasks;
- tasks with significant manual verification;
- handoff between developers or sessions.

### Exact Rule

`Worklog is mandatory for all medium/high-risk, multi-session, multi-step, and human-verification-heavy tasks. For local low-risk changes, worklog is not needed.`

### Worklog is mandatory if at least one of these applies

- task does not fit in one short session;
- multiple subsystems or code zones are touched;
- external API, migration, async/recovery, auth, payments, PII;
- need to give a person a list of manual checks later;
- handoff needed;
- risk of scope creep.

### Worklog can be skipped if all of these are true simultaneously

- task is low-risk;
- task is local;
- fits in one session;
- fully verifiable with fast automatic checks;
- does not require separate handoff or human verification.

### Minimum worklog structure

- task name;
- risk level;
- loaded skills;
- milestones;
- invariants, if needed;
- deterministic checks;
- `Needs Human Verification`;
- session log;
- surprises;
- next-step handoff.

If a starter skeleton is needed, use `.agents/templates/WorklogTemplate.md` and trim it for the specific task.

### Compactness Rule

Worklog must not grow uncontrollably.

Practical heuristic:
- for most tasks, it is normal to keep worklog in the `50-100` line range;
- if worklog is longer than the expected implementation, that is a warning sign;
- for simple tasks, remove sections rather than add formality;
- long historical analyses should be compressed into a short summary and moved to archive, decision trace, or profile doc.

### Storage Policy

- active worklogs live in a separate folder;
- after task completion they are moved to archive;
- archive is stored in the repository until the project owner decides otherwise;
- worklog is not required to live forever if its useful content is already compressed into skills/docs.

In the current task/history model:
- `docs/tasks/` is the general container for task documents;
- worklog is one type of document inside `docs/tasks/active/`, not a replacement for the entire task system.

## Phase 7. Implementation in Small Steps

Small, reviewable, verifiable steps should be preferred over large opaque rewrites.

By default:
- choose the smallest meaningful milestone;
- make the change;
- run the cheapest useful check;
- record the result;
- only proceed if failure is understood or the gate is green.

Where appropriate, prefer:
- red/green TDD;
- small commits;
- splitting into reviewable chunks;
- serial execution instead of premature parallelism.

## Phase 8. Continuous Verification

"Looks correct" is not sufficient evidence.

Verification should be multi-layered:
- first cheap checks;
- then more expensive ones;
- then human judgment where automation is incomplete.

### Verification pyramid

- `Base`: exit codes, format, lint, typecheck, targeted unit tests.
- `Middle`: integration tests, contract tests, smoke checks, screenshot diffs, scripted browser checks.
- `Top`: manual QA, visual assessment, product judgment, production-like verification, logs and metrics.

Useful rule:
- if there is a relevant test suite in the zone, it is usually worth running it before changing code.

If correctness cannot be verifiably checked, the task is poorly prepared for autonomous execution.

## Phase 9. Persona-based Review

For significant changes, it is useful to perform review from multiple perspectives.

Review statuses:
- `BLOCKER`
- `RISK`
- `NOTE`

Unresolved `BLOCKER` blocks task completion.

For high-risk tasks where possible, it is useful to separate executor and reviewer:
- one executor implements;
- another checks;
- the decision owner makes the final call.

## Phase 10. Closing the Loop

A task is considered complete not when the code is written, but when there is evidence that the intended behavior actually works.

Closing the loop must include:
- green deterministic gates;
- at least one reality-based verification;
- updated worklog;
- explicit enumeration of what still requires human verification;
- extraction of reusable lessons if there were surprises.

Examples of reality-based verification:
- real request to endpoint;
- browser scenario;
- migration check in a clean environment;
- recovery check after restart;
- visual behavior check on typical viewports;
- log/observable state review after change.

## Phase 11. Learn Immediately

Every important surprise should make the system smarter immediately, not "someday later".

There are two main mechanisms:

`Real-time updates`
- create or update skill;
- adjust persona checklist;
- adjust worklog template;
- adjust verification checklist;
- add reusable prompt or reference artifact.

`Decision traces`
- save the reason for an architectural or process decision;
- link it to downstream rules that grew from it.

Rule:
- tactical lesson -> update skill now;
- strategic lesson -> update decision trace.

### Repetition Rule

If the same pattern, remark, manual instruction, or correction repeats approximately `3+` times within a project, it is worth raising the question of systematization.

By default this means:
- propose creating a new skill;
- or propose updating an existing skill;
- or explain why this pattern is better recorded in skill, `Flow`, persona, task/history note, or decision trace.

The project owner should not have to remember this manually every time.
If an automated executor sees repeatability and context allows, they should raise this themselves.

## Human Verification Queue

For everything that cannot be reliably checked automatically, a separate list should be kept for humans.

This list must:
- accumulate as work progresses;
- be deduplicated;
- be updated after each fix;
- at the end of the task, output a compact set of checks.

### Element format

- `Priority`
- `Area`
- `Why human`
- `Steps`
- `Expected`
- `Observed by agent`
- `Devices / environments`

### When this is especially needed

- frontend polish;
- mobile/responsive behavior;
- visual quality;
- subtle UX;
- real devices;
- scenarios where automatic verification is partial or unreliable.

## Frontend / Visual Verification Policy

If UI changes, it is worth maximizing automatic checks before showing the result to a human.

By default, you should be able to or try to:
- check desktop viewport;
- check mobile viewport;
- take screenshots;
- check obvious overflow / clipping;
- check clickability;
- check scroll / autoscroll behavior;
- check console/runtime/network errors;
- go through key user scenario in browser-like environment.

The human checks a result that is already technically tidied up, not a raw iteration.

## Parallelism Policy

Parallel executors are an additional mode, not the default.

Use them only when:
- tasks are truly independent;
- file overlap is low;
- review burden remains manageable;
- machine and build system can handle it.

Modes:
- `Subagents` — for parallel research.
- `Independent sessions/worktrees` — for isolated implementation.
- `Serial handoffs` — for dependency-heavy or resource-heavy work.

Orchestrator should coordinate.
Executor should execute.
These roles should not be mixed unnecessarily.

## Context Policy

Context should be designed, not just accumulated.

Default rules:
- permanent instructions should be compact;
- dynamic facts should not be mixed with durable guidance;
- large libraries of skills and docs should be connected via progressive disclosure;
- long histories should be compressed into handoff/worklog summary;
- do not load everything on the executor at once.

Prefer:
- index + pointers instead of giant blobs;
- thin skills instead of copied docs;
- canonical document in one place;
- lazy loading of heavy artifacts.

## Tooling Policy

Prefer deterministic tools wherever they reduce uncertainty, latency, and token waste.

For hot paths, prefer:
- schema-driven interfaces;
- CLI tools;
- scripts with stable output;
- direct protocols instead of slow step-by-step orchestration.

More expensive exploratory tools are left for cold paths:
- research;
- fuzzy discovery;
- cross-system exploration;
- rare complex analysis.

Right question:
- not "how powerful is this tool?";
- but "how often will it be called and how well does it constrain the error space?".

## Code Generation Policy

Writing code has become cheap.
Understanding and maintaining it is still expensive.

Therefore the developer must:
- freely make prototypes during research;
- aggressively simplify before completion;
- not leave extra generated code without reason;
- optimize for maintainability and future understanding.

Generation speed does not justify cognitive debt.

## Review Burden Policy

The first serious review of a significant change should be done by whoever initiated its creation.

Do not pass on to subsequent reviewers:
- raw diff;
- unchecked code;
- poorly understood implementation;
- "convincing-looking" result without evidence.

A good reviewable change should contain:
- limited scope;
- clear goal;
- verification evidence;
- references to spec/worklog/decision context;
- explicit risks and deferred items.

## Universal vs Project-specific Separation

The system should port well between languages, stacks, and project types.

Therefore artifacts are split into at least two levels.

### Universal

This is what can be ported to almost any project:
- general `Flow.md`;
- cross-project skills;
- general-purpose personas;
- worklog templates;
- decision trace templates;
- reusable prompts;
- verification patterns;
- human verification queue format.

### Project-specific

This is what adapts to a specific project:
- project-specific skills;
- project-specific personas or additions to them;
- stack rules;
- verification scenarios;
- naming conventions;
- tool-specific bootstrap;
- specific risk zones and invariants;
- reference artifacts for specific codebase.

### Stack-specific

To avoid dragging unnecessary baggage between unrelated projects, it is useful to have a separate layer of stack-specific knowledge.

Examples:
- `web-frontend`
- `backend-api`
- `mobile`
- `cpp-engine`
- `graphics`
- `multithreading`
- `binary-formats`
- `streaming/protocols`

This layer is better stored not in one giant document, but in skills, templates, and reference artifacts by topic.

Thus:
- marketing will not interfere with system projects;
- web-specific practices will not drag into C++ engine;
- graphics and multithreading patterns can be added separately.

## Packaging Model

Recommended packaging model:
- automation-specific canon in `.agents/`;
- thin bootstrap at root if tool requires it;
- shared and project-specific skills separated;
- stack-specific knowledge connected separately as needed.
- task/history layer lives separately in human working directories.

Recommended structure:
- `.agents/core/Flow.md`
- `.agents/core/Personas.md`
- `.agents/core/ResearchBase.md`
- `.agents/skills/shared-*`
- `.agents/skills/project-*`
- `.agents/skills/stack-*`
- `docs/decisions/active/...`
- `docs/decisions/accepted/...`
- `docs/decisions/archived/...`
- `docs/tasks/active/...`
- `docs/tasks/planned/...`
- `docs/tasks/archived/...`
- root bootstrap files only if truly needed

## Naming Policy

File names inside automation-specific and task/history layers do not have to follow environment requirements if they are not tool-discovery entrypoints.

Therefore:
- internal documents can be named in `UpperCamelCase` if convenient;
- special bootstrap files should be named as required by the specific tool;
- content documents are better stored under convenient names rather than historical convention names, if the environment does not look for them itself.

## Changing High-level AI Files

Automated executors can update themselves:
- worklogs;
- task-level notes;
- draft skills;
- local checklists;
- temporary handoff artifacts.

Automated executors must coordinate with project owner to change:
- `Flow.md`;
- persona definitions;
- shared cross-project skills;
- high-level AI policy files;
- other strategic AI artifacts.

## Accepted Universal Decisions

At this point, the following universal decisions have been accepted:

- Primary language for documents and discussion is English.
- Skills may be in English if it provides real context compactness benefit.
- Worklog is not always mandatory, only by trigger rule from this document.
- Invariants are mandatory for high-risk tasks.
- Personas are used as mandatory review perspectives, but the initial review contract is built on `BLOCKER / RISK / NOTE`, not hard `VETO`.
- Shared and project-specific skills are stored separately.
- Stack-specific knowledge is stored separately from general flow and connected as needed.
- Automation-specific canon lives in `.agents/`.
- Root bootstrap files are allowed, but should be thin and not duplicate canonical documents.
- High-level AI files are changed only with project owner agreement.
- For everything that cannot be reliably automatically verified, `Needs Human Verification` / human verification queue must be kept.
- Frontend and other visual changes must be automatically verified as much as possible before showing the result to a human.

## Anti-patterns

Avoid:
- working without understanding current behavior;
- implementation without spec for significant tasks;
- skipping invariants in high-risk zones;
- giant stale skills;
- huge static instructions with duplication;
- "done" status without explicit verification;
- excessive local parallelism;
- merging code that no one understood;
- accumulating extra generated complexity;
- hiding risks behind confident tone.

## Final Rule

If forced to choose, prefer:
- smaller scope over hidden risk;
- clear artifacts over clever prompting;
- verifiability over convincingness;
- reusable learning over one-time success.
