# Release Notes

## v0.1.9 - 2026-10-05

- Rebuild from an isolated clean clone with all pinned dependencies downloaded
  from GitHub. All eleven runtime scenarios, four upstream native tests, and
  the save-file and launcher-policy test programs passed.
- Build the game and runtime in optimized Release mode by default. The maintainer
  validated normal gameplay and smooth fast-forward.
- Integrate the selected upstream opt-in diagnostic performance update, with
  regenerated game and BIOS code. Normal play no longer records costly debug
  captures by default; diagnostic flags remain available.
- Open the BIOS picker from Play when the BIOS is missing or invalid. After
  selecting a valid BIOS, press Play again.
- Remember the selected input device, including when closing the launcher without
  playing. Resolve the preferred controller by SDL GUID, with safe fallback if
  absent; keyboard input remains available.
- Show actual controller connection status in the launcher.
- Retain safe SRAM replacement, explicit save paths, save states, and rewind.
- Pin the runtime and launcher to project forks so clean clones can reproduce
  these fixes independently of future upstream changes.
- Validate isolated boot, SRAM reload, save overrides, locked-save recovery,
  input, normal/diagnostic windowed replays, and an unavailable controller.
  The maintainer validated combat, in-game saves, save states, rewind, BIOS
  selection, and controller-selection persistence.
- Keep the executable at the archive root. The Windows asset contains compiled
  binaries, launcher assets, configuration, README, and release notes only;
  no ROM, BIOS dump, user save, cache, or generated source is distributed.
- Update the README's project status and retain the AI-assisted development
  disclosure. Minor intro audio and incomplete full-game coverage remain known
  limitations; broader upstream launcher and CPU timing updates are deferred.

## v0.1.8 - 2026-10-05

- Preserve both the previous SRAM save and its temporary recovery file when
  replacement fails, including a Windows file lock.
- Honor explicit `--save` and `--save-path` destinations through launcher startup
  without migrating or modifying the default save.
- Generate game code with the pinned framework and verify source/input provenance
  before runner builds.
- Enable static game resume coverage; the previously failing 3,600-frame menu
  input test now passes with no interpreter or compiler fallback.
- Packaging tests boot, reload, save overrides, locked-save recovery, and menu
  input. A separate 1,200-frame windowed replay checks presentation and coverage.
- The user validated the local v0.1.8 archive before publication.

## v0.1.7 - 2026-10-05

- Fix the crash after the Game Boy Advance logo by using the pinned BIOS
  recompilation configuration, including interrupt returns and resume entries.
- Require a BIOS for runner builds and limit default build parallelism to two jobs.
- Test staged binaries through boot and SRAM-file reload before creating archives.
- Keep isolated validation logs under `build/validation/`.
- Additional review findings and remaining test gaps are recorded in
  `docs/review-2026-10-05.md`.
- The corrected archive was tested successfully by the user before publication.
  Automated checks cover startup and SRAM-file reload; they do not certify all
  gameplay paths or in-game save-slot contents.

## v0.1.6-preview - Proposed

Clean build and packaging reproducibility update for the Windows preview build.

### Fixed

- Clean checkouts can now rebuild the launcher runner end to end with `scripts/build-runner.ps1`.
- The runner build regenerates local BIOS-derived recompilation output from the user-provided BIOS instead of relying on files left in a developer checkout.
- The generated game library is now built with MinGW and linked as `libgbarecomp_game.a`, removing the previous `corrupt .drectve` linker warnings caused by mixing MSVC objects with the MinGW runner.
- MinGW runtime DLL staging now follows the configured compiler path instead of assuming `C:\msys64`.

### Verified

- Fresh clone build, package creation, BIOS LLE backend, and SRAM save configuration were validated from `F:\git\SacredStonesRecompTemp`.

## v0.1.5-preview - Proposed

Save handling and launcher BIOS picker hotfix for the Windows preview build.

### Fixed

- The launcher exposes the GBA BIOS picker again and persists the selected BIOS
  path beside the executable.
- The runner now forces FE8's SRAM save type at process startup. This keeps the
  interactive launcher path from losing `[save].type = "sram"` before runtime
  save-chip setup.
- The runner now creates the executable-local `saves/` directory before runtime
  startup.
- Existing battery saves are migrated to the stable path
  `saves/SacredStonesRecomp.sav` when the stable file is missing or still blank.
- Migration checks the prior launcher/runtime conventions: `saves/<ROM>.sav`,
  a `.sav` beside the ROM, a `.sav` beside the executable, and the temporary
  singular `save/` folder used during manual testing.
- SRAM 16-bit and 32-bit writes now persist every byte instead of only the low
  byte. This matches BIOS copy routines that write wider words into the 8-bit
  SRAM region.
## v0.1.4-preview - Proposed

Save-path hotfix for the Windows preview build.

### Fixed

- Battery saves now use an executable-local stable path:
  `saves/SacredStonesRecomp.sav`.
- The save filename no longer depends on the selected ROM filename, and the
  runtime no longer falls back to writing a `.sav` beside the ROM when launched
  through the packaged runner.
## v0.1.3-preview - Proposed

Third hotfix for the Windows preview build.

### Fixed

- The Windows preview now uses a user-provided GBA BIOS instead of forcing the
  incomplete BIOS HLE path. This restores correct boot timing and interrupt
  behavior in the packaged launcher build.
- The executable-local `game.toml` handling from v0.1.2 is kept, so launcher
  state, ROM cache, BIOS cache, save path derivation, and runtime configuration
  stay anchored to the extracted release folder.

### Packaging

- Windows archive remains binary-only and user-facing: executable at archive
  root, runtime DLLs, launcher assets, `game.toml`, README, and release notes.
- ROM files, BIOS dumps, saves, local picker/config files, caches, diagnostics,
  logs, generated objects, and source archives are not included in the asset.

### GitHub Cleanup

- Earlier preview releases v0.1.0-preview, v0.1.1-preview, and v0.1.2-preview
  were withdrawn because their packaged launcher builds were not reliable.
## v0.1.2-preview - Proposed

Second hotfix for the Windows preview build.

### Fixed

- The launcher now receives the executable-local launch context before it opens.
  This keeps launcher state, ROM cache, save path derivation, and `game.toml`
  lookup anchored to the extracted release folder.
- `game.toml` is explicitly exposed to the launcher through `launcher_game_config`.
- `--config <exe>/game.toml` and `--bios-hle` are applied both before and after
  the launcher step so the interactive launch path matches headless tests.
## v0.1.1-preview - Proposed

Hotfix for the first Windows preview build.

### Fixed

- The runner now resolves `game.toml` from the executable directory on Windows.
  This keeps BIOS HLE and FE8 SRAM configuration active when launching the
  release from Explorer or another working directory.

### Packaging

- Windows archive remains binary-only and user-facing: executable, runtime DLLs,
  launcher assets, `game.toml`, README, and release notes.
- ROM files, BIOS dumps, saves, local picker/config files, caches, diagnostics,
  logs, generated objects, and source archives are not included in the asset.
## v0.1.0-preview - Proposed

First Windows preview build for *Fire Emblem: The Sacred Stones* (USA).

### Highlights

- Integrated `recomp-ui` launcher with GBA profile and box art.
- ROM selection is cached locally after first launch.
- BIOS HLE is enabled by default; normal users are not asked for a BIOS file.
- SRAM save and load are configured for FE8.
- Save states and rewind are enabled.
- Runtime state is anchored next to the executable for portable extracted builds.
- Build output is quiet: generated/framework warnings are filtered from normal
  project builds.

### Known Issues

- Minor intro audio artifact reported at the very beginning of the game.
- Windows packaging only for this preview.
- Mods are intentionally not exposed yet.

### Release Artifact Policy

Release archives must not contain:

- ROM files (`*.gba`).
- BIOS dumps (`*.bin`).
- Battery saves or save states.
- `recomp_cache/` or diagnostic dump files.
- Local picker/config files such as `*rom.cfg`, `*bios.cfg`, and
  `sacredstonesrecomp.ini`.

The expected Windows archive contains the executable, required runtime DLLs,
launcher assets, and `game.toml`.
