param(
    [string] $GbaRecompExe,
    [string] $RomPath,
    [switch] $WriteLocalConfig,
    [switch] $WithFireEmblem8u
)

. "$PSScriptRoot/common.ps1"

$resolvedGbaRecompExe = Resolve-Setting $GbaRecompExe "GBARECOMP_EXE" "GbaRecompExe" "D:\Jeux\GBARecomp\gbarecomp.exe"
$resolvedRomPath = Resolve-Setting $RomPath "FE8_ROM" "RomPath" $script:DefaultRomPath

Assert-File -Path $resolvedGbaRecompExe -Label "GBARecomp CLI"
Assert-Rom -Path $resolvedRomPath
$cmakePath = Get-NativeCMake

Write-Host "GBARecomp: $resolvedGbaRecompExe"
Write-Host "ROM:        $resolvedRomPath"
Write-Host "CMake:      $cmakePath"
Write-Host "ROM SHA1:   $script:ExpectedRomSha1"

if ($WriteLocalConfig) {
    $localConfig = Join-Path $script:RepoRoot "config/project.local.ps1"
    if (Test-Path -LiteralPath $localConfig) {
        Write-Host "Local config already exists: $localConfig"
    } else {
        Copy-Item -LiteralPath (Join-Path $script:RepoRoot "config/project.local.example.ps1") -Destination $localConfig
        Write-Host "Wrote local config: $localConfig"
    }
}

if ($WithFireEmblem8u) {
    $extern = Join-Path $script:RepoRoot "extern"
    $fe8u = Join-Path $extern "fireemblem8u"
    if (Test-Path -LiteralPath $fe8u) {
        Write-Host "fireemblem8u checkout already exists: $fe8u"
    } else {
        New-Item -ItemType Directory -Force -Path $extern | Out-Null
        git clone https://github.com/FireEmblemUniverse/fireemblem8u.git $fe8u
    }
}
