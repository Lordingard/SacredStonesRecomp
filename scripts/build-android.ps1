param(
    [string] $RomPath,
    [Parameter(Mandatory = $true)][string] $BiosPath,
    [string] $MingwBin = 'C:/tools/msys64/mingw64/bin',
    [string] $PythonPath = 'python',
    [string] $SdkRoot,
    [string] $JavaHome,
    [ValidateSet('arm64-v8a', 'x86_64')][string] $Abi = 'arm64-v8a',
    [ValidateRange(1, 4)][int] $Jobs = 2
)

. "$PSScriptRoot/common.ps1"
$lock = Get-Content (Join-Path $script:RepoRoot 'android/dependencies.json') -Raw | ConvertFrom-Json
$tools = Join-Path $script:RepoRoot 'build/android-tools'
if (-not $SdkRoot) { $SdkRoot = if ($env:ANDROID_HOME) { $env:ANDROID_HOME } else { Join-Path $tools 'sdk' } }
if (-not $JavaHome) {
    $localJdk = Get-ChildItem (Join-Path $tools 'jdk') -Directory -ErrorAction SilentlyContinue |
        Where-Object { Test-Path (Join-Path $_.FullName 'bin/java.exe') } | Select-Object -First 1
    $JavaHome = if ($localJdk) { $localJdk.FullName } else { $env:JAVA_HOME }
}
if (-not $JavaHome -or -not (Test-Path (Join-Path $JavaHome 'bin/java.exe'))) { throw 'Provide a JDK 17 using -JavaHome.' }
foreach ($component in @('platforms/android-35/android.jar', 'build-tools/35.0.0/aapt2.exe',
                         'ndk/27.1.12297006/build/cmake/android.toolchain.cmake', 'cmake/3.22.1/bin/cmake.exe')) {
    Assert-File (Join-Path $SdkRoot $component) 'Android SDK component'
}
$resolvedRom = Resolve-Setting $RomPath 'FE8_ROM' 'RomPath' $script:DefaultRomPath
Assert-Rom $resolvedRom
Assert-File $BiosPath 'GBA BIOS'
if ((Get-FileHash $BiosPath -Algorithm SHA1).Hash -ne '300C20DF6731A33952DED8C436F7F186D25D3492') {
    throw 'The BIOS must match the reviewed 16 KiB retail dump.'
}
$previousPath = $env:PATH
$previousJava = $env:JAVA_HOME
$previousSdk = $env:ANDROID_HOME
$previousSdkRoot = $env:ANDROID_SDK_ROOT
$previousCore = $env:GBARECOMP_CORE
$previousGradleHome = $env:GRADLE_USER_HOME
$previousAndroidUserHome = $env:ANDROID_USER_HOME
try {
    $env:JAVA_HOME = $JavaHome
    $env:ANDROID_HOME = $SdkRoot
    $env:ANDROID_SDK_ROOT = $SdkRoot
    $env:GRADLE_USER_HOME = Join-Path $tools 'gradle-user-home'
    $env:ANDROID_USER_HOME = Join-Path $tools 'android-user-home'
    $env:PATH = "$JavaHome/bin;$MingwBin;C:/Program Files/Git/usr/bin;$previousPath"
    $deps = Join-Path $script:RepoRoot 'build/android-deps'
    New-Item -ItemType Directory -Force -Path $deps | Out-Null
    foreach ($name in @('gbarecomp', 'recomp-ui')) {
        $dep = $lock.$name
        $destination = Join-Path $deps $name
        if (-not (Test-Path $destination)) {
            & git clone --no-checkout $dep.url $destination
            if ($LASTEXITCODE) { throw "Cannot clone $name" }
            & git -C $destination checkout --detach $dep.commit
            if ($LASTEXITCODE) { throw "Cannot pin $name" }
        }
        $head = & git -C $destination rev-parse HEAD
        if ($LASTEXITCODE -or $head -ne $dep.commit) { throw "Unexpected $name revision; keep Android dependencies pinned." }
        if ($dep.PSObject.Properties['patches']) {
            $allowedPaths = @($dep.patches | ForEach-Object { $_.patchedPath })
            $modified = @(& git -C $destination diff --name-only)
            if ($LASTEXITCODE -or @($modified | Where-Object { $_ -notin $allowedPaths }).Count) {
                throw 'Unexpected Android engine changes.'
            }
            & git -C $destination diff --cached --quiet
            if ($LASTEXITCODE) { throw 'Unexpected staged Android dependency changes.' }
            foreach ($entry in $dep.patches) {
                $patch = Join-Path $script:RepoRoot "android/$($entry.patch)"
                & git -C $destination apply --reverse --check $patch 2>$null
                if ($LASTEXITCODE) {
                    & git -C $destination diff --quiet -- $entry.patchedPath
                    if ($LASTEXITCODE) { throw "Unexpected modifications in $($entry.patchedPath)." }
                    & git -C $destination apply $patch
                    if ($LASTEXITCODE) { throw "Cannot apply Android patch to $name." }
                }
                $text = [IO.File]::ReadAllText((Join-Path $destination $entry.patchedPath)).Replace("`r`n", "`n")
                $sha = [Security.Cryptography.SHA256]::Create()
                try { $hash = [Convert]::ToHexString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($text))) }
                finally { $sha.Dispose() }
                if ($hash -ne $entry.patchedSha256) { throw 'Patched Android engine does not match the reviewed content.' }
            }
            $statusArgs = @('-C', $destination, 'status', '--porcelain', '--untracked-files=no', '--', '.')
            $statusArgs += @($allowedPaths | ForEach-Object { ":(exclude)$_" })
            $dirty = & git @statusArgs
        } else {
            $dirty = & git -C $destination status --porcelain --untracked-files=no
        }
        if ($LASTEXITCODE -or $dirty) { throw "Android dependency $name has local modifications." }
        & git -C $destination submodule update --init --recursive --jobs $Jobs
        if ($LASTEXITCODE) { throw "Cannot initialize $name submodules" }
    }
    $engine = Join-Path $deps 'gbarecomp'
    $generatorBuild = Join-Path $script:RepoRoot 'build/android-generator'
    $biosOutput = Join-Path $script:RepoRoot 'build/android-generated-bios'
    $generated = Join-Path $script:RepoRoot '.generated/android'
    & cmake -S $engine -B $generatorBuild -G Ninja "-DCMAKE_C_COMPILER=$MingwBin/gcc.exe" `
        "-DCMAKE_CXX_COMPILER=$MingwBin/g++.exe" '-DCMAKE_BUILD_TYPE=Release'
    if ($LASTEXITCODE) { throw 'Android host generator configure failed.' }
    & cmake --build $generatorBuild --target gba_recompile --parallel $Jobs
    if ($LASTEXITCODE) { throw 'Android host generator build failed.' }
    $env:GBARECOMP_CORE = Join-Path $generatorBuild 'gba_recompile.exe'
    & $PythonPath (Join-Path $engine 'tools/cli.py') build --rom $resolvedRom --output $generated `
        --config (Join-Path $script:RepoRoot 'config/game.fe8u.toml') `
        --symbols (Join-Path $script:RepoRoot 'symbols/imported_symbols.tsv') --force
    if ($LASTEXITCODE) { throw 'Android game generation failed.' }
    New-Item -ItemType Directory -Force -Path $biosOutput | Out-Null
    & $env:GBARECOMP_CORE --bios $BiosPath --config (Join-Path $engine 'bios/gba_bios.toml') --out $biosOutput
    if ($LASTEXITCODE) { throw 'Android BIOS generation failed.' }

    $gradleRoot = Join-Path $tools "gradle/gradle-$($lock.gradle)"
    if (-not (Test-Path (Join-Path $gradleRoot 'bin/gradle.bat'))) {
        $downloads = Join-Path $tools 'downloads'
        New-Item -ItemType Directory -Force -Path $downloads | Out-Null
        $uri = "https://services.gradle.org/distributions/gradle-$($lock.gradle)-bin.zip"
        $expected = (Invoke-RestMethod "$uri.sha256").Trim()
        $archive = Join-Path $downloads "gradle-$($lock.gradle)-bin.zip"
        Invoke-WebRequest $uri -OutFile $archive
        if ((Get-FileHash $archive -Algorithm SHA256).Hash -ne $expected) { throw 'Gradle checksum mismatch.' }
        Expand-Archive $archive -DestinationPath (Join-Path $tools 'gradle') -Force
    }
    & (Join-Path $gradleRoot 'bin/gradle.bat') -p (Join-Path $script:RepoRoot 'android') `
        --no-daemon --console=plain :app:assembleDebug "-PgbaAbis=$Abi" "-PgbaNativeJobs=$Jobs"
    if ($LASTEXITCODE) { throw 'Android APK build failed.' }
    $apk = Join-Path $script:RepoRoot 'android/app/build/outputs/apk/debug/app-debug.apk'
    Assert-File $apk 'Android experimental APK'
    & "$PSScriptRoot/test-android-package.ps1" -ApkPath $apk -Abi $Abi -SdkRoot $SdkRoot -JavaHome $JavaHome
    Write-Host "Experimental APK (no ROM or BIOS): $apk"
} finally {
    $env:PATH = $previousPath
    $env:JAVA_HOME = $previousJava
    $env:ANDROID_HOME = $previousSdk
    $env:ANDROID_SDK_ROOT = $previousSdkRoot
    $env:GBARECOMP_CORE = $previousCore
    $env:GRADLE_USER_HOME = $previousGradleHome
    $env:ANDROID_USER_HOME = $previousAndroidUserHome
}
