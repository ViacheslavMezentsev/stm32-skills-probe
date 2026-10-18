# AGENTS.md

Правила для ИИ-агентов и участников. Общий порядок работы с git — как в
[demo-stm32-skills](https://github.com/ViacheslavMezentsev/demo-stm32-skills/blob/main/AGENTS.md) (ветки
`<агент>/<задача>`, слияние перемоткой `git land`, подписанные Conventional Commits на английском, push и
теги — владелец); команды — в его HOWTO.

## Проект

Прошивка-зонд для исследований навыков demo-stm32-skills и сценариев stm32-gdbtest. Платы: `F411CE`, `F401CC`
(WeAct BlackPill), `F103C8` (Blue Pill). Подробности — [README](README.md).

| Путь | Содержимое |
| --- | --- |
| `src/` | зонд (`probe.cpp`, `commands.cpp`), журнал (`log.c`), RTT (`rtt.c`), платформа (`platform_f1.c`, `platform_f4.c`), запуск |
| `cmsis/` | заголовки CMSIS и ST без изменений; не редактировать и не форматировать |
| `cmake/`, `ld/` | toolchain, выбор платы (`probe.cmake`), эмуляция, HIL, шаблон компоновщика |
| `emu/` | модели Renode и ожидания к стартовому логу |
| `hil/` | HIL stm32-gdbtest по образцу stm32-hwtest-bluepill: `common/tests/`, `<ПЛАТА>/` (`session.toml`, `target.toml`, `board.toml`, `stands/`), `templates/`, `api.toml` |
| `tools/` | дымовой тест в Renode, проверка `.vscode` (`check_vscode.py`), поля записи диагностики |
| `.vscode/` | задачи и конфигурации отладки по образцу stm32-hwtest-bluepill |
| `modules/stm32-gdbtest` | подмодуль, v0.4.1 |
| `.claude/skills/` | копии навыков stm32-gdbtest из подмодуля (обновлять вместе с ним) и stm32-diag-record из demo-stm32-skills (обновлять с его выпусками) |

## Правила

1. До изменений проверьте `git status`; чужие незакоммиченные правки не откатывайте.
2. Поведение зонда — контракт для навыков и исследований: журнал (формат строк), выводы, команды, имена
   символов, раскладка RTT. Меняя его, обновите README, `emu/expected/`, `hil/` и CHANGELOG и поднимите
   версию в `CMakeLists.txt`.
3. Перед коммитом: сборка всех пресетов, `ctest --preset <плата>-debug-emulator`, `ctest --preset <плата>-hil-host`,
   `clang-format --dry-run --Werror src/*.c src/*.cpp src/*.h`,
   `python modules/stm32-gdbtest/tests/host/test_scenario_style.py hil/common/tests/board/*.py`,
   `python tools/check_vscode.py`.
4. Эмулятор не заменяет плату: в отчётах указывайте, что проверено в Renode или QEMU, а что на железе.
5. Не коммитьте серийные номера пробников (`hil/*/stands/local.toml` в `.gitignore`), персональные пути,
   прошивки, журналы прогонов и кэши.
6. README, CHANGELOG и требования HIL ведутся на русском языке.
7. Раскладка `hil/` — как в stm32-hwtest-bluepill (каталог на плату, общие сценарии в `common/`). Она
   предпочтительнее варианта с `profiles/`, `sessions/`, `boards/`, который описывает навык
   stm32-gdbtest-integrate (копия из подмодуля): новые платы добавляются каталогом `hil/<ПЛАТА>/`.
