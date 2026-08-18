param(
    [string] $GbaRecompExe,
    [string] $RomPath,
    [string] $OutputPath,
    [string] $ConfigPath,
    [string] $SymbolsPath,
    [int] $CodegenShards = 0,
    [int] $MaxFunctions = 0,
    [switch] $NoConfig,
    [switch] $NoSymbols,
    [switch] $Force,
    [switch] $VerboseRecompiler
)

. "$PSScriptRoot/common.ps1"

$resolvedGbaRecompExe = Resolve-Setting $GbaRecompExe "GBARECOMP_EXE" "GbaRecompExe" "D:\Jeux\GBARecomp\gbarecomp.exe"
$resolvedRomPath = Resolve-Setting $RomPath "FE8_ROM" "RomPath" "E:\git\Fire Emblem - The Sacred Stones (U).gba"
$resolvedOutputPath = Resolve-Setting $OutputPath "SACREDSTONES_RECOMP_OUTPUT" "GeneratedProjectPath" $script:DefaultGeneratedProject
$resolvedConfigPath = Resolve-Setting $ConfigPath "SACREDSTONES_RECOMP_CONFIG" "GameConfigPath" (Join-Path $script:RepoRoot "config/game.fe8u.toml")
$resolvedSymbolsPath = Resolve-Setting $SymbolsPath "SACREDSTONES_RECOMP_SYMBOLS" "ImportedSymbolsPath" (Join-Path $script:RepoRoot "symbols/imported_symbols.tsv")

Assert-File -Path $resolvedGbaRecompExe -Label "GBARecomp CLI"
Assert-Rom -Path $resolvedRomPath
if (-not $NoConfig) {
    Assert-File -Path $resolvedConfigPath -Label "GBARecomp TOML config"
}
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
& $resolvedGbaRecompExe @argsList
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$required = @(
    (Join-Path $resolvedOutputPath "generated/recompiled.h"),
    (Join-Path $resolvedOutputPath "generated/dispatch_table.cpp"),
    (Join-Path $resolvedOutputPath "build.ps1")
)
foreach ($path in $required) {
    Assert-File -Path $path -Label "Generated file"
}

Write-Host "Generation complete."
