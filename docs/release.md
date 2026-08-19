# Release Notes

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