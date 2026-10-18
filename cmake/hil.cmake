# =============================================================================
# HIL-сценарии stm32-gdbtest (метод DDTT), выключены по умолчанию.
# Включение: пресеты *-hil (-DPROBE_HIL=ON). Сценарии и требования общие (hil/tests); плату описывает
# конфигурация прогона hil/sessions/<плата>.toml. Устроено как в stm32-hwtest-blackpill.
# =============================================================================
option(PROBE_HIL "Hardware scenarios via stm32-gdbtest" OFF)

if(PROBE_HIL)
    set(HIL_SESSION "${CMAKE_SOURCE_DIR}/hil/sessions/${PROBE_PROFILE}.toml")
    set(STM32_GDBTEST_SOURCE_DIR "${CMAKE_SOURCE_DIR}/modules/stm32-gdbtest" CACHE PATH "stm32-gdbtest checkout")
    if(NOT EXISTS "${STM32_GDBTEST_SOURCE_DIR}/stm32_gdbtest/cmake/STM32GDBTest.cmake")
        message(FATAL_ERROR "stm32-gdbtest not found: ${STM32_GDBTEST_SOURCE_DIR}\nRun: git submodule update --init")
    endif()

    # GDB с Python из того же toolchain, если не задан явно.
    if(NOT STM32_GDBTEST_GDB AND ARM_TOOLCHAIN_ROOT)
        foreach(name arm-none-eabi-gdb-py3 arm-none-eabi-gdb)
            if(EXISTS "${ARM_TOOLCHAIN_ROOT}/bin/${name}${ARM_TOOLCHAIN_EXT}")
                set(STM32_GDBTEST_GDB "${ARM_TOOLCHAIN_ROOT}/bin/${name}${ARM_TOOLCHAIN_EXT}" CACHE FILEPATH "GDB with Python")
                break()
            endif()
        endforeach()
    endif()

    # Типы CMSIS, которые прошивка не использует, GCC не пишет в отладочную информацию; сценарии
    # обращаются к ним по именам, поэтому в HIL-сборке типы сохраняются (код не меняется).
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-eliminate-unused-debug-types)

    message(STATUS "HIL: run configuration ${HIL_SESSION}")
    include(CTest)
    include("${STM32_GDBTEST_SOURCE_DIR}/stm32_gdbtest/cmake/STM32GDBTest.cmake")
    stm32_gdbtest_attach(${PROJECT_NAME}
        PROFILE_DIR "${CMAKE_SOURCE_DIR}/hil"
        SESSION_CONFIG "${HIL_SESSION}"
        MANIFEST_INPUTS
            "${CMAKE_BINARY_DIR}/probe.ld"
            "${CMAKE_SOURCE_DIR}/cmake/probe.cmake"
            "${CMAKE_CURRENT_LIST_FILE}")
endif()
