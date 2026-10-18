/*
 * Зонд: известное поведение, которое наблюдают навыки (журнал, кадры, переменные, выводы, нагрузка).
 * Имена с C-связыванием: их ищут GDB-сценарии, resolve_addresses.py и перехваты RTT.
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Поля разных типов: маршрут A stm32-ros-alt-channel и поля структур по DWARF. */
typedef struct
{
    uint32_t frames;      /* номер последнего кадра */
    uint32_t tick;        /* platform_tick в момент кадра */
    int32_t sine_milli;   /* синус ×1000 с периодом 2 с */
    float ramp;           /* пила 0..1 с периодом 1 с */
    uint16_t samples;     /* вызовов probe_sample() по модулю 65536 */
    uint8_t mode;         /* 0 — штатный режим, 1 — после команды burst или gate */
    uint8_t flags;        /* бит 0 — HSE, бит 1 — HSE не запустился */
    uint32_t work_cycles; /* такты последнего участка известной длины (DWT) */
    uint32_t work_iterations;
} ProbeTelemetry;

extern volatile ProbeTelemetry probe_telemetry;
extern volatile uint32_t probe_counter; /* итерации главного цикла */
extern volatile uint32_t probe_state;   /* константа 0xA5 после старта */
extern volatile uint32_t probe_frame_period_ms;
extern volatile uint32_t probe_work_period_ms;
extern volatile uint32_t probe_work_iterations;
extern volatile uint32_t probe_last_compute; /* результат probe_compute() */

/* Вызывается раз в миллисекунду через указатель в RAM: цель вариантов перехвата RTT. */
extern void ( *volatile probe_sample_hook )( void );
void probe_sample( void );
uint32_t probe_compute( uint32_t input ); /* возвращаемое значение — для перехвата по lr */

void probe_init( void );
void probe_loop( void );
void probe_emit_frame( void );
void probe_burst( uint32_t count );
void probe_gate( uint32_t count );
uint32_t probe_work( uint32_t iterations );

void probe_commands_poll( void );

#ifdef __cplusplus
}
#endif
