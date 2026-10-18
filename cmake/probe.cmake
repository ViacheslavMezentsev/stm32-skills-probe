# =============================================================================
# Прошивка-зонд: плата (BOARD), версия и необязательные каналы журнала.
# =============================================================================
set(BOARD "F411CE" CACHE STRING "Board: F411CE, F401CC (WeAct BlackPill) or F103C8 (Blue Pill)")
set_property(CACHE BOARD PROPERTY STRINGS F411CE F401CC F103C8)
if(DEFINED PROBE_CONFIGURED_BOARD AND NOT PROBE_CONFIGURED_BOARD STREQUAL BOARD)
    message(FATAL_ERROR "Use a separate build directory for each BOARD")
endif()
set(PROBE_CONFIGURED_BOARD "${BOARD}" CACHE INTERNAL "Configured board")

if(BOARD STREQUAL "F411CE")
    set(PROBE_SERIES f4)
    set(PROBE_DEVICE STM32F411xE)
    set(PROBE_CORE cortex-m4)
    set(FLASH_SIZE 512K)
    set(RAM_SIZE 128K)
    set(PROBE_IRQ_COUNT 86)
    set(PROBE_PROFILE f411ce)
elseif(BOARD STREQUAL "F401CC")
    set(PROBE_SERIES f4)
    set(PROBE_DEVICE STM32F401xC)
    set(PROBE_CORE cortex-m4)
    set(FLASH_SIZE 256K)
    set(RAM_SIZE 64K)
    set(PROBE_IRQ_COUNT 85)
    set(PROBE_PROFILE f401cc)
elseif(BOARD STREQUAL "F103C8")
    set(PROBE_SERIES f1)
    set(PROBE_DEVICE STM32F103xB)
    set(PROBE_CORE cortex-m3)
    set(FLASH_SIZE 64K)
    set(RAM_SIZE 20K)
    set(PROBE_IRQ_COUNT 43)
    set(PROBE_PROFILE f103c8)
else()
    message(FATAL_ERROR "Unknown BOARD=${BOARD}; use F411CE, F401CC or F103C8")
endif()
message(STATUS "Probe board: ${BOARD} (${PROBE_DEVICE}, Flash ${FLASH_SIZE}, RAM ${RAM_SIZE})")

# Тактирование: HSE без PLL (кварц платы: 25 МГц BlackPill, 8 МГц Blue Pill) с откатом на HSI,
# если кварц не запустился (в эмуляторе бит готовности HSE не выставляется).
option(PROBE_HSE "Clock from the board crystal (HSE) without PLL; HSI fallback" ON)
# Каналы журнала: UART и RTT (без отладчика безвредны) всегда, ITM пишет, только если отладчик его
# включил; семихостинг без отладчика останавливает ядро, поэтому только по выбору.
option(PROBE_LOG_SEMIHOSTING "Duplicate the log over ARM semihosting (needs a debugger or emulator)" OFF)
# Завершение через семихостинг (SYS_EXIT) через заданное число миллисекунд: QEMU останавливается сам,
# Renode останавливает ядро. Нужно маршрутам эмуляции, которые ждут конца прогона (stm32-renode-emulator).
set(PROBE_EXIT_MS 0 CACHE STRING "Semihosting SYS_EXIT after this many ms (0: never; needs PROBE_LOG_SEMIHOSTING)")
if(PROBE_EXIT_MS AND NOT PROBE_LOG_SEMIHOSTING)
    message(FATAL_ERROR "PROBE_EXIT_MS needs PROBE_LOG_SEMIHOSTING=ON: without a debugger SYS_EXIT stops the core")
endif()

# Номер сборки и ревизия из окружения CI (иначе 0): попадают в секцию .fw_version.
set(PROBE_BUILD "$ENV{PROBE_BUILD}")
if(NOT PROBE_BUILD)
    set(PROBE_BUILD 0)
endif()
set(PROBE_REVISION "$ENV{PROBE_REVISION}")
if(NOT PROBE_REVISION)
    set(PROBE_REVISION 0)
endif()

configure_file("${CMAKE_SOURCE_DIR}/ld/probe.ld.in" "${CMAKE_BINARY_DIR}/probe.ld" @ONLY)
set(CMSIS_DIR "${CMAKE_SOURCE_DIR}/cmsis/${PROBE_SERIES}")

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/probe.cpp
    src/commands.cpp
    src/log.c
    src/rtt.c
    src/version.c
    src/startup.c
    src/platform_${PROBE_SERIES}.c)

set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX ".elf"
    C_STANDARD 11
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    LINK_DEPENDS "${CMAKE_BINARY_DIR}/probe.ld")

target_include_directories(${PROJECT_NAME} PRIVATE src "${CMSIS_DIR}/Core/Include" "${CMSIS_DIR}/Device/Include")
target_compile_definitions(${PROJECT_NAME} PRIVATE
    ${PROBE_DEVICE}
    BOARD_${BOARD}
    PROBE_BOARD_NAME="${BOARD}"
    PROBE_IRQ_COUNT=${PROBE_IRQ_COUNT}
    PROBE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    PROBE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    PROBE_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    PROBE_BUILD=${PROBE_BUILD}
    PROBE_REVISION=${PROBE_REVISION}
    $<$<BOOL:${PROBE_HSE}>:PROBE_HSE=1>
    $<$<BOOL:${PROBE_LOG_SEMIHOSTING}>:PROBE_LOG_SEMIHOSTING=1>
    PROBE_EXIT_MS=${PROBE_EXIT_MS})

set(PROBE_ARCH -mcpu=${PROBE_CORE} -mthumb -mfloat-abi=soft)
target_compile_options(${PROJECT_NAME} PRIVATE
    ${PROBE_ARCH}
    -ffunction-sections -fdata-sections -fno-lto
    -Wall -Wextra -Werror
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-use-cxa-atexit>
    $<$<CONFIG:Debug>:-Og -g3>
    $<$<CONFIG:Release>:-Os -g>)

target_link_options(${PROJECT_NAME} PRIVATE
    ${PROBE_ARCH}
    -nostdlib
    "-T${CMAKE_BINARY_DIR}/probe.ld"
    -Wl,--gc-sections
    "-Wl,-Map=$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.map"
    -Wl,--print-memory-usage
    $<$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,12.0>:-Wl,--no-warn-rwx-segments>)
target_link_libraries(${PROJECT_NAME} PRIVATE gcc)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O ihex "$<TARGET_FILE:${PROJECT_NAME}>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.hex"
    COMMAND "${CMAKE_OBJCOPY}" -O binary --gap-fill 0xFF "$<TARGET_FILE:${PROJECT_NAME}>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.bin"
    COMMAND "${CMAKE_SIZE}" "$<TARGET_FILE:${PROJECT_NAME}>"
    VERBATIM)
