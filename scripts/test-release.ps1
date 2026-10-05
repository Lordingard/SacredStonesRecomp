param(
    [string] $BuildDir = "build/runner-mingw",
    [string] $RomPath,
    [string] $BiosPath,
    [ValidateRange(1, 600)][int] $TimeoutSeconds = 60,
    [switch] $ExtendedInput,
    [switch] $WindowedInput,
    [switch] $DiagnosticCapture
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
if ($WindowedInput) { $scenarios += 'window-missing-controller' }
if ($DiagnosticCapture) {
    $scenarios += 'diagnostics'
    if ($WindowedInput) { $scenarios += 'window-diagnostics' }
}
if ($WindowedInput) { $scenarios += @('window-controls', 'window-controls-invalid') }
foreach ($scenario in $scenarios) {
    $windowed = $scenario -in @('window-input', 'window-diagnostics', 'window-missing-controller', 'window-controls', 'window-controls-invalid')
    $diagnostics = $scenario -in @('diagnostics', 'window-diagnostics')
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
    if ($scenario -in @('window-controls', 'window-controls-invalid')) {
        $controlsPath = Join-Path $testRoot 'runtime-controls.toml'
        $content = if ($scenario -eq 'window-controls') {
            "fast_forward_multiplier = 7`nrewind_enabled = false`nstate_slot = 8`nassist_tools_enabled = true`n"
        } else { 'invalid = [' }
        [IO.File]::WriteAllText($controlsPath, $content)
        $controlsHash = (Get-FileHash -LiteralPath $controlsPath).Hash
        $otherCwd = Join-Path $testRoot 'different-working-directory'
        [IO.Directory]::CreateDirectory($otherCwd) | Out-Null
        $info.WorkingDirectory = $otherCwd
    }
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    foreach ($key in @($info.Environment.Keys)) {
        if ($key -like 'GBARECOMP_*') { $info.Environment.Remove($key) | Out-Null }
    }
    $info.Environment['PATH'] = "$env:SystemRoot\System32;$env:SystemRoot"
    $info.Environment['GBARECOMP_DEBUG_BOOT'] = '1'
    if ($scenario -eq 'window-missing-controller') {
        $info.Environment['GBARECOMP_CONTROLLER_GUID'] = 'ffffffffffffffffffffffffffffffff'
    }
    $mmioDump = Join-Path $testRoot "$scenario-mmio.csv"
    $phaseDump = Join-Path $testRoot "$scenario-phase.csv"
    $cadenceDump = Join-Path $testRoot "$scenario-cadence.csv"
    if ($diagnostics) {
        $info.Environment['GBARECOMP_RUNTIME_TRACE'] = '1'
        $info.Environment['GBARECOMP_MMIO_DUMP'] = $mmioDump
        $info.Environment['GBARECOMP_AUDIO_FIFO_TRACE'] = '1'
        $info.Environment['GBARECOMP_HANG_WATCHDOG'] = '1'
        if ($windowed) {
            $info.Environment['GBARECOMP_FRAME_PHASE'] = $phaseDump
            $info.Environment['GBARECOMP_PRESENT_CADENCE'] = '1'
            $info.Environment['GBARECOMP_PRESENT_CADENCE_DUMP'] = $cadenceDump
        }
    }
    if ($scenario -eq 'input') { $info.Environment['GBARECOMP_DEMO_INPUT'] = 'menu' }
    if ($windowed) {
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
    if ($windowed) { $runArgs += '--window' }
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
        if ((-not $windowed -and $log -notmatch 'bios_backend=LLE') -or
            $log -notmatch "ppu_frames=$frames\b" -or
            $log -notmatch 'self_heal_coverage=FULLY_STATIC dispatch_misses=0 interpreted_insns=0' -or
            $log -match 'compile FAILED|hang-watchdog|SELF-HEAL bridge hit') {
            throw "Release $scenario failed runtime checks. Log: $logPath"
        }
        if ($windowed -and $log -notmatch "frames_presented=$frames\b") {
            throw "Windowed replay did not present the requested frames. Log: $logPath"
        }
        if ($scenario -eq 'window-controls' -and
            $log -notmatch 'runtime_controls_loaded speed=7 rewind=0 slot=8 assist=1') {
            throw "Executable-local controls did not reload from another working directory. Log: $logPath"
        }
        if ($scenario -eq 'window-controls-invalid' -and
            $log -notmatch 'invalid runtime-controls TOML; using defaults') {
            throw "Invalid controls did not fall back safely. Log: $logPath"
        }
        if ($scenario -in @('window-controls', 'window-controls-invalid') -and
            (Get-FileHash -LiteralPath $controlsPath).Hash -ne $controlsHash) {
            throw "Loading controls unexpectedly modified the file. Log: $logPath"
        }
        if ($scenario -eq 'window-missing-controller' -and
            $log -notmatch 'preferred controller unavailable; using available input') {
            throw "Missing-controller fallback was not exercised. Log: $logPath"
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
        if ($diagnostics) {
            if (-not (Test-Path -LiteralPath $mmioDump) -or
                @(Get-Content -LiteralPath $mmioDump -TotalCount 2).Count -lt 2) {
                throw "Enabled MMIO capture produced no records. Log: $logPath"
            }
            if ($windowed -and (-not (Test-Path -LiteralPath $phaseDump) -or
                @(Get-Content -LiteralPath $phaseDump -TotalCount 2).Count -lt 2)) {
                throw "Enabled frame-phase capture produced no records. Log: $logPath"
            }
            if ($windowed -and (-not (Test-Path -LiteralPath $cadenceDump) -or
                @(Get-Content -LiteralPath $cadenceDump -TotalCount 2).Count -lt 2)) {
                throw "Enabled present-cadence capture produced no records. Log: $logPath"
            }
        }
        Write-Host "PASS: $scenario ($frames frames). Log: $logPath"
    } finally {
        if ($saveLock) { $saveLock.Dispose() }
        $process.Dispose()
    }
}
