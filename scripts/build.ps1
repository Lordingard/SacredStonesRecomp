param(
    [string] $ProjectPath,
    [switch] $GenerateFirst,
    [switch] $Clean
)

. "$PSScriptRoot/common.ps1"

$resolvedProjectPath = Resolve-Setting $ProjectPath "SACREDSTONES_RECOMP_OUTPUT" "GeneratedProjectPath" $script:DefaultGeneratedProject

if ($GenerateFirst) {
    & (Join-Path $script:RepoRoot "scripts/generate.ps1") -OutputPath $resolvedProjectPath -Force
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

$buildScript = Join-Path $resolvedProjectPath "build.ps1"
Assert-File -Path $buildScript -Label "Generated build script"

if ($Clean) {
    $buildDir = Join-Path $resolvedProjectPath "build"
    if (Test-Path -LiteralPath $buildDir) {
        Remove-Item -LiteralPath $buildDir -Recurse -Force
    }
}

& $buildScript
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$candidateLibs = @(
    (Join-Path $resolvedProjectPath "build/gbarecomp_game.lib"),
    (Join-Path $resolvedProjectPath "build/Release/gbarecomp_game.lib")
)
$lib = $candidateLibs | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
if (-not $lib) {
    throw "gbarecomp_game.lib not found under $(Join-Path $resolvedProjectPath "build")."
}
Write-Host "Built: $lib"
