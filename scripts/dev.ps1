# Команди розробки для Windows (там немає make). Запуск з кореня проєкту:
#   powershell -ExecutionPolicy Bypass -File scripts/dev.ps1 build
# Завдання: image | build | test | coverage | check | all
param([Parameter(Position = 0)][string]$Task = "build")

$root = Split-Path -Parent $PSScriptRoot
$img = "stm32-build"

function Run-In-Docker([string]$cmd) {
    & docker run --rm -v "${root}:/workspace" -w /workspace $img sh -c $cmd
    if ($LASTEXITCODE -ne 0) { Write-Host "FAILED: $cmd" -ForegroundColor Red; exit 1 }
}

switch ($Task) {
    "image"    { & docker build -t $img "$root/docker"; if ($LASTEXITCODE -ne 0) { exit 1 } }
    "build"    { Run-In-Docker "make -C firmware all" }
    "test"     { Run-In-Docker "make -C tests test" }
    "coverage" { Run-In-Docker "make -C tests coverage" }
    "check"    { Run-In-Docker "python3 scripts/check_deps.py" }
    "all"      { Run-In-Docker "python3 scripts/check_deps.py && make -C tests test && make -C firmware all" }
    default    { Write-Host "Невідоме завдання: $Task (image|build|test|coverage|check|all)"; exit 1 }
}
