#!/usr/bin/env python3
"""Перевірка правил залежностей між шарами прошивки.

Правила (хто що має право підключати через #include):
  firmware/Devices/*  -> лише стандартні заголовки і заголовки Devices
  firmware/App/*      -> стандартні заголовки, Devices та platform.h
  Ні Devices, ні App не підключають HAL/CMSIS/stm32*, main.h та заголовки Platform
  (окрім нейтрального platform.h, у якому немає типів HAL).

Код виходу 1, якщо знайдено порушення (це використовує make check і CI).
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
FW = ROOT / "firmware"

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]')
FORBIDDEN_SYSTEM = re.compile(r"(stm32|hal|cmsis|core_cm|main\.h)", re.IGNORECASE)


def headers_of(layer):
    return {p.name for p in (FW / layer).glob("*.h")}


def allowed_quoted(layer):
    if layer == "Devices":
        return headers_of("Devices")
    if layer == "App":
        return headers_of("App") | headers_of("Devices") | {"platform.h"}
    raise ValueError(layer)


def check_layer(layer):
    problems = []
    allowed = allowed_quoted(layer)
    for path in sorted((FW / layer).rglob("*")):
        if path.suffix not in (".c", ".h"):
            continue
        for lineno, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            m = INCLUDE_RE.match(line)
            if not m:
                continue
            kind, name = m.groups()
            base = name.split("/")[-1]
            rel = path.relative_to(ROOT)
            if FORBIDDEN_SYSTEM.search(base):
                problems.append(f"{rel}:{lineno}: заборонений заголовок '{name}' у шарі {layer}")
            elif kind == '"' and base not in allowed:
                problems.append(f"{rel}:{lineno}: шар {layer} не має права підключати '{name}'")
    return problems


def main():
    problems = []
    for layer in ("Devices", "App"):
        if (FW / layer).is_dir():
            problems += check_layer(layer)
    if problems:
        print("ПОРУШЕННЯ ПРАВИЛ ЗАЛЕЖНОСТЕЙ:")
        for p in problems:
            print("  " + p)
        return 1
    print("check_deps: OK (Devices та App не залежать від HAL/Platform)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
