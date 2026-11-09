param([ValidateRange(1,8)][int]$Jobs = 2, [ValidateSet('All','Normal','Simulator','Release')][string]$Target='All')
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
        $targets = if ($Target -eq 'All') { @('guition_k5_normal','guition_k5_offline') } elseif ($Target -eq 'Normal') { @('guition_k5_normal') } elseif ($Target -eq 'Release') { @('guition_k5_release','guition_k5_offline') } else { @('guition_k5_offline') }
        foreach ($targetName in $targets) {
            & V:\.venv\Scripts\python.exe -m platformio run -e $targetName -j $Jobs
            if ($LASTEXITCODE -ne 0) { throw "Guition build failed: $targetName" }
        }
    } finally { Pop-Location }
} finally {
    $env:PLATFORMIO_CORE_DIR = $previousCore
    subst V: /D
}
