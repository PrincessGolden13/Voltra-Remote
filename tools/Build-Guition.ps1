param([ValidateRange(1,8)][int]$Jobs = 2)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
# Windows Xtensa commands exceed CreateProcess limits with the full project path.
if (Test-Path V:\) { throw 'V: is in use. Free this temporary build alias first.' }
$previousCore = $env:PLATFORMIO_CORE_DIR
subst V: $projectRoot
if ($LASTEXITCODE -ne 0) { throw 'Could not create build alias' }
try {
    $env:PLATFORMIO_CORE_DIR = 'V:\.tools\platformio'
    Push-Location V:\
    try {
        & V:\.venv\Scripts\python.exe -m platformio run -e guition_k5_offline -j $Jobs
        if ($LASTEXITCODE -ne 0) { throw 'Guition offline build failed' }
    } finally { Pop-Location }
} finally {
    $env:PLATFORMIO_CORE_DIR = $previousCore
    subst V: /D
}
