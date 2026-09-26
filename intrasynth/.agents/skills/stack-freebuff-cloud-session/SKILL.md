---
name: stack-freebuff-cloud-session
description: Load FIRST, once per session, inside a Freebuff/Codebuff Cloud browser workspace (Vly/Daytona sandbox, managed preview and deploy, injected git credential, .scratch/ workspace): orient the repo in one call instead of re-exploring it for 15 minutes, then follow the sandbox, terminal-timeout, file-editing, env, git and reporting rules. Not applicable to local CLI or other-agent runs.
---

# Freebuff / Codebuff Cloud Session

This skill is the runtime contract for this repository when it is opened inside a
Freebuff/Codebuff Cloud workspace. Load it first; everything else (task-init,
worklog, closeout, the project fitting skill) hangs off it.

If you are not inside Freebuff/Codebuff Cloud, skip this skill — the repo rules in
`AGENTS.md` still apply.

## Step 0. Orient in one call, do not re-explore

A Freebuff turn can start with an empty context, so the repo looks unknown every
time. Do **not** answer that with a broad `glob`/grep sweep and bulk reads — that is
exactly the "first 15 minutes" the owner wants gone. Run, from the project root:

```sh
node intrasynth/tools/agent/orient.mjs --brief   # state, commands, where the map is
node intrasynth/tools/agent/orient.mjs           # full digest: subsystems, pitfalls, recent history
```

Then read only what the task needs:

1. `intrasynth/docs/Architecture.md` — the project map (subsystems, file-to-topic index,
   invariants that already bit us, things not to do).
2. `intrasynth/docs/Rules.md` — hard core rules, always in force.
3. `intrasynth/.agents/skills/<relevant>/SKILL.md` — task-init, worklog, closeout, and
   `project-intra-fitting-and-measurement` for any sound work.
4. `intrasynth/tools/analysis/README.md` — only when measuring; it is large, read
   the section for the symptom you actually have.

Keep the map honest: `node intrasynth/tools/agent/orient.mjs --check` fails if any
path or command mentioned in `docs/Architecture.md`, `AGENTS.md` or the skills no
longer exists. Fix the map rather than silently working around it.

Do not re-derive from scratch: canonical build config, measurement commands, the
meaning of "note-on", the bank-program mapping, and the table-loop/vibrato/noise
pitfalls are all already written down. Rediscovering them costs a session.

Note that file-discovery tools honour `.gitignore`, so `scripts/`, `intrasynth/web/generated/`,
`dist/` and `.scratch/` are invisible to search but very much present on disk. Use the
map, not a search hit list, to know what exists.

## Sandbox rules

- Terminal commands are synchronous and killed on a hard timeout. Give a build an
  explicit longer budget; never background a process (`&`, `nohup`) — it will not
  survive and the call will hang until the timeout.
- Never start, stop, restart or kill dev/preview servers. The platform owns those
  processes; edits are picked up automatically. Inspect state instead of restarting.
- Edit files with the file tools (`write_file`, `str_replace`, `apply_patch`), not
  with `sed`/redirection — shell edits can fail to persist through the synced build
  state.
- Very large files are only partially visible to the edit tools (tail beyond roughly
  the first ten kilobytes may not be found). When a file like
  `intrasynth/tools/ab/render-ab.mjs` must change at the end, write a small anchored
  patch script under `.scratch/`, assert on unique anchors, run it, and verify the
  result.
- Never read, print or source `.env` / `.env.local`. To learn which keys exist, use
  the environment listing tool (names only, never values). Missing secret → name the
  key and ask the owner to add it in Settings → Environment; you cannot write it.
- Git: the platform injects a short-lived, repository-scoped credential per command.
  Run `git`/`gh` normally. Never ask for a PAT, never configure SSH, never rewrite the
  remote, never fall back to a stale credential. Do not commit, push or open PRs
  unless the owner explicitly asks.
- Temporary work (probe builds, scratch renders, patch scripts) goes under
  `.scratch/`. Never leave `build-*` or `probe-*` directories next to `AGENTS.md`.
  Keep `.scratch/ab-builds/<id>/` — the A/B panel replays "before/after" from those
  snapshots.

## Always-on duties in this repository

- Answer, and write task/decision/worklog documents, **in Russian**. Code comments may
  be English. Commit messages are the one English exception.
- Any change to synth code: rebuild the canonical configuration, stage the web
  artifacts, and report the exact WASM size and md5 in the reply and in the worklog.
  Never switch build configuration to make a number look better; the canonical config
  is defined in `AGENTS.md`.
- Non-trivial work: create or attach the worklog and decision documents as the
  task-init / worklog / closeout skills prescribe.
- Owner-facing acceptance of sound is by ear; the agent's job is to make the audition
  a clean comparison (A/B panel, level-matched, one variable at a time).

## Preview / deploy, only when asked

The repository's preview commands are already configured (install, dev server binding
`0.0.0.0`, and a static build that writes `dist/` and exits). Do not change them
without a request, and do not run the build scripts yourself just to "check" unless
the task needs it. If a preview looks stale, fix the code or type errors instead of
restarting anything, and inspect logs through the provided tooling.
