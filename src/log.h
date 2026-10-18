/*
 * Журнал зонда: строки с префиксом времени "[   1.234] " в выбранные каналы.
 * UART и RTT — по умолчанию; ITM пишет, только если отладчик включил ITM и порт 0;
 * семихостинг — при сборке с PROBE_LOG_SEMIHOSTING (без отладчика остановил бы ядро).
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    PROBE_SINK_UART = 1U << 0,
    PROBE_SINK_RTT  = 1U << 1,
    PROBE_SINK_ITM  = 1U << 2,
    PROBE_SINK_SEMI = 1U << 3,
};

extern volatile uint32_t probe_log_sinks; /* маска каналов, меняется командой sinks */
extern volatile uint32_t probe_log_lines; /* выведено строк */

/* Формат: %s %c %u %d %x %X, ширина и заполнение нулями (%08X), %%. Целые — 32 бита (int и long
   на Cortex-M одного размера), поэтому uint32_t передаётся как есть; проверка printf не включается. */
void sys_log( const char* format, ... );

/* Семихостинг SYS_EXIT (ADP_Stopped_ApplicationExit); без PROBE_LOG_SEMIHOSTING ничего не делает. */
void probe_semihosting_exit( void );

#ifdef __cplusplus
}
#endif
