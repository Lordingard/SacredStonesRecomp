# SacredStonesRecomp

Recompilation project scaffolding for *Fire Emblem: The Sacred Stones* (U)
using [mstan/gbarecomp](https://github.com/mstan/gbarecomp).

This repository is ROM-free. The local FE8 ROM, generated GBARecomp C++, and
compiled libraries are intentionally ignored by Git.

## Current target

- Game: Fire Emblem: The Sacred Stones (U)
- Expected ROM SHA1: `c25b145e37456171ada4b0d440bf88a19f4d509f`
- First artifact: `.generated/gbarecomp/build/gbarecomp_game.lib`

## Windows quick start

```powershell
pwsh scripts/bootstrap.ps1 -WriteLocalConfig
pwsh scripts/build.ps1 -GenerateFirst
```

Defaults match the initial local setup:

- `D:\Jeux\GBARecomp\gbarecomp.exe`
- `E:\git\Fire Emblem - The Sacred Stones (U).gba`

See [docs/workflow.md](docs/workflow.md) for generation, build, and
FireEmblemUniverse/fireemblem8u symbol import notes.
