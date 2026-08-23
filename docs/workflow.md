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

- GBARecomp CLI: `D:\Jeux\GBARecomp\gbarecomp.exe`
- FE8 US ROM: sibling of this repo, `..\Fire Emblem - The Sacred Stones (U).gba`

Override them with parameters, environment variables, or
`config/project.local.ps1`.

## Generate and build

```powershell
pwsh scripts/generate.ps1 -Force
pwsh scripts/build.ps1
```

Or do both:

```powershell
pwsh scripts/build.ps1 -GenerateFirst
```

The generated project is written to `.generated/gbarecomp/`. Its first target artifact is `build/gbarecomp_game.lib`, or `build/Release/gbarecomp_game.lib` with Visual Studio-style generators.

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
