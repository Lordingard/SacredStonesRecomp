# SacredStonesRecomp - Fire Emblem: The Sacred Stones, Recompiled

> This is an in-development static recompilation preview, not a finished PC port.
> It is already playable, but the project is still early and the runtime will keep
> improving. Testing reports and focused bug reproductions are useful.

Static recompilation of *Fire Emblem: The Sacred Stones* (Game Boy Advance) to a
native Windows executable, built on a pinned [`gbarecomp`](https://github.com/Lordingard/gbarecomp/tree/sacred-stones-runtime)
runtime fork with the [`recomp-ui`](https://github.com/mstan/recomp-ui) launcher.

## Status - Playable Preview

The game boots through the launcher and is playable into normal gameplay. The
major visible issues found during early testing have been resolved or confirmed
to match original/emulator behavior.

Working now:

- Integrated pre-boot launcher with ROM and BIOS selection plus box art.
- Correct FE8 SRAM save/load configuration.
- Save states and rewind.
- Xbox-compatible controller support through SDL.
- User-provided GBA BIOS support for correct boot, timing, and interrupt behavior.
- Quiet project builds: generated/framework warning noise is filtered from the
  normal build output.

Known limitations:

- A very small audio artifact may be heard at the very beginning of the intro.
- Windows is the only packaged target for now.
- Mods are not exposed in this preview.
- The game has not yet been exhaustively tested from start to finish.

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

1. Download the latest `SacredStonesRecomp-*-win64.zip` release and extract it.
2. Run `SacredStonesRecomp.exe`.
3. Select your legally obtained GBA BIOS when prompted.
4. Select your legally obtained *Fire Emblem: The Sacred Stones* USA ROM when
   prompted.
5. Press Play.

The selected BIOS and ROM paths are cached next to the executable for future launches.
Keep the extracted folder together when moving the game.

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

For the current FE8 preview, the tested startup path reports `FULLY_STATIC`, so
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
pwsh scripts/build.ps1 -GenerateFirst
```

The MinGW runner is built separately with CMake after the generated game library
exists. From an MSYS2 MINGW64 shell, configure and build it with:

```sh
cmake -S . -B build/runner-mingw -G Ninja
cmake --build build/runner-mingw --target SacredStonesRecomp --parallel
```

Release packages are created with:

```powershell
pwsh scripts/package-release.ps1 -Version 0.1.0-preview
```

The package script uses a whitelist and must not include ROMs, BIOS dumps, save
files, caches, logs, generated objects, or local configuration.

## Legal

This project contains no copyrighted ROM data and no Nintendo BIOS. You must
supply your own legally obtained *Fire Emblem: The Sacred Stones* USA ROM and
your own legally obtained GBA BIOS. Fire
Emblem and The Sacred Stones are trademarks of Nintendo and Intelligent Systems.
This project is an unaffiliated preservation and research effort.
