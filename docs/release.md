# Release Notes

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