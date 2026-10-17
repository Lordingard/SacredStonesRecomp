# Mouse Tutorial Audit

Scope: FE8 USA, experimental Windows mouse controls, 2026-10-16.
The native tutorial remains authoritative. No key masks, unit coordinates,
event queues, path data, script flags or player save files are rewritten.

## Findings And Shared Coverage

| Mechanism | Native source | Mouse policy / verification |
| --- | --- | --- |
| Prologue first attack uses a separate forced-A callback | `src/playerphase.c`, `src/events/prologue-tutorials.h` | Recognized callback. Recover only an occupied, rejected destination to the active-unit cursor anchor; do not redirect valid guided movement. Real snapshot reaches combat. |
| Instant cursor movement previously bypassed ignored D-pad directions | `Event11_SetIgnoredKeys`, `src/events_script_utils.c` | Check native mask before queueing and before hook execution; A remains native-filtered. Unit tests cover mask changes and hook write boundaries. |
| Forecast explanation overlays an otherwise live target selector | `src/helpbox.c`, `src/bksel.c`, `src/uiselecttarget.c` | Printing/waiting text owns input above targets. Keep native target freezes and temporary A restriction. Text advance cannot inherit attack confirmation. Snapshot test exercises this sequence. |
| Frozen menus could fall through to a scene skip listener | `src/uimenu.c`, `src/eventscr.c` | Live menu/target/trade blocks fallback even while temporarily unavailable. Native dialogue remains usable above it. Policy regression covers the fallthrough. |
| Chapter-1 guided Trade has special input states | `src/bmtrade.c`, `src/events/ch1-tutorials.h` | Right inventory, Vulnerary, current destination, then B. State changes cancel plans. Automatic states do not accept clicks. Synthetic tests and an isolated native snapshot sequence pass; interactive mouse acceptance still needs confirmation. |
| Village, attack and movement instruction helpers repeat | `src/events_script_utils.c`, prologue/chapter-1 event headers | Reuse the same map masks, menu availability and text handling; no per-chapter script patch. |
| Chapter-2 action tutorials repeat selection/menu restrictions | `src/events/ch2-eventscript.h` | Reviewed the shared Exec0/Exec1 calls, forced-A selections and menu overrides (including rescue/drop and staff-related instruction chains). Same guards apply; no chapter-2 runtime fixture available for this audit. |
| Later story scenes temporarily ignore A | `src/events/ch12b-eventscript.h` | Preserve the game's native A mask and scripted camera movement. No bypass or event advancement by memory edits. |

Source paths above are relative to `extern/fireemblem8u/`; addresses used by
the adapter are checked against this checkout's `fireemblem8.map`.

## Validation Boundaries

- Tests generate native input through the production adapter and runtime;
  they do not edit the snapshot to jump past a tutorial event.
- The supplied snapshot reproduces the first-attack blockage, not the entire
  prologue. Only that recovery/attack sequence has a real-game automated test.
- Synthetic tests exercise guided Trade and input ownership but cannot prove
  every script timing or inventory layout throughout a full chapter.
- Remaining manual acceptance: finish the prologue, then chapter-1 village,
  trade and item use; chapter-2 guided action sequences; both routes' later
  menus, preparations and event choices. Preserve a state at any mouse-only
  dead end to add a reproducible integration case.
- Unsupported screens are not claimed as covered by this audit. The original
  keyboard/controller controls remain available; no public package or Android
  build was changed for this experimental Windows increment.

## Chapter-1 Snapshot Follow-Up (2026-10-17)

The supplied October-17 state1 starts at the Trade partner selector, before
the inventory screen. An isolated headless native-input test reproduced the
full sequence with the current policy: preview and separately confirm the
partner, advance the instruction boxes, choose the Vulnerary, confirm the
destination, then right-click to supply B at the final tutorial instruction.
The native trade process reported an actual swap, the receiving inventory
contained the Vulnerary and the action menu returned. No guest memory or
snapshot was edited. The run stayed fully static.

Evidence: `build/validation/mouse-trade-tutorial-7b285fea32f9449f89b3beab11f83b8d/`.
This does not reproduce the reported mouse-only blockage in a visible Windows
session. The exact failing instruction still needs confirmation; no production
input behavior was changed based solely on this successful headless test.
