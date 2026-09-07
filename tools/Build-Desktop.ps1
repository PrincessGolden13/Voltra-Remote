param([ValidateRange(1, 8)][int]$Jobs = 2)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Push-Location $projectRoot
try {
    $cmakeExe = Join-Path $projectRoot '.venv\Lib\site-packages\cmake\data\bin\cmake.exe'
    $ninjaExe = Join-Path $projectRoot '.venv\Scripts\ninja.exe'
    $zigExe = Join-Path $projectRoot '.venv\Lib\site-packages\ziglang\zig.exe'
    foreach ($required in @($cmakeExe, $ninjaExe, $zigExe)) {
        if (!(Test-Path -LiteralPath $required)) { throw "Missing build dependency: $required" }
    }
    $env:ZIG_GLOBAL_CACHE_DIR = Join-Path $projectRoot '.tools\zig-cache'
    $env:ZIG_LOCAL_CACHE_DIR = Join-Path $projectRoot 'build\zig-cache'
    & $cmakeExe -S . -B build/native -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninjaExe" `
        "-DCMAKE_C_COMPILER=$zigExe" '-DCMAKE_C_COMPILER_ARG1=cc' `
        "-DCMAKE_CXX_COMPILER=$zigExe" '-DCMAKE_CXX_COMPILER_ARG1=c++' `
        '-DCMAKE_C_FLAGS=--target=x86_64-windows-gnu' `
        '-DCMAKE_CXX_FLAGS=--target=x86_64-windows-gnu' '-DCMAKE_BUILD_TYPE=Debug'
    if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed' }
    & $cmakeExe --build build/native --parallel $Jobs
    if ($LASTEXITCODE -ne 0) { throw 'Native build failed' }
} finally { Pop-Location }
