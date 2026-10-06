param([Parameter(Mandatory=$true)][string]$TestDirectory)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
New-Item -ItemType Directory -Path $TestDirectory -Force | Out-Null
$root=(Resolve-Path -LiteralPath $TestDirectory).Path
$script=Join-Path $PSScriptRoot '../scripts/export-windows-saves.ps1'
$save=Join-Path $root 'battery.sav'
$rom=Join-Path $root 'Fire Emblem [test].gba'
$state=[IO.Path]::ChangeExtension($rom,'.state1')
$zip=Join-Path $root 'backup.zip'
function Require($value,$message) { if (-not $value) { throw $message } }
function Reject($action) {
    $rejected=$false
    try { & $action } catch { $rejected=$true }
    Require $rejected 'Invalid export accepted'
}
Reject { & $script -SavePath $save -RomPath $rom -Destination $zip }
[IO.File]::WriteAllBytes($save,[byte[]]::new(32768))
[IO.File]::WriteAllBytes($state,[byte[]]@(1,2,3,4,5))
[IO.File]::WriteAllText($rom,'ROM must not enter ZIP')
[IO.File]::WriteAllText([IO.Path]::ChangeExtension($rom,'.state10'),'unsupported slot')
$before=@{}
foreach ($file in @($save,$state,$rom)) { $before[$file]=@((Get-FileHash -LiteralPath $file).Hash,(Get-Item -LiteralPath $file).LastWriteTimeUtc) }
& $script -SavePath $save -RomPath $rom -Destination $zip
$archive=[IO.Compression.ZipFile]::OpenRead($zip)
try {
    Require ($archive.Entries.Count -eq 3) 'Unexpected archive entries'
    Require ($null -ne $archive.GetEntry('backup.json')) 'Missing manifest'
    $entry=$archive.GetEntry('saves/SacredStonesRecomp.sav')
    Require ($entry.Length -eq 32768) 'Incorrect battery save'
    $reader=[IO.BinaryReader]::new($archive.GetEntry('roms/sacred_stones_usa.state1').Open())
    try { Require (([Convert]::ToBase64String($reader.ReadBytes(5))) -eq 'AQIDBAU=') 'State bytes changed' } finally { $reader.Dispose() }
} finally { $archive.Dispose() }
foreach ($file in $before.Keys) {
    Require ((Get-FileHash -LiteralPath $file).Hash -eq $before[$file][0]) 'Original file changed'
    Require ((Get-Item -LiteralPath $file).LastWriteTimeUtc -eq $before[$file][1]) 'Original timestamp changed'
}
& $script -SavePath $save -RomPath $rom -Destination $zip
$originalHash=(Get-FileHash -LiteralPath $zip).Hash
[IO.File]::WriteAllBytes($state,[byte[]]::new(0))
Reject { & $script -SavePath $save -RomPath $rom -Destination $zip }
Require ((Get-FileHash -LiteralPath $zip).Hash -eq $originalHash) 'Failed export replaced good backup'
Reject { & $script -SavePath $save -RomPath $rom -Destination $save }
Reject { & $script -SavePath $save -RomPath $rom -Destination $rom }
[IO.File]::WriteAllBytes($state,[byte[]]@(1,2,3))
$lock=[IO.File]::Open($zip,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::Read)
try { Reject { & $script -SavePath $save -RomPath $rom -Destination $zip } } finally { $lock.Dispose() }
Require ((Get-FileHash -LiteralPath $zip).Hash -eq $originalHash) 'Locked backup changed'
$state9=[IO.Path]::ChangeExtension($rom,'.state9')
$suspend=[IO.Path]::ChangeExtension($rom,'.suspend.state')
[IO.File]::WriteAllBytes($state9,[byte[]]@(9,8,7))
[IO.File]::WriteAllBytes($suspend,[byte[]]@(6,5,4))
[IO.File]::Move($save,"$save.original")
try {
    & $script -SavePath $save -RomPath $rom -Destination $zip
    $archive=[IO.Compression.ZipFile]::OpenRead($zip)
    try {
        Require ($archive.Entries.Count -eq 4) 'Incorrect state-only export entries'
        Require ($null -eq $archive.GetEntry('saves/SacredStonesRecomp.sav')) 'Absent battery included'
        Require ($null -ne $archive.GetEntry('roms/sacred_stones_usa.state9')) 'Slot 9 excluded'
        Require ($null -ne $archive.GetEntry('roms/sacred_stones_usa.suspend.state')) 'Suspend excluded'
    } finally { $archive.Dispose() }
} finally { [IO.File]::Move("$save.original",$save) }
Require (@(Get-ChildItem -LiteralPath $root -Filter '*.tmp.*').Count -eq 0) 'Temporary archives left behind'
Write-Host 'PASS: ZIP content, original save safety, overwrite, invalid data and locked destination'
