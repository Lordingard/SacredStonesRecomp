param(
    [Parameter(Mandatory = $true)][string] $ElfPath,
    [string] $OutputPath,
    [string] $ReadElfPath = "arm-none-eabi-readelf",
    [switch] $UseWsl,
    [switch] $IncludeLocalSymbols
)

. "$PSScriptRoot/common.ps1"

$resolvedOutputPath = Resolve-Setting $OutputPath "SACREDSTONES_RECOMP_SYMBOLS" "ImportedSymbolsPath" (Join-Path $script:RepoRoot "symbols/imported_symbols.tsv")
Assert-File -Path $ElfPath -Label "fireemblem8u ELF"

if ($UseWsl) {
    Assert-WslAvailable
    $wslElf = ConvertTo-WslPath $ElfPath
    $lines = & wsl.exe bash -lc "$ReadElfPath -Ws $(Quote-WslShellArgument $wslElf)"
} else {
    $lines = & $ReadElfPath "-Ws" $ElfPath
}
if ($LASTEXITCODE -ne 0) {
    throw "Failed to run $ReadElfPath. Put arm-none-eabi-readelf in PATH, pass -ReadElfPath, or use -UseWsl."
}

$rows = New-Object System.Collections.Generic.List[string]
$rows.Add("# Imported from FireEmblemUniverse/fireemblem8u ELF.")
$rows.Add("# Format: 0xADDR<TAB>mode<TAB>name")

$seen = @{}
foreach ($line in $lines) {
    if ($line -notmatch "^\s*\d+:\s+([0-9a-fA-F]+)\s+\d+\s+FUNC\s+(LOCAL|GLOBAL|WEAK)\s+\S+\s+\S+\s+(.+?)\s*$") {
        continue
    }

    $rawAddr = [Convert]::ToUInt32($Matches[1], 16)
    $bind = $Matches[2]
    $name = $Matches[3].Trim()
    if (-not $IncludeLocalSymbols -and $bind -ceq "LOCAL") {
        continue
    }
    if ($name.StartsWith("$") -or $name.StartsWith(".") -or $name -match "\s") {
        continue
    }
    if ($rawAddr -lt 0x08000000 -or $rawAddr -ge 0x0A000000) {
        continue
    }

    $mode = if (($rawAddr -band 1) -eq 1) { "thumb" } else { "arm" }
    $addr = $rawAddr -band 0xFFFFFFFE
    $key = "{0:X8}:{1}" -f $addr, $mode
    if ($seen.ContainsKey($key)) {
        continue
    }
    $seen[$key] = $true
    $rows.Add(("0x{0:X8}`t{1}`t{2}" -f $addr, $mode, $name))
}

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $resolvedOutputPath) | Out-Null
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText($resolvedOutputPath, (($rows -join "`n") + "`n"), $utf8NoBom)
Write-Host "Wrote $($rows.Count - 2) function seed symbols to: $resolvedOutputPath"
