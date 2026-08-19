param(
    [string] $FireEmblem8uRoot,
    [string] $Repository = "https://github.com/FireEmblemUniverse/fireemblem8u.git"
)

. "$PSScriptRoot/common.ps1"

$resolvedRoot = Resolve-Setting $FireEmblem8uRoot "FIREEMBLEM8U_ROOT" "FireEmblem8uRoot" (Join-Path $script:RepoRoot "extern/fireemblem8u")
$resolvedRoot = [System.IO.Path]::GetFullPath($resolvedRoot)

if (Test-Path -LiteralPath (Join-Path $resolvedRoot ".git")) {
    Write-Host "fireemblem8u checkout already exists: $resolvedRoot"
    exit 0
}

$parent = Split-Path -Parent $resolvedRoot
New-Item -ItemType Directory -Force -Path $parent | Out-Null

git clone $Repository $resolvedRoot
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
Write-Host "Cloned fireemblem8u: $resolvedRoot"
