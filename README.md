# SacredStonesRecomp - Fire Emblem: The Sacred Stones, Recompiled

> This is a playable, actively maintained static recompilation, not a finished PC port.
> It is already playable, but the project is still early and the runtime will keep
> improving. Testing reports and focused bug reproductions are useful.

Static recompilation of *Fire Emblem: The Sacred Stones* (Game Boy Advance) to a
native Windows executable, built on a pinned [`gbarecomp`](https://github.com/Lordingard/gbarecomp/tree/sacred-stones-runtime)
runtime fork with a pinned [`recomp-ui`](https://github.com/Lordingard/recomp-ui/tree/sacred-stones-launcher)
launcher fork. Both retain their upstream projects' code and licenses.

## AI-Assisted Development

This project is developed with substantial assistance from OpenAI Codex, including
code changes, runtime integration, debugging, code review, build tooling, and
documentation. The maintainer directs the work, makes project decisions, and
tests releases through gameplay, alongside automated checks.

This describes the work on SacredStonesRecomp and its dependency forks; it does not
attribute AI use to the upstream projects or the original game.

## Status - Playable, In Active Development

The game boots through the launcher and is playable into normal gameplay. The
major visible issues found during early testing have been resolved or confirmed
to match original/emulator behavior.

Working now:

- Optimized Windows Release builds for the game and runtime.
- Integrated pre-boot launcher with ROM and BIOS selection plus box art.
- Play prompts for a missing or invalid BIOS.
- Remembered input-device selection, with safe fallback when a controller is absent.
- Correct FE8 SRAM save/load configuration.
- Save states and rewind.
- Xbox-compatible controller support through SDL.
- User-provided GBA BIOS support for correct boot, timing, and interrupt behavior.
- Quiet project builds: generated/framework warning noise is filtered from the
  normal build output.

Known limitations:

- A very small audio artifact may be heard at the very beginning of the intro.
- Windows is the only packaged target for now.
- Mods are not currently exposed.
- The game has not yet been exhaustively tested from start to finish.

Current maintenance priorities:

- Evaluate further upstream launcher improvements independently.
- Evaluate upstream CPU timing improvements separately, with boot, gameplay,
  battery-save, save-state, and rewind regression checks.

Version 0.1.9 includes the selected upstream update that makes costly diagnostic
captures opt-in, while preserving the project's save-safety fixes. Automated
boot, SRAM-file reload, save-path, locked-save recovery, input, and diagnostic
tests pass, including windowed replays and a missing-controller fallback.
The maintainer also validated gameplay, fast-forward, in-game saves, save states,
rewind, the BIOS picker, and controller-selection persistence. This is not a
claim of exhaustive start-to-finish coverage. Further upstream launcher and CPU
timing updates remain planned.

## What Static Recompilation Means Here

The ROM's ARM7TDMI machine code is translated ahead of time into native code and
linked with a PC runtime that models the GBA hardware: graphics, audio, DMA,
timers, input, save memory, cartridge mapping, and BIOS services.

This repository does not contain the ROM, the GBA BIOS, or generated ROM-derived
C/C++ output. You supply your own legally obtained ROM and GBA BIOS; local
generated files and build products stay ignored by Git.

## ROM

Target | Game | ROM | SHA-1 | Save | Debug port
--- | --- | --- | --- | --- | ---
`SacredStonesRecomp` | Fire Emblem: The Sacred Stones | USA | `c25b145e37456171ada4b0d440bf88a19f4d509f` | SRAM, 32 KiB | 19842

The runtime validates the ROM SHA-1 and refuses unrecognized ROMs.

## Quick Start

1. Download [SacredStonesRecomp-0.1.9-win64.zip](https://github.com/Lordingard/SacredStonesRecomp/releases/download/v0.1.9/SacredStonesRecomp-0.1.9-win64.zip)
   from the release's **Assets** section and extract it. The **Source code**
   archives are for developers, not the playable package.
2. Run `SacredStonesRecomp.exe`.
3. Select your legally obtained GBA BIOS when prompted.
4. Select your legally obtained *Fire Emblem: The Sacred Stones* USA ROM when
   prompted.
5. Press Play.

The selected BIOS and ROM paths are cached next to the executable for future launches.
Keep the extracted folder together when moving the game.

Play opens the BIOS picker if no valid BIOS is configured.
After selecting it, press Play again. The preferred controller is remembered;
if it is disconnected, the game falls back to available input without discarding
the preference. Keyboard controls remain available.

## Controls

GBA button | Keyboard
--- | ---
D-Pad | Arrow keys
A | X
B | Z
L / R | C / V
Start | Enter
Select | Right Shift

Default assist bindings:

- Save state: Shift+F1 through Shift+F9.
- Load state: F1 through F9.
- Rewind: hold `1` on keyboard or the left trigger on an Xbox-compatible
  controller.
- Fast-forward: hold `2` on keyboard or the right trigger on an Xbox-compatible
  controller.

## Saves And Runtime State

Runtime files are local to the extracted folder:

- Battery save: `saves/SacredStonesRecomp.sav`
- Launcher settings: `sacredstonesrecomp.ini`
- ROM picker cache: `sacredstonesrecomp-rom.cfg`
- BIOS picker cache: `sacredstonesrecomp-bios.cfg`
- Self-heal cache and diagnostics: `recomp_cache/` and `recomp_coverage_*.json`

These files are intentionally excluded from source control and release archives.

## How It Self-Improves

`gbarecomp` tracks coverage honestly. If execution reaches a code path that was
not part of the static corpus, the runtime can bridge it safely, compile a native
replacement in-process, and cache that result under `recomp_cache/<rom-sha1>/`.
The next launch can reuse the warmed path.

For the current FE8 build, the tested startup path reports `FULLY_STATIC`, so
normal boot does not need a warmed cache. The cache still remains useful as the
project explores more of the game and closes rare coverage gaps.

## Building From Source

Developer workflow details live in [docs/workflow.md](docs/workflow.md). Clone
submodules before building:

```powershell
git submodule update --init --recursive
```

The short Windows path is:

```powershell
pwsh scripts/bootstrap.ps1 -WriteLocalConfig
pwsh scripts/generate.ps1 -Force
pwsh scripts/build-runner.ps1
```

Release packages are created with:

```powershell
pwsh scripts/package-release.ps1 -Version 0.1.9
```

The package script uses a whitelist and must not include ROMs, BIOS dumps, save
files, caches, logs, generated objects, or local configuration.

Configure `BiosPath`, `RomPath`, and `MingwBin` in `config/project.local.ps1`, or
pass them explicitly. Source generation needs Python 3 and builds its compiler
from the pinned GBARecomp sources. Runner builds reject outdated generation.

Packaging tests staged binaries in isolation: boot, SRAM-file reload, explicit
save paths, locked-save recovery, and 3,600 frames with menu input. Logs remain
in `build/validation/`. This does not replace manual gameplay testing.

## Legal

This project contains no copyrighted ROM data and no Nintendo BIOS. You must
supply your own legally obtained *Fire Emblem: The Sacred Stones* USA ROM and
your own legally obtained GBA BIOS. Fire
Emblem and The Sacred Stones are trademarks of Nintendo and Intelligent Systems.
This project is an unaffiliated preservation and research effort.
