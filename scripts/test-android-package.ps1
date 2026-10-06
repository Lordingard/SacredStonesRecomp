param(
    [string] $ApkPath,
    [ValidateSet('arm64-v8a', 'x86_64')][string] $Abi = 'arm64-v8a',
    [string] $SdkRoot,
    [string] $JavaHome
)
. "$PSScriptRoot/common.ps1"
if (-not $ApkPath) { $ApkPath = Join-Path $script:RepoRoot 'android/app/build/outputs/apk/debug/app-debug.apk' }
if (-not $SdkRoot) { $SdkRoot = Join-Path $script:RepoRoot 'build/android-tools/sdk' }
if (-not $JavaHome) {
    $JavaHome = (Get-ChildItem (Join-Path $script:RepoRoot 'build/android-tools/jdk') -Directory |
        Where-Object { Test-Path (Join-Path $_.FullName 'bin/java.exe') } | Select-Object -First 1).FullName
}
Assert-File $ApkPath 'Android APK'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($ApkPath)
try {
    foreach ($name in @("lib/$Abi/libmain.so", "lib/$Abi/libSDL2.so", "lib/$Abi/libc++_shared.so",
                         'assets/payload/variants/sacred_stones/game.toml', 'classes.dex', 'AndroidManifest.xml')) {
        if (-not $zip.GetEntry($name)) { throw "Missing APK entry: $name" }
    }
    foreach ($entry in $zip.Entries) {
        if ($entry.FullName -match '\.(gba|agb|sav|state[1-9]|cpp|h)$|gba_bios\.bin|PRIVATE-rom-included') {
            throw "Private data or source in APK: $($entry.FullName)"
        }
        if ($entry.FullName -notmatch '^lib/[^/]+/[^/]+\.so$') { continue }
        if (-not $entry.FullName.StartsWith("lib/$Abi/")) { throw "Unexpected ABI: $($entry.FullName)" }
        $stream = $entry.Open()
        $memory = [IO.MemoryStream]::new()
        try { $stream.CopyTo($memory); $bytes = $memory.ToArray() }
        finally { $stream.Dispose(); $memory.Dispose() }
        if ($bytes.Length -lt 64 -or $bytes[0] -ne 0x7f -or
            [Text.Encoding]::ASCII.GetString($bytes, 1, 3) -ne 'ELF' -or $bytes[4] -ne 2 -or $bytes[5] -ne 1) {
            throw "Invalid ELF64 library: $($entry.FullName)"
        }
        $machine = [BitConverter]::ToUInt16($bytes, 18)
        $expected = if ($Abi -eq 'arm64-v8a') { 183 } else { 62 }
        if ($machine -ne $expected) { throw "Wrong native architecture: $($entry.FullName)" }
        $offset = [BitConverter]::ToUInt64($bytes, 32)
        $size = [BitConverter]::ToUInt16($bytes, 54)
        $count = [BitConverter]::ToUInt16($bytes, 56)
        if ($size -lt 56 -or $count -eq 0 -or $offset + [uint64]$size * $count -gt $bytes.Length) {
            throw 'Invalid ELF program headers.'
        }
        for ($i = 0; $i -lt $count; $i++) {
            $header = [int]($offset + $size * $i)
            if ([BitConverter]::ToUInt32($bytes, $header) -ne 1) { continue }
            $alignment = [BitConverter]::ToUInt64($bytes, $header + 48)
            if ($alignment -lt 16384) { throw "Native library lacks 16 KiB alignment: $($entry.FullName)" }
        }
    }
} finally { $zip.Dispose() }
$previousJava = $env:JAVA_HOME
$previousPath = $env:PATH
try {
    $env:JAVA_HOME = $JavaHome
    $env:PATH = "$JavaHome/bin;$previousPath"
    & (Join-Path $SdkRoot 'build-tools/35.0.0/apksigner.bat') verify --verbose $ApkPath
    if ($LASTEXITCODE) { throw 'APK signature verification failed.' }
    $metadata = & (Join-Path $SdkRoot 'build-tools/35.0.0/aapt.exe') dump badging $ApkPath
    if ($LASTEXITCODE) { throw 'APK manifest verification failed.' }
    if (-not ($metadata -match "package: name='com.lordingard.sacredstonesrecomp'")) { throw 'Wrong application ID.' }
    if (-not ($metadata -match "sdkVersion:'28'")) { throw 'Unexpected minimum Android API.' }
    Write-Host "PASS: signed $Abi APK, application identity, private-asset exclusion and 16 KiB native alignment."
    Get-FileHash $ApkPath -Algorithm SHA256
} finally {
    $env:JAVA_HOME = $previousJava
    $env:PATH = $previousPath
}
