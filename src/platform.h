/*
 * Платформа зонда: тактирование, SysTick 1 кГц, отладочные выводы, USART1, ШИМ TIM1, DWT.
 * Реализации: platform_f1.c (STM32F103) и platform_f4.c (STM32F401/F411), только CMSIS.
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Отладочные выводы для логического анализатора (одинаковы на BlackPill и Blue Pill). */
typedef enum
{
    PROBE_PIN_ISR    = 0, /* PB12: высокий уровень внутри SysTick_Handler */
    PROBE_PIN_WORK   = 1, /* PB13: высокий уровень на участке известной длины */
    PROBE_PIN_GATE   = 2, /* PB14: «гейт» сценария, поднимается командой gate */
    PROBE_PIN_FRAME  = 3, /* PB15: переключается на каждом кадре */
    PROBE_PIN_SAMPLE = 4, /* PB0:  переключается в probe_sample() (цель перехватов RTT) */
} ProbePin;

extern volatile uint32_t platform_tick;  /* миллисекунды от старта (SysTick) */
extern volatile uint32_t platform_fault; /* код отказа, 0 — нет */
extern uint32_t SystemCoreClock;

void platform_init( void );
uint32_t platform_ticks( void );
const char* platform_clock_source( void ); /* "HSE" или "HSI" */
int platform_hse_failed( void );           /* 1, если HSE запрашивали, но он не запустился */
uint32_t platform_cpuid( void );

void platform_pin_write( ProbePin pin, int level );
void platform_pin_toggle( ProbePin pin );
void platform_led_toggle( void );

void platform_uart_write( const char* data, uint32_t length ); /* блокирующая передача */
int platform_uart_read( void );                                /* байт или -1 */

uint32_t platform_cycles( void ); /* DWT CYCCNT; 0, если счётчика нет (часть эмуляторов) */
void platform_pwm_set( uint32_t frequency_hz, uint32_t duty_permille ); /* TIM1_CH1 на PA8 */


#ifdef __cplusplus
}
#endif
