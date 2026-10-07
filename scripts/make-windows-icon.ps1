param(
    [string]$Source = "$PSScriptRoot/../android/app/src/main/res/mipmap-nodpi/ic_launcher.png",
    [string]$Destination = "$PSScriptRoot/../assets/game.ico"
)
$ErrorActionPreference = 'Stop'
$png = [IO.File]::ReadAllBytes([IO.Path]::GetFullPath($Source))
if ($png.Length -lt 24 -or [BitConverter]::ToString($png,0,8) -ne '89-50-4E-47-0D-0A-1A-0A' -or
    [BitConverter]::ToString($png,12,12) -ne '49-48-44-52-00-00-01-00-00-00-01-00') {
    throw 'Expected the original 256x256 Android PNG icon.'
}
# ICO can wrap the original PNG verbatim; no artwork resampling or changes.
$memory = [IO.MemoryStream]::new()
$writer = [IO.BinaryWriter]::new($memory)
try {
    $writer.Write([uint16]0); $writer.Write([uint16]1); $writer.Write([uint16]1)
    $writer.Write([byte]0); $writer.Write([byte]0); $writer.Write([byte]0); $writer.Write([byte]0)
    $writer.Write([uint16]1); $writer.Write([uint16]32)
    $writer.Write([uint32]$png.Length); $writer.Write([uint32]22)
    $writer.Write($png)
    $writer.Flush()
    [IO.File]::WriteAllBytes([IO.Path]::GetFullPath($Destination),$memory.ToArray())
} finally { $writer.Dispose(); $memory.Dispose() }
