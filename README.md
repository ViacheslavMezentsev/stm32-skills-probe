# stm32-skills-probe

Нейтральная прошивка-зонд для исследований и проверки навыков
[demo-stm32-skills](https://github.com/ViacheslavMezentsev/demo-stm32-skills) и сценариев
[stm32-gdbtest](https://github.com/ViacheslavMezentsev/stm32-gdbtest). Зонд ведёт себя заранее известным
образом — журнал, нумерованные кадры, переменные разных типов, отладочные выводы, участок известной длины,
блок RTT — поэтому замеры разных инструментов можно сравнивать между собой, повторять и публиковать.

Структура проекта повторяет [stm32-hwtest-blackpill](https://github.com/ViacheslavMezentsev/stm32-hwtest-blackpill)
и [stm32-hwtest-bluepill](https://github.com/ViacheslavMezentsev/stm32-hwtest-bluepill): CMake и CMSIS без HAL,
HIL-сценарии stm32-gdbtest в `hil/`, модели Renode в `emu/`.

## Платы

| `BOARD` | Плата | MCU | Flash/RAM | Кварц (HSE) | Без кварца (HSI) |
| --- | --- | --- | --- | --- | --- |
| `F411CE` | WeAct BlackPill | STM32F411CEU6, Cortex-M4 | 512/128 КиБ | 25 МГц | 16 МГц |
| `F401CC` | WeAct BlackPill | STM32F401CCU6, Cortex-M4 | 256/64 КиБ | 25 МГц | 16 МГц |
| `F103C8` | Blue Pill | STM32F103C8T6, Cortex-M3 | 64/20 КиБ | 8 МГц | 8 МГц |

Тактирование — от кварца без PLL: частота точная и одинаковая между запусками. Если кварц не запустился
(в эмуляторе бит готовности не выставляется), зонд работает от HSI и пишет об этом в журнал.

## Выводы

| Вывод | Назначение | Для чего |
| --- | --- | --- |
| PB12 | высокий уровень внутри `SysTick_Handler` (1 кГц) | джиттер ISR, влияние отладчика на время |
| PB13 | высокий уровень на участке известной длины | калибровка DWT против анализатора |
| PB14 | «гейт»: поднимается командой `gate` | поиск начала обмена анализатором |
| PB15 | переключается на каждом кадре | частота и потери кадров |
| PB0 | переключается в `probe_sample()` (1 кГц) | искажение времени перехватами RTT |
| PA8 | ШИМ TIM1_CH1, по умолчанию 1 кГц, 25 % | калибровка анализатора |
| PA9, PA10 | USART1 TX и RX, 115200 8N1 | журнал и команды |
| PC13 | светодиод, мигает раз в секунду | признак работы |
| PA13, PA14, PB3 | SWDIO, SWCLK, SWO | отладчик и трассировка; зонд их не занимает |

## Журнал

Строки с префиксом времени `[   1.234] ` (миллисекунды SysTick) идут одновременно в несколько каналов:
UART и RTT — всегда, ITM (порт 0) — если отладчик включил ITM, семихостинг — при сборке с
`-DPROBE_LOG_SEMIHOSTING=ON` (без отладчика остановил бы ядро). Каналы меняет команда `sinks`.
С `-DPROBE_EXIT_MS=<мс>` (только вместе с семихостингом) зонд через заданное время пишет `exit: <мс> ms` и
завершается вызовом `SYS_EXIT`: QEMU останавливается сам, Renode останавливает ядро. Так зонд запускает
`scripts/run.py` навыка stm32-renode-emulator.

```text
[   0.000] boot: start
[   0.000] board: F411CE
[   0.000] CPU: Cortex-M4 (CPUID 0x410FC241)
[   0.000] clock: HSE 25000000 Hz
[   0.000] Firmware: v0.1.0 build 0.0
[   0.000] pins: isr=PB12 work=PB13 gate=PB14 frame=PB15 sample=PB0 pwm=PA8 tx=PA9 rx=PA10
[   0.000] rtt: _SEGGER_RTT at 0x200000D0 up=1024 down=64
[   0.000] telemetry: probe_telemetry at 0x20000030, probe_counter at 0x2000002C
[   0.000] boot: done
[   0.100] frame 1 tick=100 counter=148839 sine=290 samples=100
```

Кадры нумеруются подряд: пропуск номера — потеря в канале. Ожидания к стартовому логу для
`check_startup_log.py` навыка stm32-renode-emulator — `emu/expected/<плата>.toml`.

## Команды

Строки по UART или каналу RTT вниз, завершаются `\n` или `\r`:

| Команда | Действие |
| --- | --- |
| `ping` | ответ `pong <n>` |
| `status` | номер кадра, расписание, каналы, отброшенные сообщения RTT |
| `rate <мс>` | период кадров (0 — выключить), по умолчанию 100 |
| `burst <n>` | n кадров подряд |
| `gate <n>` | поднять PB14, n кадров, опустить PB14 |
| `work <итерации>` | участок известной длины сейчас; ответ с тактами DWT |
| `period <мс>` | период участка известной длины (0 — выключить), по умолчанию 1000 |
| `sinks <маска>` | каналы журнала: 1 UART, 2 RTT, 4 ITM, 8 семихостинг |
| `pwm <Гц> <промилле>` | ШИМ на PA8 |

## Символы для инструментов

| Символ | Что это | Кто использует |
| --- | --- | --- |
| `probe_telemetry` | структура: номер кадра, тик, синус ×1000, `float`, счётчики, флаги, такты DWT | stm32-ros-alt-channel (маршрут A, поля по DWARF) |
| `probe_counter`, `probe_state` | итерации главного цикла и константа 0xA5 | чтение памяти отладчиком и эмулятором |
| `probe_sample_hook` | указатель в RAM на `probe_sample()`, вызывается раз в мс | вариант «подмена указателя» stm32-rtt-console |
| `probe_compute()` | функция с возвращаемым значением | вариант «перехват возврата» stm32-rtt-console |
| `_SEGGER_RTT` | блок RTT, раскладка SEGGER: канал вверх 1024 Б, вниз 64 Б | OpenOCD `rtt setup`, J-Link |
| `.fw_version` | версия и номер сборки | check_startup_log.py |

Верхние 8 КиБ RAM не заняты данными (проверяет компоновщик): там стек и свободное окно, куда
stm32-rtt-console может внедрить свой блок RTT (на F411 — 0x2001E000..0x2001F600). Прерывания
устройства зонд не использует, TIM2–TIM4 свободны — для варианта «свободный таймер».

## Сборка и проверки

Нужны CMake ≥ 3.25, Ninja, xPack GNU Arm 14.2.1-1.1 (`ARM_TOOLCHAIN_ROOT` или PATH), для HIL — Python ≥ 3.11,
для эмуляции — Renode 1.17 (`RENODE_BINARY` или PATH).

```sh
git submodule update --init                     # stm32-gdbtest для HIL
cmake --preset f411ce-debug && cmake --build --preset f411ce-debug
ctest --preset f411ce-debug-emulator            # Renode: лог, кадры, команда ping, чтение RAM
cmake --build --preset f411ce-debug --target emulate-debug   # Renode с GDB-сервером :3334
cmake --preset f411ce-hil && cmake --build --preset f411ce-hil
ctest --preset f411ce-hil-host                  # без платы: traceability и prepare.*
ctest --preset f411ce-hil-hw                    # на плате, нужен hil/stands/f411ce.local.toml
```

Пресеты есть для `f411ce`, `f401cc` и `f103c8`. CI (`.github/workflows/ci.yml`) собирает все платы,
прогоняет зонд в Renode и HIL-проверки без платы. Эмулятор подтверждает только логику и раскладку, не время
и не работу на железе.

## HIL-сценарии stm32-gdbtest

`hil/` устроен как в stm32-hwtest-blackpill: `sessions/` (конфигурация прогона на плату), `profiles/`
(описания MCU), `boards/` (данные платы), `api.toml`, `tests/` (сценарии, требования), `stands/` (примеры).
Сценарии: `HW_BOOT`, `HW_BOARD_PROFILE`, `HW_FRAMES`, `HW_SAMPLE_HOOK`, `HW_COMPUTE_RETURN`, `HW_WORK`,
`HW_RTT_BLOCK` — требования в `hil/tests/requirements.md`. Навыки stm32-gdbtest скопированы в
`.claude/skills/` из подмодуля (обновляются вместе с ним).

## Связь с планом исследований

Зонд — исследование R0 [плана](https://github.com/ViacheslavMezentsev/demo-stm32-skills/blob/main/docs/research-plan.md):

| Исследование | Что даёт зонд |
| --- | --- |
| R1, R3 (RTT, варианты перехвата) | `probe_sample()` и PB0, `probe_sample_hook`, `probe_compute()`, свободная память под блок |
| R2 (штатный RTT) | `_SEGGER_RTT`, нумерованные кадры, команды по каналу вниз, `sinks 2` |
| R5–R6 (SWO, DWT) | ITM-канал журнала, PB3 свободен, участок известной длины на PB13 |
| R8–R9 (ROS) | `probe_telemetry`, `probe_counter`, нумерация кадров для потерь |
| R12–R15 (анализатор) | ШИМ на PA8, PB12–PB15, `gate` |
| R16 (эмулятор против железа) | одна прошивка в Renode, QEMU и на плате, `emu/expected/` |

## Лицензия

[MIT](LICENSE); CMSIS — лицензии в `cmsis/` (см. `cmsis/README.md`).
