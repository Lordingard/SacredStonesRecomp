param(
    [string] $MingwBin,
    [ValidateRange(1, 64)][int] $Jobs = 2
)

. "$PSScriptRoot/common.ps1"
$bin = Resolve-Setting $MingwBin "MINGW_BIN" "MingwBin" ""
if (-not $bin) {
    $compiler = Get-Command c++.exe -ErrorAction Stop
    $bin = Split-Path -Parent $compiler.Source
}
$cc = (Join-Path $bin "cc.exe").Replace('\', '/')
$cxx = (Join-Path $bin "c++.exe").Replace('\', '/')
Assert-File -Path $cc -Label "MinGW C compiler"
Assert-File -Path $cxx -Label "MinGW C++ compiler"
$env:PATH = "$bin;$env:PATH"
$cmake = Get-NativeCMake
$framework = Join-Path $script:RepoRoot "extern/gbarecomp"
$build = Join-Path $script:RepoRoot "build/generator-mingw"
& $cmake -S $framework -B $build -G Ninja "-DCMAKE_C_COMPILER=$cc" "-DCMAKE_CXX_COMPILER=$cxx"
if ($LASTEXITCODE -ne 0) { throw "Generator configuration failed." }
& $cmake --build $build --target gba_recompile --parallel $Jobs
if ($LASTEXITCODE -ne 0) { throw "Generator build failed." }
