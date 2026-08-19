$ErrorActionPreference = "Stop"

$script:RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$script:LocalConfigPath = Join-Path $script:RepoRoot "config/project.local.ps1"
$script:DefaultGeneratedProject = Join-Path $script:RepoRoot ".generated/gbarecomp"
$script:DefaultRomPath = Join-Path (Split-Path -Parent $script:RepoRoot) "Fire Emblem - The Sacred Stones (U).gba"
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

function ConvertTo-WslPath {
    param([Parameter(Mandatory = $true)][string] $Path)

    $resolved = if (Test-Path -LiteralPath $Path) {
        (Resolve-Path -LiteralPath $Path).Path
    } else {
        [System.IO.Path]::GetFullPath($Path)
    }

    $fullPath = [System.IO.Path]::GetFullPath($resolved)
    $root = [System.IO.Path]::GetPathRoot($fullPath)
    if ($root -notmatch "^([A-Za-z]):\\$") {
        throw "Could not convert non-drive Windows path to WSL path: $Path"
    }

    $drive = $Matches[1].ToLowerInvariant()
    $relative = $fullPath.Substring($root.Length).Replace("\", "/")
    return "/mnt/$drive/$relative"
}

function Quote-WslShellArgument {
    param([Parameter(Mandatory = $true)][string] $Value)

    return "'$($Value.Replace("'", "'\''"))'"
}

function Test-WslAvailable {
    if (-not (Get-Command wsl.exe -ErrorAction SilentlyContinue)) {
        return $false
    }

    & wsl.exe true *> $null
    return ($LASTEXITCODE -eq 0)
}

function Assert-WslAvailable {
    if (-not (Test-WslAvailable)) {
        throw "WSL is not ready. Install a WSL distribution first, then rerun this command."
    }
}

function Invoke-WslBash {
    param([Parameter(Mandatory = $true)][string] $Command)

    Assert-WslAvailable
    & wsl.exe bash -lc $Command
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}
