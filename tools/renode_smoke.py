#!/usr/bin/env python3
"""Дымовой тест зонда в Renode, без платы.

    python3 tools/renode_smoke.py --elf build/f411ce-debug/stm32_skills_probe.elf --board F411CE

Проверяет: стартовый лог в USART1 (плата, ядро, версия из ELF, адреса блоков), непрерывную нумерацию
кадров, команду ping по UART (ответ pong), рост probe_counter между двумя чтениями памяти и
константу probe_state. Это доказательство работы в эмуляторе, не на плате.
Код возврата: 0 — PASS, 1 — FAIL, 2 — ошибка запуска.
"""
import argparse
import json
import os
import pathlib
import re
import shutil
import struct
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
MODELS = {"F411CE": "stm32f4_probe.repl", "F401CC": "stm32f4_probe.repl", "F103C8": "stm32f103_probe.repl"}
CORES = {"F411CE": "Cortex-M4", "F401CC": "Cortex-M4", "F103C8": "Cortex-M3"}
CLOCKS = {"F411CE": 16000000, "F401CC": 16000000, "F103C8": 8000000}  # HSI: HSE в модели не запускается


def renode_path(path):
    return "@" + pathlib.Path(path).resolve().as_posix().replace(" ", "\\ ")


def find_renode(explicit):
    for candidate in (explicit, os.environ.get("RENODE_BINARY"), "renode"):
        if candidate and shutil.which(candidate):
            return shutil.which(candidate)
    raise SystemExit("Renode not found: --renode, RENODE_BINARY or PATH")


def elf_version(elf, objcopy):
    """Версия из секции .fw_version: (major.minor.patch, build, revision) или None."""
    out = elf.with_suffix(".fw_version.bin")
    result = subprocess.run([objcopy, "-O", "binary", "-j", ".fw_version", str(elf), str(out)], capture_output=True)
    if result.returncode != 0 or not out.is_file() or out.stat().st_size < 8:
        return None
    major, minor, patch, _reserved, build, revision = struct.unpack("<BBBBHH", out.read_bytes()[:8])
    out.unlink()
    return "%d.%d.%d" % (major, minor, patch), build, revision


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--elf", type=pathlib.Path, required=True)
    parser.add_argument("--board", choices=sorted(MODELS), required=True)
    parser.add_argument("--renode")
    parser.add_argument("--objcopy", default="arm-none-eabi-objcopy")
    parser.add_argument("--output", type=pathlib.Path, help="каталог результатов (по умолчанию рядом с ELF)")
    args = parser.parse_args(argv)

    elf = args.elf.resolve()
    output = (args.output or elf.parent / "renode-smoke").resolve()
    if output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True)
    uart_log = output / "uart.log"
    command = "ping\n"

    lines = ["logLevel 3", 'mach create "probe"',
             "machine LoadPlatformDescription " + renode_path(ROOT / "emu" / "renode" / MODELS[args.board]),
             "sysbus.usart1 CreateFileBackend %s true" % renode_path(uart_log),
             "sysbus LoadELF " + renode_path(elf),
             'emulation RunFor "0.5"',
             'echo "A="', 'sysbus ReadDoubleWord `sysbus GetSymbolAddress "probe_counter"`']
    lines += ["sysbus.usart1 WriteChar %d" % ord(char) for char in command]
    lines += ['emulation RunFor "0.5"',
              'echo "B="', 'sysbus ReadDoubleWord `sysbus GetSymbolAddress "probe_counter"`',
              'echo "S="', 'sysbus ReadDoubleWord `sysbus GetSymbolAddress "probe_state"`', "quit", ""]
    script = output / "smoke.resc"
    script.write_text("\n".join(lines), encoding="utf-8")

    try:
        result = subprocess.run([find_renode(args.renode), "--disable-gui", "--console", "--plain",
                                 "-e", "include " + renode_path(script)],
                                capture_output=True, text=True, errors="replace", timeout=180)
    except (OSError, subprocess.TimeoutExpired) as error:
        print("ERROR: %s" % error)
        return 2
    (output / "renode.log").write_text(result.stdout + result.stderr, encoding="utf-8")
    values = dict(re.findall(r"^([ABS])=\s*\n(0x[0-9A-Fa-f]+)", result.stdout, re.M))
    log = uart_log.read_text(encoding="utf-8", errors="replace") if uart_log.is_file() else ""
    text = [re.sub(r"^\[\s*\d+\.\d+\]\s?", "", line) for line in log.splitlines() if line.strip()]

    checks = []

    def check(name, ok, details=""):
        checks.append({"check": name, "ok": bool(ok), "details": details})
        print("%-4s %s %s" % ("OK" if ok else "FAIL", name, details))

    version = elf_version(elf, args.objcopy)
    check("boot log starts and ends", text[:1] == ["boot: start"] and "boot: done" in text, "%d lines" % len(text))
    check("board name", "board: %s" % args.board in text)
    core = next((line for line in text if line.startswith("CPU: ")), "")
    check("core", core.startswith("CPU: %s (CPUID 0x410FC2" % CORES[args.board]), core)
    check("clock", "clock: HSI %d Hz (HSE timeout)" % CLOCKS[args.board] in text)
    if version:
        check("firmware version from ELF", "Firmware: v%s build %d.%d" % version in text)
    else:
        check("firmware version from ELF", False, "no .fw_version section")
    frames = [int(m.group(1)) for line in text for m in [re.match(r"frame (\d+) ", line)] if m]
    check("frames numbered without gaps", frames and frames == list(range(1, len(frames) + 1)), "%d frames" % len(frames))
    check("command over UART", "pong 1" in text)
    a, b = int(values.get("A", "0"), 16), int(values.get("B", "0"), 16)
    check("probe_counter grows", 0 < a < b, "%d -> %d" % (a, b))
    check("probe_state", values.get("S") and int(values["S"], 16) == 0xA5, values.get("S", "missing"))

    passed = all(item["ok"] for item in checks)
    report = {"schema": 1, "board": args.board, "elf": str(elf), "environment": "emulator", "emulator": "renode",
              "status": "PASS" if passed else "FAIL", "checks": checks}
    (output / "result.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("%s: %s" % (report["status"], output))
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
