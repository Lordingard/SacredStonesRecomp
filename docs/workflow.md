# SacredStonesRecomp workflow

This repository keeps only reproducible project metadata and scripts.

It must not contain:

- the Fire Emblem: The Sacred Stones ROM;
- generated GBARecomp C++;
- generated static libraries or build directories;
- local FireEmblemUniverse/fireemblem8u checkouts.

## First bootstrap

```powershell
pwsh scripts/bootstrap.ps1 -WriteLocalConfig
```

The default local paths are:

- GBARecomp generator: built locally from `extern/gbarecomp` into `build/generator-mingw`
- FE8 US ROM: sibling of this repo, `..\Fire Emblem - The Sacred Stones (U).gba`

Configure ROM, BIOS, and MinGW paths with parameters, environment variables, or
`config/project.local.ps1`. Source generation requires Python 3. External
`GBARECOMP_EXE` and legacy `GbaRecompExe` local settings are no longer used.

## Runtime Dependency Updates

`extern/gbarecomp` is a pinned submodule that points to the Sacred Stones runtime
branch on the project fork:

```powershell
git submodule update --init --recursive extern/gbarecomp
```

The fork tracks upstream `mstan/gbarecomp` through the `sacred-stones-runtime`
branch. To refresh it, update that branch in `Lordingard/gbarecomp`, rebuild this
project, verify boot and internal saves, then bump the submodule pointer in this
repository.

`extern/recomp-ui` remains a separate submodule because launcher UI updates can
be evaluated independently from runtime changes.

## Generate and build

```powershell
pwsh scripts/generate.ps1 -Force
pwsh scripts/build.ps1
```

Or do both, then build the launcher runner:

```powershell
pwsh scripts/generate.ps1 -Force
pwsh scripts/build-runner.ps1
```

The generated project is written to `.generated/gbarecomp/`. The Windows runner
uses its MinGW library, `build-mingw/libgbarecomp_game.a`. `build-runner.ps1`
requires `BiosPath` (or `GBA_BIOS`) and regenerates BIOS output using the pinned
`extern/gbarecomp/bios/gba_bios.toml`. Omitting this configuration loses interrupt
return entries and caused the v0.1.6 startup crash. Generated files stay ignored.
Build parallelism defaults to two jobs; use `-Jobs` to change it.

The generator is built from the same pinned framework as the runtime.
Generation records both framework revisions, source fingerprints, and ROM,
annotation, and symbol-input hashes in the ignored
`.generated/gbarecomp/sacredstones-generation.json`. Runner builds reject changes
to these inputs until the game is regenerated. Full static resume entries are
enabled in `config/game.fe8u.toml` to cover frame-boundary and interrupt returns.

## Release validation

Run `pwsh scripts/test-release.ps1 -ExtendedInput` for isolated boot, SRAM-file
reload, save overrides, locked-save recovery, and input tests.
`scripts/package-release.ps1 -Version 0.1.8` runs these tests against its staged
files before creating the archive. Both accept `-BiosPath` and `-RomPath`.
Each run preserves logs and disposable saves in a unique `build/validation/`
directory, with developer compiler directories removed from the child PATH.

The `-ExtendedInput` test presses menu buttons for 3,600 headless frames and
enforces static coverage. `-WindowedInput` adds a 1,200-frame windowed replay.
Use `scripts/test-generation-provenance.ps1` to test stale-generation rejection.
Build target `sacredstones_save_file_tests` and run it with an isolated directory
argument to test native file replacement, including a locked destination.
These checks do not certify slot contents, combat, rewind, or physical controller
behavior. Check those manually before publishing.

## Symbol import

GBARecomp accepts imported function seed symbols as:

```text
0xADDR<TAB>mode<TAB>name
```

`mode` is `arm` or `thumb`.

Once a local `fireemblem8u` checkout has produced `fireemblem8.elf`, import
function symbols with:

```powershell
pwsh scripts/import-fireemblem8u-symbols.ps1 `
  -ElfPath .\extern\fireemblem8u\fireemblem8.elf `
  -UseWsl
```

If `fireemblem8u` is not built yet, WSL is the recommended path on Windows.
`bootstrap-fireemblem8u.ps1` only needs Git; `build-fireemblem8u.ps1` requires
a working WSL distribution:

```powershell
pwsh scripts/bootstrap-fireemblem8u.ps1
pwsh scripts/build-fireemblem8u.ps1
pwsh scripts/import-fireemblem8u-symbols.ps1 `
  -ElfPath .\extern\fireemblem8u\fireemblem8.elf `
  -UseWsl
```

If Windows reports that WSL is not installed, install a distribution first:

```powershell
wsl --install Ubuntu
```

Review the resulting `symbols/imported_symbols.tsv` before regenerating.

## Launcher runtime state

The Windows runner uses `recomp-ui` from `extern/recomp-ui` and calls the
GBARecomp launcher seam before `run_game()`.

Expected runtime behavior:

- `src/main.cpp` forces the executable-local `game.toml` through `--config` so
  launches from Explorer, terminals, and tests resolve the same save type and
  ROM identity settings.
- The packaged preview uses a user-provided GBA BIOS by default. The launcher
  exposes the BIOS picker and caches the selected path next to the executable.
- `src/main.cpp` forces FE8's SRAM save type at process startup because this
  single-game runner must not depend on launcher/config propagation for save-chip
  setup.
- Battery saves are anchored to `saves/SacredStonesRecomp.sav` next to the
  executable, not beside the ROM.
- On launch, `src/main.cpp` creates the `saves/` directory and migrates useful
  legacy saves from `saves/<ROM filename>.sav`, a `.sav` beside the ROM, a
  `.sav` beside the executable, or the temporary `save/` folder.
- `Assist Tools` is hidden in the launcher. Rewind and fast-forward remain
  enabled in-game through the runtime bindings.

Local files that must not be committed include `build/`, `recomp_cache/`,
`keybinds.ini`, `sacredstonesrecomp.ini`, `*rom.cfg`, `*bios.cfg`, `*.sav`, and
savestate files.
