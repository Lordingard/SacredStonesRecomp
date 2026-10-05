# Launcher upstream review - 2026-10-05

Scope: evaluate launcher improvements without changing the validated game
runtime, CPU timing, or pinned dependency revisions. Upstream refs were fetched
from GitHub; this is a source review, not a build or visual validation of the
candidate patches.

Current project launcher: `2b0a36e07fef97e9de14d3477f0131ff0383cc3d`.
Upstream master: `d3151bf5f4fbb3e63faf7e204ab2937d16fe5324`.

## Recommended next batch

1. Adapt `76392fb` (fit the initial window to small displays). It changes the
   SDL platform layer, not the game. Validate usable work areas, scaling, and
   reachable controls on 1280x720, 1366x768, and high-DPI displays. The initial
   fit alone does not prove the entire interface fits at every size.
2. Adapt `37c0f21` (remember resized launcher dimensions). It stores logical
   dimensions in `launcher-window.ini` beside the host configuration, separately
   from game-window settings. Preserve the project's commit-on-Quit fix. Test
   Play/Quit/reopen, missing or malformed files, unavailable windows, read-only
   destinations, and moving the installation to a smaller display. Its upstream
   test depends on newer settings infrastructure; port focused coverage rather
   than importing that infrastructure wholesale.
3. Adapt `122bf21` (do not steal text/modal focus with Backspace or controller
   Back/Start). The same unconditional footer shortcut exists in our pin.
   Port the predicate and real ImGui-frame tests, then check typing and modal
   navigation interactively. No player-reported occurrence is claimed.

Apply these as individually traceable upstream-derived patches to the existing
fork. Keep the current BIOS picker, controller GUID persistence/fallback, and
save behavior. Do not advance the whole upstream branch to obtain three fixes.
Before a binary release, run launcher policy tests and the existing runtime
regressions, followed by visual and maintainer validation of the launcher.

## Deferred or not applicable

- `4eda654`: localization infrastructure is useful later, but inspected upstream
  master contains English/Italian selection, not a French translation. Adding
  French is additional work, not a benefit obtained just by updating the pin.
- `5006ac0`: fast-forward slider lives in Assist Tools, deliberately hidden by
  our host (`gi.has_assist_tools = 0`). The runtime must consume and persist the
  setting before exposing a working control.
- `6d40ae5`: a rewind toggle also needs explicit host-side support and a default
  that preserves today's enabled rewind. A launcher-only patch is insufficient.
- `516cbb0`: launcher icon support is lower priority for the currently packaged
  Windows target; inspect our resource setup before claiming a visible benefit.
- Netplay, mods, other-console settings, BIOS preparation jobs, and in-session
  launcher APIs are outside this batch. In particular, retain our validated
  Play-to-BIOS-picker behavior rather than importing the upstream Settings-only
  required-BIOS flow (`796c513`).

## First implementation batch

The three recommended window/navigation changes have now been applied to the
local launcher fork as a candidate, without changing game-runtime source or
the published v0.1.9. Cherry-pick conflicts were resolved without importing
unrelated NDS tests or newer SNES settings infrastructure. The project's
commit-on-Quit and Play-to-BIOS-picker behavior are retained.

The upstream real-ImGui navigation test passes. The project policy test now
covers geometry round trips, malformed dimensions, path capacity, and parsing,
alongside BIOS policy and controller persistence. The Release runner builds.
Scripted captures in `build/launcher-candidate-validation/` verify an 820x656
initial fit for a simulated 1280x720 usable area, reopening at a saved 1000x700,
and refitting a saved 1100x880 after moving to the smaller area. Dashboard and
settings were visually checked. These simulations are not a physical
multi-monitor/high-DPI certification.

All eleven existing runtime regression scenarios passed on the rebuilt
candidate with extended input, windowed input, and diagnostic capture. Logs:
`build/validation/3e1bff39679148549fd780a7d80f457e/`.

The subsequent runtime-control integration, persistent preferences, and menu
layout correction are recorded in `runtime-menu-candidate.md` and included in
v0.1.10. The maintainer accepted the UI and publication. French localization is
intentionally deferred while the supported ROM is English-only. No unrelated
upstream console/network changes were imported.
