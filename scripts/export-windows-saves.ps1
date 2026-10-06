param(
    [Parameter(Mandatory=$true)][string]$SavePath,
    [Parameter(Mandatory=$true)][string]$RomPath,
    [Parameter(Mandatory=$true)][string]$Destination
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
$destinationPath = [IO.Path]::GetFullPath($Destination)
if ([IO.Path]::GetExtension($destinationPath) -ine '.zip') { throw 'Choose a ZIP destination.' }
$sources = @([pscustomobject]@{ Path=[IO.Path]::GetFullPath($SavePath); Entry='saves/SacredStonesRecomp.sav'; Battery=$true })
for ($slot=1; $slot -le 9; $slot++) {
    $sources += [pscustomobject]@{ Path=[IO.Path]::ChangeExtension([IO.Path]::GetFullPath($RomPath), ".state$slot"); Entry="roms/sacred_stones_usa.state$slot"; Battery=$false }
}
$suspend = [IO.Path]::ChangeExtension([IO.Path]::GetFullPath($RomPath), '.suspend.state')
$sources += [pscustomobject]@{ Path=$suspend; Entry='roms/sacred_stones_usa.suspend.state'; Battery=$false }
if ($destinationPath -ieq [IO.Path]::GetFullPath($RomPath) -or
    @($sources | Where-Object { $_.Path -ieq $destinationPath }).Count) {
    throw 'The destination must not replace the ROM or a save.'
}
$streams = @()
$temporary = $destinationPath + '.tmp.' + [guid]::NewGuid().ToString('N')
try {
    foreach ($source in $sources) {
        if (-not [IO.File]::Exists($source.Path)) { continue }
        $item = Get-Item -LiteralPath $source.Path -Force
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Linked saves are not exported.' }
        $stream = [IO.File]::Open($source.Path, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
        $streams += [pscustomobject]@{ Stream=$stream; Entry=$source.Entry }
        if (($source.Battery -and $stream.Length -ne 32768) -or
            (-not $source.Battery -and ($stream.Length -lt 1 -or $stream.Length -gt 16777216))) {
            throw 'Unexpected save size.'
        }
    }
    if (-not $streams.Count) { throw 'No saved games or states to export.' }
    $output = [IO.File]::Open($temporary, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
    try {
        $zip = [IO.Compression.ZipArchive]::new($output, [IO.Compression.ZipArchiveMode]::Create, $true)
        try {
            $entry = $zip.CreateEntry('backup.json').Open()
            try {
                $metadata = '{"schema":1,"game":"SacredStonesRecomp","romSha1":"c25b145e37456171ada4b0d440bf88a19f4d509f"}'
                $bytes = [Text.Encoding]::UTF8.GetBytes($metadata)
                $entry.Write($bytes, 0, $bytes.Length)
            } finally { $entry.Dispose() }
            foreach ($source in $streams) {
                $entry = $zip.CreateEntry($source.Entry).Open()
                try { $source.Stream.CopyTo($entry) } finally { $entry.Dispose() }
            }
        } finally { $zip.Dispose() }
    } finally { $output.Dispose() }
    if ([IO.File]::Exists($destinationPath)) {
        [IO.File]::Replace($temporary, $destinationPath, [System.Management.Automation.Language.NullString]::Value)
    } else { [IO.File]::Move($temporary, $destinationPath) }
} finally {
    foreach ($source in $streams) { $source.Stream.Dispose() }
    if ([IO.File]::Exists($temporary)) { [IO.File]::Delete($temporary) }
}
