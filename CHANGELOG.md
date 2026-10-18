# Changelog

Все заметные изменения в этом проекте документируются в этом файле.
Формат основан на [Keep a Changelog](https://keepachangelog.com/ru/1.1.0/).

## [Unreleased]

### Изменено

- Раскладка `hil/` переведена на образец stm32-hwtest-bluepill: `hil/common/tests/` (сценарии, требования,
  контракты), `hil/<ПЛАТА>/` (`session.toml`, `target.toml`, `board.toml`, локальный `stands/local.toml`),
  `hil/templates/` (примеры стендов OpenOCD, ST-LINK GDB Server, J-Link), `hil/README.md`.

### Добавлено

- Опция `PROBE_EXIT_MS` (вместе с `PROBE_LOG_SEMIHOSTING`): семихостинг `SYS_EXIT` через заданное время, чтобы
  прогоны в QEMU и Renode заканчивались сами — так зонд запускается `run.py` навыка stm32-renode-emulator.
- Версия 0.2.0.

## [0.1.0]

### Добавлено

- Прошивка-зонд для STM32F411CE, STM32F401CC (WeAct BlackPill) и STM32F103C8 (Blue Pill): CMake и CMSIS без HAL,
  тактирование от кварца без PLL с откатом на HSI, SysTick 1 кГц.
- Журнал с префиксом времени в UART, RTT, ITM и (по выбору) семихостинг; нумерованные кадры; команды по UART
  и каналу RTT вниз.
- Отладочные выводы PB12–PB15 и PB0, калибровочный ШИМ на PA8, участок известной длины с замером DWT.
- Минимальный RTT с раскладкой SEGGER (`_SEGGER_RTT`), версия сборки в секции `.fw_version`.
- Модели Renode для F4 и F103, дымовой тест `tools/renode_smoke.py`, цели `emulate` и `emulate-debug`,
  ожидания к стартовому логу для навыка stm32-renode-emulator (`emu/expected/`).
- HIL-сценарии stm32-gdbtest v0.4.1 (подмодуль): запуск, профиль платы, кадры, вызов через указатель,
  возвращаемое значение, участок известной длины, блок RTT.
- CI: сборка всех плат, Renode, HIL без платы, проверка форматирования.
