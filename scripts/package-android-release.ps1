param(
    [string] $Version = '0.1.11',
    [ValidateRange(7, 2147483647)][int] $VersionCode = 7,
    [string] $SigningDirectory = (Join-Path $env:LOCALAPPDATA 'SacredStonesRecomp/signing'),
    [switch] $InitializeSigning
)
. "$PSScriptRoot/common.ps1"
if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw 'Use a numeric release version.' }
$tools = Join-Path $script:RepoRoot 'build/android-tools'
$jdk = (Get-ChildItem "$tools/jdk" -Directory | Select-Object -First 1).FullName
$sdk = Join-Path $tools 'sdk'
$key = Join-Path $SigningDirectory 'release.jks'
$legacy = Join-Path $SigningDirectory 'legacy-development.keystore'
$credentials = Join-Path $SigningDirectory 'release.credentials.xml'
$lineage = Join-Path $SigningDirectory 'release.lineage'
$saved = @{}
foreach ($name in @('JAVA_HOME','PATH','ANDROID_HOME','ANDROID_SDK_ROOT','ANDROID_USER_HOME',
                   'GRADLE_USER_HOME','GBARECOMP_KEYSTORE','GBARECOMP_KEYSTORE_PASSWORD',
                   'GBARECOMP_KEY_ALIAS','GBARECOMP_KEY_PASSWORD')) {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
}
try {
    $env:JAVA_HOME = $jdk
    $env:PATH = "$jdk/bin;$env:PATH"
    $env:ANDROID_HOME = $sdk
    $env:ANDROID_SDK_ROOT = $sdk
    $env:ANDROID_USER_HOME = Join-Path $tools 'android-user-home'
    $env:GRADLE_USER_HOME = Join-Path $tools 'gradle-user-home'
    $signer = Join-Path $sdk 'build-tools/35.0.0/apksigner.bat'
    if ($InitializeSigning) {
        if (Test-Path $SigningDirectory) { throw 'Signing directory already exists; refusing to replace keys.' }
        New-Item -ItemType Directory -Path $SigningDirectory | Out-Null
        $user = [Security.Principal.WindowsIdentity]::GetCurrent().Name
        & icacls $SigningDirectory /inheritance:r /grant:r "${user}:(OI)(CI)F" 'SYSTEM:(OI)(CI)F' | Out-Null
        if ($LASTEXITCODE) { throw 'Cannot protect signing directory.' }
        $random = [Security.Cryptography.RandomNumberGenerator]::GetBytes(32)
        $password = [Convert]::ToBase64String($random)
        $secure = ConvertTo-SecureString $password -AsPlainText -Force
        [pscredential]::new('sacredstones-release', $secure) | Export-Clixml $credentials
        $env:GBARECOMP_KEYSTORE_PASSWORD = $password
        & "$jdk/bin/keytool.exe" -genkeypair -keystore $key -storetype JKS `
            -alias sacredstones-release -keyalg RSA -keysize 3072 -validity 10000 `
            -dname 'CN=SacredStonesRecomp, OU=Android Release, O=Lordingard' `
            -storepass:env GBARECOMP_KEYSTORE_PASSWORD -keypass:env GBARECOMP_KEYSTORE_PASSWORD
        if ($LASTEXITCODE) { throw 'Release key creation failed; keep the directory for recovery.' }
        Copy-Item -LiteralPath "$tools/android-user-home/debug.keystore" -Destination $legacy
        & $signer rotate --out $lineage --old-signer --ks $legacy --ks-key-alias androiddebugkey `
            --ks-pass pass:android --key-pass pass:android --set-installed-data true `
            --set-rollback false --set-permission false --set-shared-uid false --set-auth false `
            --new-signer --ks $key --ks-key-alias sacredstones-release `
            --ks-pass env:GBARECOMP_KEYSTORE_PASSWORD --key-pass env:GBARECOMP_KEYSTORE_PASSWORD
        if ($LASTEXITCODE) { throw 'Signing lineage creation failed.' }
    }
    foreach ($file in @($key, $legacy, $credentials, $lineage)) { Assert-File $file 'Release signing material' }
    $credential = Import-Clixml $credentials
    $env:GBARECOMP_KEYSTORE = $key
    $env:GBARECOMP_KEYSTORE_PASSWORD = $credential.GetNetworkCredential().Password
    $env:GBARECOMP_KEY_ALIAS = $credential.UserName
    $env:GBARECOMP_KEY_PASSWORD = $env:GBARECOMP_KEYSTORE_PASSWORD
    $gradle = Join-Path $tools 'gradle/gradle-8.11.1/bin/gradle.bat'
    & $gradle -p "$script:RepoRoot/android" :app:assembleRelease --no-daemon --console=plain `
        -PgbaAbis=arm64-v8a -PgbaNativeJobs=2 "-PgbaVersionName=$Version" "-PgbaVersionCode=$VersionCode"
    if ($LASTEXITCODE) { throw 'Android release build failed.' }
    $source = Join-Path $script:RepoRoot 'android/app/build/outputs/apk/release/app-release.apk'
    $output = Join-Path $script:RepoRoot "dist/android/SacredStonesRecomp-$Version-android-arm64.apk"
    New-Item -ItemType Directory -Force -Path (Split-Path $output) | Out-Null
    # Supply both lineage members; supported Android versions use production v3.
    & $signer sign --ks $legacy --ks-key-alias androiddebugkey --ks-pass pass:android --key-pass pass:android `
        --next-signer --ks $key --ks-key-alias $credential.UserName `
        --ks-pass env:GBARECOMP_KEYSTORE_PASSWORD --key-pass env:GBARECOMP_KEYSTORE_PASSWORD `
        --lineage $lineage --rotation-min-sdk-version 28 --debuggable-apk-permitted false `
        --out $output $source
    if ($LASTEXITCODE) { throw 'Production signing failed.' }
    & "$PSScriptRoot/test-android-package.ps1" -ApkPath $output -SdkRoot $sdk -JavaHome $jdk
    $metadata = & "$sdk/build-tools/35.0.0/aapt.exe" dump badging $output
    if ($LASTEXITCODE -or ($metadata -match 'application-debuggable')) { throw 'Release APK must not be debuggable.' }
    if (-not ($metadata -match "versionCode='$VersionCode' versionName='$Version'")) { throw 'Wrong release version.' }
    foreach ($api in @(28, 31, 35)) {
        & $signer verify --min-sdk-version $api --max-sdk-version $api $output
        if ($LASTEXITCODE) { throw "Signature invalid on API $api." }
    }
    $certificates = & $signer verify --verbose --print-certs $output
    if ($LASTEXITCODE) { throw 'Certificate inspection failed.' }
    $pin = Get-Content (Join-Path $script:RepoRoot 'android/release-signing.json') -Raw | ConvertFrom-Json
    if (-not ($certificates -match "Signer #1 certificate SHA-256 digest: $($pin.certificateSha256)$")) {
        throw 'Release signer does not match the reviewed public certificate.'
    }
    $certificates | Write-Host
    $hash = (Get-FileHash $output -Algorithm SHA256).Hash.ToLowerInvariant()
    [IO.File]::WriteAllText("$output.sha256", "$hash  $([IO.Path]::GetFileName($output))`n", [Text.UTF8Encoding]::new($false))
    Write-Host "Production APK: $output"
    Write-Host "Private signing material (never commit): $SigningDirectory"
} finally {
    foreach ($name in $saved.Keys) { [Environment]::SetEnvironmentVariable($name, $saved[$name], 'Process') }
    $password = $null
    $credential = $null
}
