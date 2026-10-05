param(
    [string] $BuildDir = "build/runner-mingw",
    [string] $RomPath,
    [string] $BiosPath,
    [ValidateRange(1, 600)][int] $TimeoutSeconds = 60,
    [switch] $ExtendedInput,
    [switch] $WindowedInput
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

$scenarios = @('boot', 'reload', 'save-path', 'save-alias', 'save-relative', 'locked-save')
if ($ExtendedInput) { $scenarios += 'input' }
if ($WindowedInput) { $scenarios += 'window-input' }
foreach ($scenario in $scenarios) {
    $frames = if ($scenario -eq 'input') { 3600 } else { 1200 }
    $defaultSave = Join-Path $testRoot 'saves/SacredStonesRecomp.sav'
    $save = $defaultSave
    $defaultHash = if (Test-Path -LiteralPath $defaultSave) { (Get-FileHash -LiteralPath $defaultSave).Hash } else { '' }
    $saveLock = $null
    if ($scenario -eq 'save-path') {
        $legacy = Join-Path $testRoot ([IO.Path]::GetFileNameWithoutExtension($resolvedRom) + '.sav')
        $legacyBytes = [byte[]]::new(32768)
        [Array]::Fill[byte]($legacyBytes, 0x55)
        [IO.File]::WriteAllBytes($legacy, $legacyBytes)
    }
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
    if ($scenario -eq 'window-input') {
        $trace = Join-Path $testRoot 'window-input.csv'
        $buttons = @(8, 1, 1, 128, 1, 16, 2, 1, 32, 1, 64, 2)
        $events = for ($frame = 0; $frame -le $frames; $frame += 6) {
            $keys = if (($frame / 6) % 2 -eq 1) { 0x3ff } else {
                0x3ff -band (-bnot $buttons[[int][Math]::Floor($frame / 12) % $buttons.Count])
            }
            '{0},0x{1:X3}' -f $frame, $keys
        }
        [IO.File]::WriteAllLines($trace, [string[]]$events)
        $info.Environment['GBARECOMP_INPUT_REPLAY'] = $trace
    }
    $runArgs = @('--bios', $resolvedBios, '--frames', "$frames")
    if ($scenario -eq 'window-input') { $runArgs += '--window' }
    $runArgs += @('--rom', $resolvedRom)
    if ($scenario -in @('save-path', 'save-alias', 'save-relative', 'locked-save')) {
        $save = Join-Path $testRoot "alternate/$scenario.sav"
        $flag = if ($scenario -eq 'save-alias') { '--save' } else { '--save-path' }
        $saveArg = $save
        if ($scenario -eq 'save-relative') {
            $saveArg = 'relative-save.sav'
            $save = Join-Path $testRoot $saveArg
        }
        $runArgs += @($flag, $saveArg)
    }
    if ($scenario -eq 'locked-save') {
        [IO.Directory]::CreateDirectory((Split-Path -Parent $save)) | Out-Null
        [IO.File]::WriteAllBytes($save, [byte[]]::new(32768))
        $lockedHash = (Get-FileHash -LiteralPath $save).Hash
        $saveLock = [IO.File]::Open($save, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
    }
    foreach ($arg in $runArgs) {
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
        if ($scenario -eq 'locked-save') {
            $saveLock.Dispose()
            $saveLock = $null
            if ($timedOut -or $process.ExitCode -eq 0 -or
                $log -notmatch 'save replacement failed' -or
                (Get-FileHash -LiteralPath $save).Hash -ne $lockedHash -or
                -not (Test-Path -LiteralPath "$save.tmp")) {
                throw "Locked save was not preserved. Log: $logPath"
            }
            Write-Host "PASS: locked save preserved with recovery file. Log: $logPath"
            continue
        }
        if ($timedOut -or $process.ExitCode -ne 0) { throw "Release $scenario failed or timed out. Log: $logPath" }
        if (($scenario -ne 'window-input' -and $log -notmatch 'bios_backend=LLE') -or
            $log -notmatch "ppu_frames=$frames\b" -or
            $log -notmatch 'self_heal_coverage=FULLY_STATIC dispatch_misses=0 interpreted_insns=0' -or
            $log -match 'compile FAILED|hang-watchdog|SELF-HEAL bridge hit') {
            throw "Release $scenario failed runtime checks. Log: $logPath"
        }
        if ($scenario -eq 'window-input' -and $log -notmatch "frames_presented=$frames\b") {
            throw "Windowed replay did not present the requested frames. Log: $logPath"
        }
        if ($scenario -eq 'reload' -and $log -notmatch 'save_loaded .*size=32768/32768') {
            throw "Release did not reload SRAM. Log: $logPath"
        }
        if (-not (Test-Path -LiteralPath $save) -or (Get-Item -LiteralPath $save).Length -ne 32768) {
            throw "Release $scenario did not write the expected 32 KiB SRAM file. Log: $logPath"
        }
        if ($save -ne $defaultSave -and (Get-FileHash -LiteralPath $defaultSave).Hash -ne $defaultHash) {
            throw "Explicit save path modified the default save. Log: $logPath"
        }
        Write-Host "PASS: $scenario ($frames frames). Log: $logPath"
    } finally {
        if ($saveLock) { $saveLock.Dispose() }
        $process.Dispose()
    }
}
