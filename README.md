# sysprog2

Лабораторні з системного програмування. STM32C031C6 (NUCLEO-C031C6), симуляція у Wokwi.

Проєкт: Motion-Mirror. MPU-6050 міряє нахил, мікроконтролер рахує кут і повертає серво. Потенціометр задає підсилення, кнопка вмикає HOLD.

## Збірка

Потрібен Docker Desktop.

```
powershell -ExecutionPolicy Bypass -File scripts/dev.ps1 image
powershell -ExecutionPolicy Bypass -File scripts/dev.ps1 all
```

`image` робить образ (один раз), `all` перевіряє залежності, ганяє тести й збирає прошивку. На Linux те саме через `make image` і `make all`.

## Запуск

Після збірки: VS Code, розширення Wokwi, F1, Wokwi: Start Simulator. У терміналі буде `[SELFTEST]` і далі `[LOG]` раз на секунду.

## Папки

- `firmware/`: Core, Drivers, Platform, Devices, App
- `tests/`: тести на хості
- `scripts/`: допоміжні скрипти
- `docs/`: PRD, pinout, architecture, звіт
- `diagram.json`: схема Wokwi

Основа проєкту (HAL, startup, linker) з шаблону wokwi/stm32-hello-wokwi та ST HAL для STM32C0.