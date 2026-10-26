#!/usr/bin/env python3
"""Підключає App до згенерованого CubeMX файлу firmware/Core/Src/main.c.

Змінюються лише три місця (усі всередині блоків USER CODE, які CubeMX не затирає):
  1. #include "platform.h" та "app.h"
  2. app_init();  (всередині викликає platform_init) після привітання в USER CODE 2
  3. тіло нескінченного циклу: замість миготіння LED і tick - app_step();
Скрипт безпечно запускати повторно: вже внесені зміни він пропускає.
"""
import pathlib
import sys

path = pathlib.Path(__file__).resolve().parent.parent / "firmware" / "Core" / "Src" / "main.c"
raw = path.read_bytes().decode("utf-8")
crlf = "\r\n" in raw
text = raw.replace("\r\n", "\n")

edits = [
    ("#include <stdio.h>\n",
     '#include <stdio.h>\n#include "platform.h"\n#include "app.h"\n',
     '#include "app.h"'),
    ('  printf("STM32C031C6 ready\\r\\n");\n',
     '  printf("STM32C031C6 ready\\r\\n");\n  (void)app_init();\n',
     "app_init();"),
    ('    HAL_GPIO_TogglePin(Led_GPIO_Port, Led_Pin);\n'
     '    printf("tick %lu\\r\\n", (unsigned long)(HAL_GetTick() / 1000U));\n'
     '    HAL_Delay(1000);\n',
     "    app_step();\n",
     "app_step();"),
]

for old, new, marker in edits:
    if marker in text:
        print("пропущено (вже внесено):", marker)
        continue
    if text.count(old) != 1:
        print("ПОМИЛКА: не знайдено очікуваний фрагмент у main.c:\n" + old)
        sys.exit(1)
    text = text.replace(old, new)

if crlf:
    text = text.replace("\n", "\r\n")
path.write_bytes(text.encode("utf-8"))
print("main.c оновлено:", path)
