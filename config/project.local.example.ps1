# Copy to config/project.local.ps1 and adjust paths for your machine.
# project.local.ps1 is intentionally ignored by Git.

$repoRoot = Split-Path -Parent $PSScriptRoot

$script:GbaRecompExe = "D:\Jeux\GBARecomp\gbarecomp.exe"
$script:RomPath = Join-Path (Split-Path -Parent $repoRoot) "Fire Emblem - The Sacred Stones (U).gba"

# Optional: local checkout of FireEmblemUniverse/fireemblem8u, used for symbol import.
$script:FireEmblem8uRoot = Join-Path $repoRoot "extern/fireemblem8u"
