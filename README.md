# SacredStonesRecomp - Fire Emblem: The Sacred Stones, Recompiled

> This is a playable, actively maintained static recompilation for Windows and Android.
> It is already playable, but the project is still early and the runtime will keep
> improving. Testing reports and focused bug reproductions are useful.

Static recompilation of *Fire Emblem: The Sacred Stones* (Game Boy Advance) to a
native Windows executable and Android ARM64 app. Windows is built on a pinned [`gbarecomp`](https://github.com/Lordingard/gbarecomp/tree/sacred-stones-runtime)
runtime fork with a pinned [`recomp-ui`](https://github.com/Lordingard/recomp-ui/tree/sacred-stones-launcher)
launcher fork. Both retain their upstream projects' code and licenses.
Android uses separately pinned upstream dependencies with reviewed local patches;
see [Android development and validation](android/README.md).

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

The previously reported intro audio artifact is no longer observed by the
maintainer in recent releases; its original cause has not been confirmed.

Working now:

- Optimized Windows Release builds for the game and runtime.
- Integrated pre-boot launcher with ROM and BIOS selection plus box art.
- Play prompts for a missing or invalid BIOS.
- Remembered input-device selection, with safe fallback when a controller is absent.
- Remembered launcher window size, with a 940x799 default and fitting to smaller displays.
- Correct FE8 SRAM save/load configuration.
- In-game settings menu with state slots 1-9, save/load, and fast-forward controls.
- Fast-forward speed from 2x to 10x and independently switchable rewind.
- Remembered speed, rewind/Assist Tools enable switches, and selected state slot.
- Remembered Windows fullscreen mode, shared by the launcher and in-game menu.
- Native-resolution exclusive fullscreen and the same application icon as Android.
- ZIP save export from the in-game menu on both platforms.
- Xbox-compatible controller support through SDL.
- User-provided GBA BIOS support for correct boot, timing, and interrupt behavior.
- Quiet project builds: generated/framework warning noise is filtered from the
  normal build output.

Android provides a production-signed ARM64 APK. The maintainer reports working
gameplay on Google Pixel 9 Pro, Xiaomi Pad 5, and Ayn Thor, and working save/load
states through the in-game menu. The app includes touch controls, physical
controller support, aspect-correct screen fitting, and automatic suspend/resume.
Version 0.1.12 adds ZIP save export from setup and the in-game menu, modern Android
Back support (validated on Ayn Thor), and larger display fitting without reserving
the touch-gesture margins around the game image. The maintainer validated export
on Pixel 9 Pro and accepted the display correction after three-device testing.
Version 0.1.13 aligns the Windows and Android release numbers. Windows adds
persistent fullscreen and ZIP save export; Android retains the validated 0.1.12
game runtime and features. The two platforms keep independent dependency pins.

Known limitations:

- Packaged targets are Windows x64 and Android ARM64 (Android 9 or newer).
- Android has a single landscape surface; Ayn Thor's second screen is not used.
- Android has not been tested on every device, or through the entire game.
- Save export is available on both platforms; importing an archive is not implemented yet.
- Save-state compatibility across platforms or runtime versions is not guaranteed.
- Mods are not currently exposed.
- The supported game ROM and launcher are in English; translation is not planned.
- The game has not yet been exhaustively tested from start to finish.

Current maintenance priorities:

- Evaluate further upstream launcher improvements independently.
- Evaluate upstream CPU timing improvements separately, with boot, gameplay,
  battery-save, save-state, and rewind regression checks.

Version 0.1.10 adds the in-game menu, persistent runtime controls, and targeted
launcher window/navigation fixes. It retains the optimized builds, opt-in
diagnostics, BIOS picker, controller persistence, and save-safety fixes from
earlier versions. Automated checks cover boot, SRAM reload, save overrides,
locked-file recovery, windowed input, diagnostics, and preference loading.
The maintainer validated the new menu controls and corrected layout, alongside
previous gameplay checks. This is not exhaustive start-to-finish coverage.
For 0.1.13, the maintainer also confirmed borderless/exclusive fullscreen sizing
and successful save export in exclusive fullscreen. Automated checks cover
temporary windowed transitions and restoration after cancellation.

## What Static Recompilation Means Here

The ROM's ARM7TDMI machine code is translated ahead of time into native code and
linked with a native runtime that models the GBA hardware: graphics, audio, DMA,
timers, input, save memory, cartridge mapping, and BIOS services.

This repository does not contain the ROM, the GBA BIOS, or generated ROM-derived
C/C++ output. You supply your own legally obtained ROM and GBA BIOS; local
generated files and build products stay ignored by Git.

## ROM

Target | Game | ROM | SHA-1 | Save | Debug port
--- | --- | --- | --- | --- | ---
`SacredStonesRecomp` | Fire Emblem: The Sacred Stones | USA | `c25b145e37456171ada4b0d440bf88a19f4d509f` | SRAM, 32 KiB | 19842

The runtime validates the ROM SHA-1 and refuses unrecognized ROMs.

## Quick Start - Windows

1. Download [SacredStonesRecomp-0.1.13-win64.zip](https://github.com/Lordingard/SacredStonesRecomp/releases/download/v0.1.13/SacredStonesRecomp-0.1.13-win64.zip)
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

## Quick Start - Android

1. Download [SacredStonesRecomp-0.1.13-android-arm64.apk](https://github.com/Lordingard/SacredStonesRecomp/releases/download/v0.1.13/SacredStonesRecomp-0.1.13-android-arm64.apk)
   from the release's **Assets**, not the source-code archives.
2. Open the APK on an ARM64 device running Android 9 or newer. Android may require
   permission to install from the browser or file manager you used.
3. Select your own legally obtained USA ROM and GBA BIOS, then press Play.
   These files are verified and imported into private app storage.
4. Use the virtual controls or a physical controller. The game keeps its original
   3:2 proportions and fits the available screen without stretching.

Select is on the left near the D-pad; Start is on the right near A/B, both at the
bottom. A connected controller hides virtual controls by default; a saved manual
visibility preference takes precedence. Disconnecting the last controller restores
the default touch controls when no manual preference is saved.
Press Android's **Back** button once, or use the Back gesture, to open the
in-game settings and Assist Tools menu. Ayn Thor's physical Back button is
supported. Controller Select keeps its GBA Select function; Home remains a
system action. R3/Guide are alternative menu inputs where the controller driver
delivers them, not a requirement for Ayn Thor.
Choose a state slot before Save state or Load state. Closing and reopening the
app normally resumes the last suspended position; this is separate from the
game's own save slots.

To back up your saves, open **Assist Tools > Export saves**, choose a ZIP
destination, and wait for **Saves exported**. The game stays paused during the
export. **Export saves** is also available on the setup screen; launchers that
support app shortcuts can expose **Saved games** even when setup is skipped.
The archive contains the game's battery save, existing state slots 1-9, the
automatic suspend snapshot and a backup manifest, never the ROM or BIOS.
Export does not remove or modify your saves. Import is not available yet.

The game maximizes its 3:2 image while protecting display cutouts. Black side
bands on wider screens are normal; filling those bands would require stretching
or cropping. Touch controls keep their own gesture-safe margins independently.

**Updates:** close the game and install the new APK over the existing app. Do not
uninstall or clear app data. This production APK includes a signing lineage from
the project's experimental builds, allowing those builds to update in place on
supported Android versions. If Android refuses an update, stop and report the
error rather than uninstalling to work around it.

The release uses a dedicated production certificate, not a debug build. Signing
does not eliminate all unknown-app or Play Protect prompts for GitHub downloads.
The SHA-256 checksum is provided alongside the APK.

## Controls - Windows

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

## In-Game Menu - Windows

Press **Escape** or the controller **Guide/Xbox** button to open the menu.
Some drivers or Windows shortcuts reserve Guide; Escape remains available.
Use the mouse or D-pad/arrows to navigate, A/Enter to confirm, and B/Escape to
go back. Resume returns to gameplay.

- Select **State slot** (1-9), then **Save state** or **Load state**.
- Set **Fast-forward speed** (2x-10x). Hold the usual shortcut or use the
  **Fast-forward** menu toggle; acceleration itself is not restored on restart.
- **Enable rewind** controls the temporary history independently. Turning it
  off discards that history; after re-enabling, wait for new history to build.
- **Assist Tools** enables/disables save-state, fast-forward, and rewind controls
  together. It does not disable the game's own battery save.
- **Export saves** opens a ZIP destination picker. The game stays paused and
  flushes its current battery save before exporting. The archive contains the
  battery save, existing state slots 1-9, any suspend snapshot, and a manifest;
  it never contains your ROM or BIOS. Keep the supplied `tools/` folder beside
  the executable. Export uses Windows' built-in PowerShell; no install is needed.
  In exclusive fullscreen, the game temporarily becomes windowed for the native
  picker and confirmation, then restores fullscreen, including after cancellation.
- Display/audio settings include fullscreen, window scale, filtering, volume,
  audio enable, and the FPS readout. Fullscreen is remembered; the other
  display/audio adjustments made in this menu are session settings.

Use **Alt+Enter** or **Display > Fullscreen** to switch between windowed and
fullscreen play. Borderless fullscreen is recommended; exclusive fullscreen
remains available. The launcher, menu and shortcut share the remembered mode.
The game preserves its original 3:2 image without stretching or cropping, so
side bands on wider monitors are normal. An explicit `--fullscreen=0`, `1`, or
`2` overrides the remembered startup mode for that launch.

Fullscreen, state slot, fast-forward speed, and the rewind/Assist Tools enable switches
are saved immediately. Savestates are snapshots, separate from the game's save
slots; rewind history is temporary and is not restored after closing the game.

## Saves And Runtime State

On Windows, runtime files are local to the extracted folder:

- Battery save: `saves/SacredStonesRecomp.sav`
- Save states: beside the selected ROM, `<ROM filename>.state1` through `.state9`
- Launcher settings: `sacredstonesrecomp.ini`
- Launcher window dimensions: `launcher-window.ini`
- In-game control preferences: `runtime-controls.toml`
- Keyboard bindings: `keybinds.ini`
- ROM picker cache: `sacredstonesrecomp-rom.cfg`
- BIOS picker cache: `sacredstonesrecomp-bios.cfg`
- Self-heal cache and diagnostics: `recomp_cache/` and `recomp_coverage_*.json`

These files are intentionally excluded from source control and release archives.

On Android, the battery save is `files/saves/SacredStonesRecomp.sav` inside private
app storage. Save states and automatic suspend snapshots are beside the imported
ROM in that storage, not beside the original file in Downloads. Installing an
update preserves that storage; uninstalling or clearing app data deletes it.
Use **Export saves** before any operation that could remove app data,
and keep the resulting ZIP separately. An export error can leave an incomplete
destination file: only an export reported successful should be used as a backup.
No ROM, BIOS dump, player save, or user configuration is bundled in the APK.

When updating, close the game and extract the new binary archive into the
existing folder. Keep `saves/` and your settings files; the archive contains no
player saves or preferences. Back up `saves/` and any savestates beside your ROM
before replacing an installation. Savestates keep their existing ROM-based
location; only the internal battery save has the standardized name above.

Missing/invalid preference values use defaults. If `runtime-controls.toml` is
malformed or cannot be updated, the game stays usable and reports the problem
in the console. After closing the game, rename that file to reset the runtime
preferences; this does not remove battery saves or savestates.

## How It Self-Improves

`gbarecomp` tracks coverage honestly. If execution reaches a code path that was
not part of the static corpus, the runtime can bridge it safely, compile a native
replacement in-process, and cache that result under `recomp_cache/<rom-sha1>/`.
The next launch can reuse the warmed path.

For the current FE8 build, the tested startup path reports `FULLY_STATIC`, so
normal boot does not need a warmed cache. The cache still remains useful as the
project explores more of the game and closes rare coverage gaps.

## Building From Source

Developer workflow details live in [docs/workflow.md](docs/workflow.md).
Android-specific tooling and signing instructions live in [android/README.md](android/README.md).
Clone
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
pwsh scripts/package-release.ps1 -Version 0.1.10
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
