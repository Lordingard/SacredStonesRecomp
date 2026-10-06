$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$root = Join-Path $repo ('build/android-package-tests/' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $root -Force | Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Test-RejectedPackage {
    param([string] $Name, [string] $Expected, [int] $Machine = 183,
          [int] $Alignment = 16384, [string] $Extra = '', [switch] $MissingMain)
    $path = Join-Path $root "$Name.apk"
    $zip = [IO.Compression.ZipFile]::Open($path, [IO.Compression.ZipArchiveMode]::Create)
    try {
        $native = [byte[]]::new(120)
        $native[0] = 0x7f
        [Text.Encoding]::ASCII.GetBytes('ELF').CopyTo($native, 1)
        $native[4] = 2
        $native[5] = 1
        [BitConverter]::GetBytes([uint16]$Machine).CopyTo($native, 18)
        [BitConverter]::GetBytes([uint64]64).CopyTo($native, 32)
        [BitConverter]::GetBytes([uint16]56).CopyTo($native, 54)
        [BitConverter]::GetBytes([uint16]1).CopyTo($native, 56)
        [BitConverter]::GetBytes([uint32]1).CopyTo($native, 64)
        [BitConverter]::GetBytes([uint64]$Alignment).CopyTo($native, 112)
        $names = @('lib/arm64-v8a/libSDL2.so', 'lib/arm64-v8a/libc++_shared.so',
                   'assets/payload/variants/sacred_stones/game.toml', 'classes.dex', 'AndroidManifest.xml')
        if (-not $MissingMain) { $names += 'lib/arm64-v8a/libmain.so' }
        if ($Extra) { $names += $Extra }
        foreach ($entryName in $names) {
            $stream = $zip.CreateEntry($entryName).Open()
            try { if ($entryName.EndsWith('.so')) { $stream.Write($native, 0, $native.Length) } }
            finally { $stream.Dispose() }
        }
    } finally { $zip.Dispose() }
    $message = ''
    try { & (Join-Path $repo 'scripts/test-android-package.ps1') -ApkPath $path | Out-Null }
    catch { $message = $_.Exception.Message }
    if ($message -notlike "*$Expected*") { throw "$Name did not fail as expected: $message" }
    Write-Host "PASS: $Name rejected"
}

Test-RejectedPackage -Name missing-main -Expected 'Missing APK entry' -MissingMain
Test-RejectedPackage -Name wrong-machine -Expected 'Wrong native architecture' -Machine 62
Test-RejectedPackage -Name small-page -Expected 'lacks 16 KiB alignment' -Alignment 4096
Test-RejectedPackage -Name rom-included -Expected 'Private data or source' -Extra 'assets/payload/roms/game.gba'
Test-RejectedPackage -Name bios-included -Expected 'Private data or source' -Extra 'assets/payload/bios/gba_bios.bin'
Test-RejectedPackage -Name save-included -Expected 'Private data or source' -Extra 'assets/payload/saves/test.sav'
