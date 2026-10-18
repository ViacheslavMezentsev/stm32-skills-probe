# stm32-skills-probe

A neutral probe firmware for studying and checking the
[demo-stm32-skills](https://github.com/ViacheslavMezentsev/demo-stm32-skills) skills and
[stm32-gdbtest](https://github.com/ViacheslavMezentsev/stm32-gdbtest) scenarios. The probe behaves in a known
way (timestamped log, numbered frames, typed variables, debug pins, a known-length section, an RTT block),
so measurements by different tools can be compared, repeated and published.

Boards: `F411CE`, `F401CC` (WeAct BlackPill) and `F103C8` (Blue Pill); CMake and CMSIS only, stm32-gdbtest HIL
scenarios in `hil/`, Renode models in `emu/`, VS Code tasks and debug configurations in `.vscode/`. The Russian [README](README.md) is the primary documentation:
pinout, log format, commands, symbols for tools and build presets.
