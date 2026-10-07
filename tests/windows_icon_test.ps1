param([Parameter(Mandatory=$true)][string]$Executable)
$ErrorActionPreference='Stop'
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class IconResources {
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode)]
    public static extern IntPtr LoadLibraryEx(string path, IntPtr file, uint flags);
    [DllImport("kernel32.dll")]
    public static extern IntPtr FindResource(IntPtr module, IntPtr name, IntPtr type);
    [DllImport("kernel32.dll")]
    public static extern IntPtr LoadResource(IntPtr module, IntPtr resource);
    [DllImport("kernel32.dll")]
    public static extern IntPtr LockResource(IntPtr resource);
    [DllImport("kernel32.dll")]
    public static extern uint SizeofResource(IntPtr module, IntPtr resource);
    [DllImport("kernel32.dll")]
    public static extern bool FreeLibrary(IntPtr module);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)]
    public static extern IntPtr LoadImage(IntPtr module, IntPtr name, uint type, int width, int height, uint flags);
    [DllImport("user32.dll")]
    public static extern bool DestroyIcon(IntPtr icon);
}
'@
$module=[IconResources]::LoadLibraryEx([IO.Path]::GetFullPath($Executable),[IntPtr]::Zero,0x22)
if ($module -eq [IntPtr]::Zero) { throw 'Cannot read executable resources.' }
function Read-Resource([int]$id,[int]$type) {
    $resource=[IconResources]::FindResource($module,[IntPtr]$id,[IntPtr]$type)
    if ($resource -eq [IntPtr]::Zero) { throw "Missing icon resource $id/$type" }
    $bytes=[byte[]]::new([IconResources]::SizeofResource($module,$resource))
    $pointer=[IconResources]::LockResource([IconResources]::LoadResource($module,$resource))
    [Runtime.InteropServices.Marshal]::Copy($pointer,$bytes,0,$bytes.Length)
    return ,$bytes
}
try {
    $group=Read-Resource 101 14
    if ($group.Length -ne 20 -or [BitConverter]::ToUInt16($group,4) -ne 1) { throw 'Unexpected icon group.' }
    $png=Read-Resource ([BitConverter]::ToUInt16($group,18)) 3
    $source=[IO.File]::ReadAllBytes([IO.Path]::GetFullPath("$PSScriptRoot/../android/app/src/main/res/mipmap-nodpi/ic_launcher.png"))
    if ([Convert]::ToBase64String($png) -ne [Convert]::ToBase64String($source)) { throw 'Executable icon differs from Android.' }
    foreach ($size in @(16,32,256)) {
        $icon=[IconResources]::LoadImage($module,[IntPtr]101,1,$size,$size,0)
        if ($icon -eq [IntPtr]::Zero) { throw "Windows cannot load the ${size}px icon." }
        [IconResources]::DestroyIcon($icon) | Out-Null
    }
    Write-Host 'PASS: embedded icon matches Android PNG and Windows loads 16/32/256px variants'
} finally { [IconResources]::FreeLibrary($module) | Out-Null }
