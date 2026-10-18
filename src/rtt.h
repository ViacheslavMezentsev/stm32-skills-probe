/*
 * Минимальный RTT, совместимый по раскладке с SEGGER RTT: OpenOCD и J-Link находят блок по строке
 * "SEGGER RTT" (символ _SEGGER_RTT). Один канал вверх (журнал) и один вниз (команды).
 * Запись не блокирует: сообщение, которое не помещается целиком, отбрасывается и считается.
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROBE_RTT_UP_SIZE   1024U
#define PROBE_RTT_DOWN_SIZE 64U

typedef struct
{
    const char* name;
    char* buffer;
    uint32_t size;
    volatile uint32_t write_offset;
    volatile uint32_t read_offset;
    uint32_t flags;
} ProbeRttBuffer;

typedef struct
{
    char id[16];
    int32_t max_up;
    int32_t max_down;
    ProbeRttBuffer up[1];
    ProbeRttBuffer down[1];
} ProbeRttControlBlock;

extern ProbeRttControlBlock _SEGGER_RTT;
extern volatile uint32_t probe_rtt_dropped; /* отброшено сообщений канала вверх */

void probe_rtt_init( void );
int probe_rtt_write( const char* data, uint32_t length ); /* 1 — записано, 0 — отброшено */
int probe_rtt_read( void );                               /* байт канала вниз или -1 */

#ifdef __cplusplus
}
#endif
