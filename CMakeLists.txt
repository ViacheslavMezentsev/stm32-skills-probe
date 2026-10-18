cmake_minimum_required(VERSION 3.25)

# Toolchain GNU Arm (xPack 14.2.1) — до project().
if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/arm-gcc.cmake")
endif()

project(stm32_skills_probe VERSION 0.2.0 LANGUAGES C CXX ASM)

# Python ищется сначала по PATH, а не в реестре Windows (как в stm32-hwtest-*).
set(Python3_FIND_REGISTRY LAST)

# Плата, тактирование, каналы журнала и сама прошивка.
include(cmake/probe.cmake)

# Дымовой тест в Renode и цели emulate / emulate-debug.
include(cmake/emulation.cmake)

# HIL-сценарии stm32-gdbtest (выключены по умолчанию).
include(cmake/hil.cmake)
