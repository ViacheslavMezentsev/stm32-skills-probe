# =============================================================================
# Эмуляция в Renode без платы (как в stm32-hwtest-bluepill):
#   ctest -L emu       — дымовой тест tools/renode_smoke.py;
#   цель emulate       — тот же прогон вручную;
#   цель emulate-debug — Renode с GDB-сервером на порту 3334, ядро ждёт подключения;
#   цель emulate-monitor — зонд работает в Renode, монитор открыт на порту 1234 (запись stm32-diag-record).
# Путь к Renode: -DRENODE_EXECUTABLE=..., переменная RENODE_BINARY или PATH.
# =============================================================================
option(PROBE_RENODE_TEST "Renode smoke test (ctest -L emu) when Renode is found" ON)
set(PROBE_RENODE_MONITOR_PORT 1234 CACHE STRING "Renode monitor (telnet) port of the emulate-monitor target")
if(PROBE_RENODE_TEST)
    if(DEFINED ENV{RENODE_BINARY} AND NOT RENODE_EXECUTABLE)
        set(RENODE_EXECUTABLE "$ENV{RENODE_BINARY}" CACHE FILEPATH "Renode executable")
    endif()
    find_program(RENODE_EXECUTABLE NAMES renode Renode PATHS "$ENV{ProgramFiles}/Renode")
    find_package(Python3 3.8 COMPONENTS Interpreter QUIET)
endif()

if(PROBE_RENODE_TEST AND RENODE_EXECUTABLE AND Python3_Interpreter_FOUND)
    set(probe_smoke
        "${Python3_EXECUTABLE}" -B "${CMAKE_SOURCE_DIR}/tools/renode_smoke.py"
        --elf "$<TARGET_FILE:${PROJECT_NAME}>" --board "${BOARD}"
        --renode "${RENODE_EXECUTABLE}" --objcopy "${CMAKE_OBJCOPY}")
    enable_testing()
    add_test(NAME emu.renode_smoke COMMAND ${probe_smoke})
    set_tests_properties(emu.renode_smoke PROPERTIES LABELS emu TIMEOUT 180)
    add_custom_target(emulate COMMAND ${probe_smoke} DEPENDS ${PROJECT_NAME} USES_TERMINAL VERBATIM)

    if(PROBE_SERIES STREQUAL "f1")
        set(RENODE_MODEL "${CMAKE_SOURCE_DIR}/emu/renode/stm32f103_probe.repl")
    else()
        set(RENODE_MODEL "${CMAKE_SOURCE_DIR}/emu/renode/stm32f4_probe.repl")
    endif()
    set(RENODE_ELF "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf")
    set(RENODE_LOG "${CMAKE_BINARY_DIR}/renode-debug/uart.log")
    set(RENODE_RUN_LOG "${CMAKE_BINARY_DIR}/renode-run/uart.log")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/renode-debug" "${CMAKE_BINARY_DIR}/renode-run")
    foreach(var RENODE_MODEL RENODE_ELF RENODE_LOG RENODE_RUN_LOG)
        string(REPLACE " " "\\ " ${var} "${${var}}")
    endforeach()
    configure_file("${CMAKE_SOURCE_DIR}/cmake/renode-debug.resc.in" "${CMAKE_BINARY_DIR}/renode-debug/debug.resc" @ONLY)
    configure_file("${CMAKE_SOURCE_DIR}/cmake/renode-run.resc.in" "${CMAKE_BINARY_DIR}/renode-run/run.resc" @ONLY)
    string(REPLACE " " "\\ " RENODE_SCRIPT "${CMAKE_BINARY_DIR}/renode-debug/debug.resc")
    add_custom_target(emulate-debug
        COMMAND "${RENODE_EXECUTABLE}" --disable-gui --console --plain -e "include @${RENODE_SCRIPT}"
        DEPENDS ${PROJECT_NAME} USES_TERMINAL VERBATIM)
    string(REPLACE " " "\\ " RENODE_RUN_SCRIPT "${CMAKE_BINARY_DIR}/renode-run/run.resc")
    add_custom_target(emulate-monitor
        COMMAND "${RENODE_EXECUTABLE}" --disable-gui --plain --port ${PROBE_RENODE_MONITOR_PORT}
                -e "include @${RENODE_RUN_SCRIPT}"
        DEPENDS ${PROJECT_NAME} USES_TERMINAL VERBATIM)
    message(STATUS "Renode: ${RENODE_EXECUTABLE}")
endif()
