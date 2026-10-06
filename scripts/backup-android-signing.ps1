param(
    [Parameter(Mandatory = $true)][string] $Destination,
    [string] $SigningDirectory = (Join-Path $env:LOCALAPPDATA 'SacredStonesRecomp/signing')
)
. "$PSScriptRoot/common.ps1"
if (Test-Path $Destination) { throw 'Use a new private destination; refusing to overwrite a backup.' }
$password = Read-Host 'Portable keystore password (save separately in your password manager)' -AsSecureString
$confirmation = Read-Host 'Confirm portable keystore password' -AsSecureString
$credential = [pscredential]::new('sacredstones-release', $password)
$plain = $credential.GetNetworkCredential().Password
if ($plain.Length -lt 16 -or $plain -ne [pscredential]::new('confirm', $confirmation).GetNetworkCredential().Password) {
    throw 'Passwords must match and contain at least 16 characters.'
}
$source = Import-Clixml (Join-Path $SigningDirectory 'release.credentials.xml')
$jdk = (Get-ChildItem (Join-Path $script:RepoRoot 'build/android-tools/jdk') -Directory | Select-Object -First 1).FullName
try {
    New-Item -ItemType Directory -Path $Destination | Out-Null
    $user = [Security.Principal.WindowsIdentity]::GetCurrent().Name
    & icacls $Destination /inheritance:r /grant:r "${user}:(OI)(CI)F" 'SYSTEM:(OI)(CI)F' | Out-Null
    if ($LASTEXITCODE) { throw 'Cannot protect backup directory.' }
    $env:SACREDSTONES_SOURCE_PASSWORD = $source.GetNetworkCredential().Password
    $env:SACREDSTONES_BACKUP_PASSWORD = $plain
    & "$jdk/bin/keytool.exe" -importkeystore -noprompt `
        -srckeystore (Join-Path $SigningDirectory 'release.jks') -srcalias $source.UserName `
        -srcstorepass:env SACREDSTONES_SOURCE_PASSWORD -srckeypass:env SACREDSTONES_SOURCE_PASSWORD `
        -destkeystore (Join-Path $Destination 'release.jks') -deststoretype JKS `
        -deststorepass:env SACREDSTONES_BACKUP_PASSWORD -destkeypass:env SACREDSTONES_BACKUP_PASSWORD
    if ($LASTEXITCODE) { throw 'Portable key export failed.' }
    & "$jdk/bin/keytool.exe" -list -keystore (Join-Path $Destination 'release.jks') `
        -storepass:env SACREDSTONES_BACKUP_PASSWORD -alias $source.UserName | Out-Null
    if ($LASTEXITCODE) { throw 'Portable key verification failed.' }
    Copy-Item -LiteralPath (Join-Path $SigningDirectory 'release.lineage') -Destination $Destination
    Copy-Item -LiteralPath (Join-Path $SigningDirectory 'legacy-development.keystore') -Destination $Destination
    Write-Host 'Portable encrypted signing backup created. Store its password separately.'
} finally {
    Remove-Item Env:SACREDSTONES_SOURCE_PASSWORD, Env:SACREDSTONES_BACKUP_PASSWORD -ErrorAction SilentlyContinue
    $plain = $null
    $source = $null
}
