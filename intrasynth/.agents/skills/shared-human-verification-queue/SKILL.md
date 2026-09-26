---
name: shared-human-verification-queue
description: Use when UX, visual, mobile, or product checks cannot be fully validated automatically. Produces a deduplicated human verification checklist.
---

# Human Verification Queue

When you cannot reliably prove correctness yourself, do not hide that gap.
Record it for the human.

## Add an item when

- Visual quality matters.
- Mobile behavior matters.
- Browser/device quirks may matter.
- The result is partly subjective.
- Automatic checks are incomplete.
- You validated mechanics, but not quality.

## Required item fields

- `Priority`
- `Area`
- `Why human`
- `Steps`
- `Expected`
- `Observed by agent`
- `Devices / environments`

## Rules

- Deduplicate similar checks.
- Keep steps concrete and short.
- Prefer one compact checklist at the end over many scattered warnings.
- Remove items that were later verified automatically.
- Mark clearly what you already checked yourself.

## Typical examples

- Mobile CTA visibility and prominence.
- Layout quality on narrow screens.
- Autoscroll that technically works but may still feel wrong.
- Visual polish of cards, spacing, typography, and hierarchy.
- Product-level clarity of wording and state transitions.
