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
- FE8 US ROM: `E:\git\Fire Emblem - The Sacred Stones (U).gba`

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

The generated project is written to `.generated/gbarecomp/`. Its
`build/gbarecomp_game.lib` is the first target artifact.

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
  -ElfPath E:\git\fireemblem8u\fireemblem8.elf
```

Review the resulting `symbols/imported_symbols.tsv` before regenerating.
