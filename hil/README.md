# HIL-сценарии зонда

Сценарии [stm32-gdbtest](https://github.com/ViacheslavMezentsev/stm32-gdbtest) проверяют работающий зонд
на плате: runner запускает GDB-сервер отладчика, GDB сбрасывает MCU, останавливается и выполняет сценарий на
Python. Тестового кода в прошивке нет. Раскладка — как в
[stm32-hwtest-bluepill](https://github.com/ViacheslavMezentsev/stm32-hwtest-bluepill/tree/main/hil).

## Состав

```text
hil/
  common/tests/       board/test_*.py, requirements.md, contracts.json
  F411CE/             session.toml, target.toml, board.toml, stands/
  F401CC/             session.toml, target.toml, board.toml, stands/
  F103C8/             session.toml, target.toml, board.toml, stands/
  templates/          *.example.toml
  api.toml
```

Плату выбирает сборка: пресеты `<плата>-hil` задают `BOARD` и `PROBE_HIL=ON`, `cmake/hil.cmake` подключает
stm32-gdbtest v0.4.1 (`modules/stm32-gdbtest`) с конфигурацией прогона `hil/<ПЛАТА>/session.toml` и общими
сценариями `hil/common/tests`. Конфигурация связывает описание MCU (`target.toml`), общий `api.toml` и файл
данных платы (`board.toml`); сценарии читают их через `t.profile`, поэтому один сценарий обслуживает все
платы. Новая плата — новый каталог `hil/<ПЛАТА>/` с тремя файлами.

## Стенд

Стенд — локальный файл с отладчиком платы (не коммитится). Скопируйте пример и укажите серийный номер:

| Плата | Файл стенда | Пресеты |
| --- | --- | --- |
| BlackPill F411CE | `hil/F411CE/stands/local.toml` | `f411ce-hil`, `f411ce-hil-host`, `f411ce-hil-hw` |
| BlackPill F401CC | `hil/F401CC/stands/local.toml` | `f401cc-hil`, `f401cc-hil-host`, `f401cc-hil-hw` |
| Blue Pill F103C8 | `hil/F103C8/stands/local.toml` | `f103c8-hil`, `f103c8-hil-host`, `f103c8-hil-hw` |

| Отладчик | Пример | Серийный номер |
| --- | --- | --- |
| ST-Link, OpenOCD | `openocd.example.toml` | `openocd` или `STM32_Programmer_CLI -l st` |
| ST-Link, ST-LINK GDB Server (STM32CubeCLT) | `stlink.example.toml` | `STM32_Programmer_CLI -l st` |
| J-Link | `jlink.example.toml` | десятичный номер из J-Link Commander |

Проверка окружения без обращения к плате:

```sh
python -B modules/stm32-gdbtest/stm32_gdbtest/cli.py doctor --stand hil/F411CE/stands/local.toml
```

## Запуск

```sh
cmake --preset f411ce-hil
cmake --build --preset f411ce-hil
ctest --preset f411ce-hil-host     # без платы: трассировка требований и prepare.*
ctest --preset f411ce-hil-hw       # на плате: все hw.*
```

Тестовые пресеты задают политику идентичности `strict`: DEV_ID кристалла должен совпасть с описанием MCU.
Прошивка записывается, только если образ во Flash отличается (`flash = "if-different"` в стенде).

| Сценарий | Что проверяет |
| --- | --- |
| `HW_BOOT` | нет отказа, `probe_state`, частота ядра — HSE или HSI из файла данных, флаги телеметрии |
| `HW_BOARD_PROFILE` | MCU, файл данных и сборка — одна плата; DEV_ID, таблица векторов, отказ чтения USART1 как памяти |
| `HW_FRAMES` | соседние кадры: номера N и N+1, интервал — период из `api.toml` |
| `HW_SAMPLE_HOOK` | указатель в RAM ведёт в `probe_sample()`, счётчик вызовов растёт |
| `HW_COMPUTE_RETURN` | возвращаемое значение `probe_compute()` через `finish` |
| `HW_WORK` | участок известной длины: итерации из `api.toml`, такты DWT |
| `HW_RTT_BLOCK` | блок `_SEGGER_RTT`: строка, буферы, стартовый лог в канале вверх |

Требования — `common/tests/requirements.md`. Стиль сценариев проверяется тестом модуля:
`python modules/stm32-gdbtest/tests/host/test_scenario_style.py hil/common/tests/board/*.py`.
