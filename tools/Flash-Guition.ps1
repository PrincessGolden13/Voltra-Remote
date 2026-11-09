param([string]$Port = 'COM3', [switch]$ConfirmUnloaded, [switch]$PublicFirmware)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if (!$ConfirmUnloaded) { throw 'Unload all VOLTRAs and stop training first, then pass -ConfirmUnloaded.' }
Push-Location $projectRoot
try {
    & ./.venv/Scripts/python.exe tools/package_release.py
    if ($LASTEXITCODE -ne 0) { throw 'Build public Release and Simulator targets first' }
    $bundle = 'build/release/usb'
    if (!$PublicFirmware) {
        # The owner's local Normal build migrates its working bootstrap into NVS.
        # This private bundle is never a release artifact and must stay ignored.
        & ./.venv/Scripts/python.exe tools/prepare_local_install.py
        if ($LASTEXITCODE -ne 0) { throw 'Build the local Normal target for this version first' }
        $bundle = 'build/local-usb'
    }
    & ./.venv/Scripts/python.exe tools/install_firmware.py --port $Port --bundle $bundle `
        --backup-dir build/hardware-backups --confirm-unloaded `
        --esptool .tools/platformio/packages/tool-esptoolpy/esptool.py
    if ($LASTEXITCODE -ne 0) { throw 'USB installation failed; recovery copy retained' }
} finally { Pop-Location }
