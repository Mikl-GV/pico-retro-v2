# Сборка pico-retro. Запускать из корня репозитория:  .\build.ps1
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

$env:PICO_SDK_PATH = "C:\pico-sdk"
$env:Path = @(
    "$env:USERPROFILE\scoop\shims",
    "$env:USERPROFILE\scoop\apps\gcc-arm-none-eabi\current\bin",
    "$env:USERPROFILE\scoop\apps\mingw\current\bin",
    "$env:USERPROFILE\scoop\apps\cmake\current\bin",
    "$env:USERPROFILE\scoop\apps\ninja\current\bin",
    $env:Path
) -join ";"

if (-not (Test-Path "$env:PICO_SDK_PATH\pico_sdk_init.cmake")) {
    throw "Pico SDK не найден в $env:PICO_SDK_PATH"
}

# GNU ld на Windows не открывает .ld-скрипты, если в пути есть кириллица.
# Собираем через ASCII-junction, если исходники лежат в D:\Программы\...
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
