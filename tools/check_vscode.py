#!/usr/bin/env python3
"""Проверка .vscode без VS Code: задачи, конфигурации отладки и настройки ссылаются на то, что есть.

Ищет: зависимости и preLaunchTask на несуществующие задачи, пресеты и цели CMake, которых нет,
пути к скриптам, сценарии HIL, переменные ${config:...} и ${input:...} без определения.
    python tools/check_vscode.py
"""
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
VSCODE = ROOT / ".vscode"


def load(name):
    """JSON с комментариями // (строки целиком) — как в settings.json."""
    text = (VSCODE / name).read_text(encoding="utf-8")
    text = "\n".join("" if line.strip().startswith("//") else line for line in text.splitlines())
    return json.loads(text)


def main():
    errors = []
    settings, tasks, launch = load("settings.json"), load("tasks.json"), load("launch.json")
    presets = json.loads((ROOT / "CMakePresets.json").read_text(encoding="utf-8"))
    names = {kind: {p["name"] for p in presets.get(kind, [])} for kind in ("configurePresets", "buildPresets", "testPresets")}
    targets = set(re.findall(r"add_custom_target\((\S+)", "".join(
        path.read_text(encoding="utf-8") for path in [ROOT / "CMakeLists.txt"] + sorted((ROOT / "cmake").glob("*.cmake")))))
    labels = [task["label"] for task in tasks["tasks"]]
    inputs = {item["id"]: item for item in tasks.get("inputs", [])}
    if len(labels) != len(set(labels)):
        errors.append("повторяющиеся метки задач")

    def expand(value):
        def config(match):
            key = match.group(1)
            if key not in settings:
                errors.append("нет настройки %s" % key)
                return ""
            return expand(str(settings[key]))
        value = re.sub(r"\$\{config:([^}]+)\}", config, value)
        return value.replace("${workspaceFolder}/", "").replace("${workspaceFolder}", ".")

    for document in (tasks["tasks"], launch["configurations"]):
        text = json.dumps(document, ensure_ascii=False)
        for key in re.findall(r"\$\{config:([^}]+)\}", text):
            if key not in settings:
                errors.append("нет настройки %s" % key)
        for key in re.findall(r"\$\{input:([^}]+)\}", text):
            if key not in inputs:
                errors.append("нет input %s" % key)

    for task in tasks["tasks"]:
        label, args = task["label"], task.get("args", [])
        depends = task.get("dependsOn")
        for dependency in ([depends] if isinstance(depends, str) else depends or []):
            if dependency not in labels:
                errors.append("%s: нет задачи %s" % (label, dependency))
        for index, arg in enumerate(args[:-1]):
            value = args[index + 1]
            if arg == "--preset":
                kind = {"cmake": "buildPresets" if "--build" in args else "configurePresets", "ctest": "testPresets"}[task["command"]]
                for board in inputs.get("board", {}).get("options", [""]) if "${input:board}" in value else [""]:
                    name = value.replace("${input:board}", board)
                    if name not in names[kind]:
                        errors.append("%s: нет пресета %s (%s)" % (label, name, kind))
            if arg == "--target" and task["command"] == "cmake" and value not in targets:
                errors.append("%s: нет цели CMake %s" % (label, value))
        for arg in args:
            if arg.endswith(".py") and "${command:" not in arg:
                path = ROOT / expand(arg)
                if not path.is_file():
                    errors.append("%s: нет файла %s" % (label, path.relative_to(ROOT)))
        for index, arg in enumerate(args[:-1]):
            if arg == "--fields" and not (ROOT / expand(args[index + 1])).is_file():
                errors.append("%s: нет файла полей %s" % (label, args[index + 1]))

    for configuration in launch["configurations"]:
        task = configuration.get("preLaunchTask")
        if task and task not in labels:
            errors.append("%s: нет задачи %s" % (configuration["name"], task))

    hil_text = "".join(path.read_text(encoding="utf-8") for path in (ROOT / "hil").rglob("*.md"))
    for test in inputs.get("test", {}).get("options", []):
        if test not in hil_text:
            errors.append("сценарий %s не описан в hil/" % test)

    for error in errors:
        print("ОШИБКА: " + error)
    print("%s: задач %d, конфигураций отладки %d, ошибок %d" % (
        VSCODE.relative_to(ROOT), len(labels), len(launch["configurations"]), len(errors)))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
