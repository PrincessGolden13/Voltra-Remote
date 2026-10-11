param([Parameter(Mandatory=$true)][string]$Python)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Push-Location $projectRoot
try {
    # Use a real Python 3.12+ interpreter, not a Windows Store alias.
    & $Python -m venv --without-pip .venv
    if ($LASTEXITCODE -ne 0) { throw 'Virtual environment creation failed' }
    & $Python -m pip --python .venv/Scripts/python.exe install -r tools/requirements-dev.txt
    if ($LASTEXITCODE -ne 0) { throw 'Development dependency installation failed' }
    New-Item -ItemType Directory -Force -Path .tools | Out-Null
    $archive = '.tools/lvgl-9.3.0.zip'
    if (!(Test-Path -LiteralPath $archive)) {
        Invoke-WebRequest 'https://github.com/lvgl/lvgl/archive/refs/tags/v9.3.0.zip' -OutFile $archive
    }
    $expected = '7225E7D2B83F89F3D089D3C3492083D447ABB990FDED24AD5BBAAA541E8746AF'
    if ((Get-FileHash $archive -Algorithm SHA256).Hash -ne $expected) {
        throw 'LVGL archive hash differs from the audited archive; investigate before using it'
    }
    if (!(Test-Path -LiteralPath '.tools/lvgl-9.3.0/CMakeLists.txt')) {
        Expand-Archive -LiteralPath $archive -DestinationPath .tools
    }
} finally { Pop-Location }
