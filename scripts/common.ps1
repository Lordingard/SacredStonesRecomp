$ErrorActionPreference = "Stop"

$script:RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$script:LocalConfigPath = Join-Path $script:RepoRoot "config/project.local.ps1"
$script:DefaultGeneratedProject = Join-Path $script:RepoRoot ".generated/gbarecomp"
$script:ExpectedRomSha1 = "C25B145E37456171ADA4B0D440BF88A19F4D509F"

if (Test-Path -LiteralPath $script:LocalConfigPath) {
    . $script:LocalConfigPath
}

function Resolve-Setting {
    param(
        [string] $ExplicitValue,
        [string] $EnvironmentName,
        [string] $ScriptVariableName,
        [string] $DefaultValue
    )

    if ($ExplicitValue) {
        return $ExplicitValue
    }
    $envValue = [Environment]::GetEnvironmentVariable($EnvironmentName)
    if ($envValue) {
        return $envValue
    }
    $var = Get-Variable -Name $ScriptVariableName -Scope Script -ErrorAction SilentlyContinue
    if ($var -and $var.Value) {
        return [string]$var.Value
    }
    return $DefaultValue
}

function Get-NativeCMake {
    $cmake = Get-Command cmake.exe -ErrorAction SilentlyContinue
    $cmakeSource = if ($cmake) { $cmake.Source.Replace("\", "/") } else { "" }

    if ($cmake -and $cmakeSource -notmatch "/(msys[^/]*|cygwin[^/]*)/" -and
        $cmakeSource -notmatch "/Git/usr/bin/") {
        return $cmake.Source
    }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $vswhere) {
        $vsCmake = & $vswhere -latest -products * `
            -find "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" |
            Select-Object -First 1
        if ($vsCmake) {
            return $vsCmake
        }
    }

    throw "Native Windows CMake 3.20+ was not found. Install CMake or the Visual Studio C++ CMake tools."
}

function Assert-File {
    param(
        [Parameter(Mandatory = $true)][string] $Path,
        [Parameter(Mandatory = $true)][string] $Label
    )
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Label not found: $Path"
    }
}

function Assert-Rom {
    param([Parameter(Mandatory = $true)][string] $Path)
    Assert-File -Path $Path -Label "ROM"
    $hash = (Get-FileHash -Algorithm SHA1 -LiteralPath $Path).Hash.ToUpperInvariant()
    if ($hash -ne $script:ExpectedRomSha1) {
        throw "Unexpected ROM SHA1: $hash. Expected $script:ExpectedRomSha1 for Fire Emblem: The Sacred Stones (U)."
    }
}
