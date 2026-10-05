param(
    [string] $BuildDir = "build/runner-mingw",
    [string] $RomPath,
    [string] $BiosPath,
    [ValidateRange(1, 600)][int] $TimeoutSeconds = 60,
    [switch] $ExtendedInput
)

. "$PSScriptRoot/common.ps1"

$resolvedRom = Resolve-Setting $RomPath "FE8_ROM" "RomPath" $script:DefaultRomPath
$resolvedBios = Resolve-Setting $BiosPath "GBA_BIOS" "BiosPath" ""
Assert-Rom -Path $resolvedRom
Assert-File -Path $resolvedBios -Label "GBA BIOS"
$resolvedRom = (Resolve-Path -LiteralPath $resolvedRom).Path
$resolvedBios = (Resolve-Path -LiteralPath $resolvedBios).Path
$source = if ([IO.Path]::IsPathRooted($BuildDir)) { $BuildDir } else { Join-Path $script:RepoRoot $BuildDir }
$testRoot = Join-Path $script:RepoRoot ("build/validation/" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testRoot -Force | Out-Null

# Exercise only packaged DLLs, with no developer PATH or player saves.
foreach ($name in @('SacredStonesRecomp.exe', 'SDL2.dll', 'libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll', 'game.toml')) {
    $file = Join-Path $source $name
    Assert-File -Path $file -Label "Release file"
    Copy-Item -LiteralPath $file -Destination $testRoot
}

$scenarios = @('boot', 'reload')
if ($ExtendedInput) { $scenarios += 'input' }
foreach ($scenario in $scenarios) {
    $frames = if ($scenario -eq 'input') { 3600 } else { 1200 }
    $info = [Diagnostics.ProcessStartInfo]::new()
    $info.FileName = Join-Path $testRoot 'SacredStonesRecomp.exe'
    $info.WorkingDirectory = $testRoot
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    foreach ($key in @($info.Environment.Keys)) {
        if ($key -like 'GBARECOMP_*') { $info.Environment.Remove($key) | Out-Null }
    }
    $info.Environment['PATH'] = "$env:SystemRoot\System32;$env:SystemRoot"
    $info.Environment['GBARECOMP_DEBUG_BOOT'] = '1'
    if ($scenario -eq 'input') { $info.Environment['GBARECOMP_DEMO_INPUT'] = 'menu' }
    foreach ($arg in @('--bios', $resolvedBios, '--rom', $resolvedRom, '--frames', "$frames")) {
        $info.ArgumentList.Add($arg)
    }
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $info
    try {
        $process.Start() | Out-Null
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        $timedOut = -not $process.WaitForExit($TimeoutSeconds * 1000)
        if ($timedOut) { $process.Kill($true); $process.WaitForExit() }
        $log = $stdout.GetAwaiter().GetResult() + "`n" + $stderr.GetAwaiter().GetResult()
        $logPath = Join-Path $testRoot "$scenario.log"
        [IO.File]::WriteAllText($logPath, $log)
        if ($timedOut -or $process.ExitCode -ne 0) { throw "Release $scenario failed or timed out. Log: $logPath" }
        if ($log -notmatch 'bios_backend=LLE' -or
            $log -notmatch "ppu_frames=$frames\b" -or
            $log -notmatch 'self_heal_coverage=FULLY_STATIC dispatch_misses=0 interpreted_insns=0' -or
            $log -match 'compile FAILED|hang-watchdog|SELF-HEAL bridge hit') {
            throw "Release $scenario failed runtime checks. Log: $logPath"
        }
        if ($scenario -eq 'reload' -and $log -notmatch 'save_loaded .*size=32768/32768') {
            throw "Release did not reload SRAM. Log: $logPath"
        }
        $save = Join-Path $testRoot 'saves/SacredStonesRecomp.sav'
        if (-not (Test-Path -LiteralPath $save) -or (Get-Item -LiteralPath $save).Length -ne 32768) {
            throw "Release $scenario did not write the expected 32 KiB SRAM file. Log: $logPath"
        }
        Write-Host "PASS: $scenario ($frames frames). Log: $logPath"
    } finally {
        $process.Dispose()
    }
}
