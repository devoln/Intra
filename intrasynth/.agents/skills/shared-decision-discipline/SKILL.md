---
name: shared-decision-discipline
description: Use when the agent must decide whether to act autonomously or pause for owner agreement. Helps distinguish local implementation choices from strategic product, architecture, and policy decisions.
---

# Decision Discipline

The agent should not escalate every small choice.
The agent should also not silently choose strategic directions that belong to the owner.

## Default rule

Act autonomously for local decisions.
Escalate strategic, policy-changing, or hard-to-reverse decisions.

## Safe to decide autonomously when all are true

- the change is local and reversible;
- it does not change project policy or architecture direction;
- it does not alter core product behavior in a surprising way;
- it stays within current specs, invariants, and accepted flow;
- relevant personas found no unresolved `BLOCKER`;
- no unresolved `RISK` changes the meaning of the task.

## Escalate when any of these are true

- the choice changes architecture direction;
- it changes product behavior beyond the user's explicit request;
- it introduces a new dependency with meaningful cost or lock-in;
- it changes security, privacy, compliance, payments, auth, or recovery policy;
- it rewrites a previously tuned behavior without strong proof;
- it edits high-level AI policy files or shared cross-project skills;
- the trade-off is real and multiple reasonable paths remain.

## What to do before escalating

Do the useful part first:
- narrow the options;
- identify trade-offs;
- run relevant persona review;
- state which option you recommend and why;
- keep the question focused on the actual decision.

## Good escalation shape

Prefer:
- "I can continue with option A or B. A keeps the current architecture and is cheaper now. B is more general but changes the system boundary. I recommend A unless you want to invest in the broader direction now."

Avoid:
- open-ended questions with no prior analysis;
- escalating choices that are merely stylistic or mechanical;
- silently committing to a strategic direction because it seems elegant.

## References

- `.agents/core/Flow.md`
- `.agents/core/Personas.md`
- `AGENTS.md`
