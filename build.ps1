# Сборка pico-retro под Windows. Запускать из корня репозитория: .\build.ps1
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

# Пути инструментов (winget-установки)
$sdk = "C:\Users\PCB\pico-sdk"
$ninjaDir   = "C:\Users\PCB\AppData\Local\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe"
$mingwDir   = "C:\Users\PCB\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin"
$gccArmDir  = "C:\Users\PCB\AppData\Local\Arduino15\packages\STMicroelectronics\tools\xpack-arm-none-eabi-gcc\14.2.1-1.1\bin"

$env:PICO_SDK_PATH = $sdk
$env:Path = @($ninjaDir, $mingwDir, $gccArmDir, $env:Path) -join ";"
$env:CC  = "$mingwDir\gcc.exe"
$env:CXX = "$mingwDir\g++.exe"

if (-not (Test-Path "$env:PICO_SDK_PATH\pico_sdk_init.cmake")) {
    throw "Pico SDK не найден в $env:PICO_SDK_PATH"
}

# GNU ld не открывает .ld-скрипты при кириллице в пути.
# Собираем через ASCII-junction, если исходники лежат в пути с кириллицей.
$src = $root
$asciiRoot = "C:\pico-work\pico-retro"
if ($root -match "[^\x00-\x7F]" -and (Test-Path $asciiRoot)) {
    $src = $asciiRoot
}

Write-Host "SDK: $env:PICO_SDK_PATH"
Write-Host "SRC: $src"
cmake --version | Select-Object -First 1
arm-none-eabi-gcc --version | Select-Object -First 1

cmake -S $src -B "$src\out" -G Ninja -DPICO_SDK_PATH="$env:PICO_SDK_PATH" -DPICO_BOARD=pico
cmake --build "$src\out"

$uf2 = Join-Path $src "out\pico_retro.uf2"
if (Test-Path $uf2) {
    New-Item -ItemType Directory -Force -Path "$root\build" | Out-Null
    Copy-Item $uf2 "$root\build\pico_retro.uf2" -Force
    Write-Host "Готово: $root\build\pico_retro.uf2"
}
