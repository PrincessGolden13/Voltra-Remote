param([string]$Port = 'COM3')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Push-Location $projectRoot
try {
    $backup = Join-Path $projectRoot 'build/hardware-backups/guition-factory-first-2MB.bin'
    $metadata = Get-Content 'build/hardware-backups/guition-factory-first-2MB.json' -Raw | ConvertFrom-Json
    if ((Get-Item $backup).Length -ne 2097152 -or (Get-FileHash $backup -Algorithm SHA256).Hash -ine $metadata.sha256) {
        throw 'Verified recovery image missing or altered'
    }
    $secondaryBackup = Join-Path $projectRoot 'build/hardware-backups/guition-factory-simulator-region.bin'
    $secondary = Get-Content 'build/hardware-backups/guition-factory-simulator-region.json' -Raw | ConvertFrom-Json
    if ($secondary.offset -ne 2162688 -or $secondary.length -ne 1900544 -or
        (Get-Item $secondaryBackup).Length -ne 1900544 -or
        (Get-FileHash $secondaryBackup -Algorithm SHA256).Hash -ine $secondary.sha256) {
        throw 'Verified simulator-region recovery image missing or altered'
    }
    $build = Join-Path $projectRoot '.pio/build/guition_k5_normal'
    $simulator = Join-Path $projectRoot '.pio/build/guition_k5_offline'
    $bootApp = Join-Path $projectRoot '.tools/platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin'
    foreach ($image in @(@("$build/bootloader.bin",32768),@("$build/partitions.bin",4096),@($bootApp,8192),@("$build/firmware.bin",1900544),@("$simulator/firmware.bin",1900544))) {
        $file = Get-Item -LiteralPath $image[0]
        if ($file.Length -le 0 -or $file.Length -gt $image[1]) { throw 'Image exceeds backed-up partition boundary' }
    }
    # Verify the actual generated table, not just the source CSV.
    & ./.venv/Scripts/python.exe -c "import pathlib,struct,sys; b=pathlib.Path(sys.argv[1]).read_bytes(); assert b==pathlib.Path(sys.argv[2]).read_bytes(), 'Build partition maps differ'; entries=[struct.unpack('<HBBII16sI',b[i:i+32]) for i in range(0,len(b)-31,32) if b[i:i+2]==b'\xaa\x50']; expected=[(1,2,0x9000,0x5000,b'nvs'),(1,0,0xe000,0x2000,b'otadata'),(0,0x10,0x10000,0x1d0000,b'normal'),(1,3,0x1e0000,0x20000,b'coredump'),(0,0x11,0x210000,0x1d0000,b'simulator')]; assert [(e[1],e[2],e[3],e[4],e[5].rstrip(bytes([0]))) for e in entries]==expected, 'Partition map differs from verified recovery regions'" "$build/partitions.bin" "$simulator/partitions.bin"
    if ($LASTEXITCODE -ne 0) { throw 'Unsafe partition map for this recovery image' }
    & ./.venv/Scripts/python.exe .tools/platformio/packages/tool-esptoolpy/esptool.py --chip esp32s3 --port $Port --baud 115200 write_flash --flash_mode keep --flash_freq 80m --flash_size 16MB 0x0 "$build/bootloader.bin" 0x8000 "$build/partitions.bin" 0xe000 $bootApp 0x10000 "$build/firmware.bin" 0x210000 "$simulator/firmware.bin"
    if ($LASTEXITCODE -ne 0) { throw 'Upload failed; recovery image retained' }
} finally { Pop-Location }
