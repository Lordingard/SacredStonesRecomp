param(
    [string] $FireEmblem8uRoot,
    [string] $RomPath,
    [switch] $RefreshAgbcc
)

. "$PSScriptRoot/common.ps1"

$resolvedRoot = Resolve-Setting $FireEmblem8uRoot "FIREEMBLEM8U_ROOT" "FireEmblem8uRoot" (Join-Path $script:RepoRoot "extern/fireemblem8u")
$resolvedRomPath = Resolve-Setting $RomPath "FE8_ROM" "RomPath" $script:DefaultRomPath

if (-not (Test-Path -LiteralPath (Join-Path $resolvedRoot "scripts/quickstart.sh") -PathType Leaf)) {
    throw "fireemblem8u checkout not found or incomplete: $resolvedRoot. Run scripts/bootstrap-fireemblem8u.ps1 first."
}
Assert-Rom -Path $resolvedRomPath
Assert-WslAvailable

$wslRoot = ConvertTo-WslPath $resolvedRoot
$wslRom = ConvertTo-WslPath $resolvedRomPath
$quotedRoot = Quote-WslShellArgument $wslRoot
$quotedRom = Quote-WslShellArgument $wslRom
$refreshValue = if ($RefreshAgbcc) { "1" } else { "0" }

Invoke-WslBash "cd $quotedRoot && find . -type f \( -name '*.sh' -o -name '*.bash' -o -name '*.py' \) -exec sed -i 's/\r$//' {} +"
Invoke-WslBash @"
set -euo pipefail
cd $quotedRoot

command -v git >/dev/null 2>&1 || { echo "Missing WSL dependency: git" >&2; exit 1; }
command -v make >/dev/null 2>&1 || { echo "Missing WSL dependency: make" >&2; exit 1; }
command -v arm-none-eabi-as >/dev/null 2>&1 || { echo "Missing WSL dependency: arm-none-eabi-as" >&2; exit 1; }
command -v pkg-config >/dev/null 2>&1 || { echo "Missing WSL dependency: pkg-config" >&2; exit 1; }
command -v python3 >/dev/null 2>&1 || { echo "Missing WSL dependency: python3" >&2; exit 1; }

python3 - <<'PY'
import importlib.util
import sys

missing = [name for name in ("numpy", "PIL") if importlib.util.find_spec(name) is None]
if missing:
    print("Missing Python module(s): " + ", ".join(missing), file=sys.stderr)
    sys.exit(1)
PY

if [ ! -f baserom.gba ]; then
    cp $quotedRom baserom.gba
fi

if [ "$refreshValue" = "1" ]; then
    rm -rf .deps/agbcc tools/agbcc
fi

if [ ! -x tools/agbcc/bin/agbcc ]; then
    mkdir -p .deps
    if [ ! -d .deps/agbcc/.git ]; then
        rm -rf .deps/agbcc
        git clone https://github.com/pret/agbcc.git .deps/agbcc
    fi

    git -C .deps/agbcc fetch origin
    git -C .deps/agbcc reset --hard origin/master
    (cd .deps/agbcc && ./build.sh && ./install.sh "$wslRoot")
fi

./build_tools.sh
make -j"`$(nproc)"
sha1sum -c checksum.sha1
"@

$elf = Join-Path $resolvedRoot "fireemblem8.elf"
Assert-File -Path $elf -Label "fireemblem8u ELF"
Write-Host "Built ELF: $elf"
