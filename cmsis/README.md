# CMSIS (подмножество)

Заголовки CMSIS и ST взяты без изменений из проектов
[stm32-hwtest-bluepill](https://github.com/ViacheslavMezentsev/stm32-hwtest-bluepill) и
[stm32-hwtest-blackpill](https://github.com/ViacheslavMezentsev/stm32-hwtest-blackpill):

| Каталог | Источник | Устройства |
| --- | --- | --- |
| `f1/` | STM32CubeF1 v1.8.7: CMSIS-Core(M) 5.0.8, CMSIS Device F1 4.3.5 | `STM32F103xB` (F103C8, F103CB) |
| `f4/` | STM32CubeF4 V1.28.3 (хеши — `f4/source.json`) | `STM32F411xE`, `STM32F401xC` |

Ядро у серий разное (Cortex-M3 и Cortex-M4) и взято из разных выпусков, поэтому каталоги не
объединяются. Лицензии: Apache-2.0 (`f1/LICENSE-Apache-2.0.md`, `f4/LICENSE-Core.txt`), лицензия ST для
заголовков устройств (`f4/LICENSE-Device.txt`); заголовки файлов сохранены. Файлы не форматируются
стилем проекта (`cmsis/.clang-format`) и не редактируются: для обновления замените их из нового выпуска
STM32Cube и исправьте таблицу.

Из CMSIS используются только заголовки: запуск (`src/startup.c`), тактирование и периферия
(`src/platform_f1.c`, `src/platform_f4.c`) написаны в проекте.
