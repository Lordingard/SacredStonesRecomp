param(
    [string] $GbaRecompExe,
    [string] $RomPath,
    [string] $OutputPath,
    [string] $ConfigPath,
    [string] $SymbolsPath,
    [string] $MingwBin,
    [string] $PythonPath = "python",
    [ValidateRange(1, 64)][int] $Jobs = 2,
    [int] $CodegenShards = 0,
    [int] $MaxFunctions = 0,
    [switch] $NoConfig,
    [switch] $NoSymbols,
    [switch] $Force,
    [switch] $VerboseRecompiler
)

. "$PSScriptRoot/common.ps1"

$resolvedGbaRecompExe = Join-Path $script:RepoRoot "build/generator-mingw/gba_recompile.exe"
if ($PSBoundParameters.ContainsKey('GbaRecompExe') -and
    (Resolve-RepoPath $PSBoundParameters['GbaRecompExe']) -ne (Resolve-RepoPath $resolvedGbaRecompExe)) {
    throw "External generators are unsupported. Use the generator built from extern/gbarecomp."
}
$resolvedRomPath = Resolve-Setting $RomPath "FE8_ROM" "RomPath" $script:DefaultRomPath
$resolvedOutputPath = Resolve-Setting $OutputPath "SACREDSTONES_RECOMP_OUTPUT" "GeneratedProjectPath" $script:DefaultGeneratedProject
$resolvedConfigPath = Resolve-Setting $ConfigPath "SACREDSTONES_RECOMP_CONFIG" "GameConfigPath" (Join-Path $script:RepoRoot "config/game.fe8u.toml")
$resolvedSymbolsPath = Resolve-Setting $SymbolsPath "SACREDSTONES_RECOMP_SYMBOLS" "ImportedSymbolsPath" (Join-Path $script:RepoRoot "symbols/imported_symbols.tsv")

Assert-Rom -Path $resolvedRomPath
if (-not $NoConfig) {
    Assert-File -Path $resolvedConfigPath -Label "GBARecomp TOML config"
}

$python = (Get-Command $PythonPath -ErrorAction Stop).Source
& "$PSScriptRoot/build-generator.ps1" -MingwBin $MingwBin -Jobs $Jobs
Assert-File -Path $resolvedGbaRecompExe -Label "Pinned GBARecomp generator"
$identity = Get-FrameworkIdentity
if (-not $NoSymbols) {
    Assert-File -Path $resolvedSymbolsPath -Label "Imported symbols TSV"
}

$argsList = @(
    "build",
    "--rom", $resolvedRomPath,
    "--output", $resolvedOutputPath
)
if (-not $NoConfig) {
    $argsList += @("--config", $resolvedConfigPath)
}
if (-not $NoSymbols) {
    $argsList += @("--symbols", $resolvedSymbolsPath)
}
if ($CodegenShards -gt 0) {
    $argsList += @("--codegen-shards", [string]$CodegenShards)
}
if ($MaxFunctions -gt 0) {
    $argsList += @("--max-functions", [string]$MaxFunctions)
}
if ($Force) {
    $argsList += "--force"
}
if ($VerboseRecompiler) {
    $argsList += "--verbose"
}

Write-Host "Generating GBARecomp project into: $resolvedOutputPath"
$previousCore = $env:GBARECOMP_CORE
try {
    $env:GBARECOMP_CORE = $resolvedGbaRecompExe
    & $python (Join-Path $script:RepoRoot "extern/gbarecomp/tools/cli.py") @argsList
    if ($LASTEXITCODE -ne 0) { throw "Pinned game generation failed." }
} finally {
    $env:GBARECOMP_CORE = $previousCore
}

$required = @(
    (Join-Path $resolvedOutputPath "generated/recompiled.h"),
    (Join-Path $resolvedOutputPath "generated/dispatch_table.cpp"),
    (Join-Path $resolvedOutputPath "build.ps1")
)
foreach ($path in $required) {
    Assert-File -Path $path -Label "Generated file"
}

$inputs = @($resolvedRomPath)
if (-not $NoConfig) { $inputs += $resolvedConfigPath }
if (-not $NoSymbols) { $inputs += $resolvedSymbolsPath }
$manifest = @{
    schema = 1
    framework = $identity
    generator_sha256 = (Get-FileHash -LiteralPath $resolvedGbaRecompExe -Algorithm SHA256).Hash
    inputs = @($inputs | ForEach-Object {
        @{ path = (Resolve-Path -LiteralPath $_).Path; sha256 = (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash }
    })
}
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $resolvedOutputPath "sacredstones-generation.json") -Encoding utf8

Write-Host "Generation complete."
