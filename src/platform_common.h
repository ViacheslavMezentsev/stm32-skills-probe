/* Общие части platform_f1.c и platform_f4.c: выводы, SysTick, DWT, USART1, TIM1 (регистры совпадают). */
#pragma once

#include "platform.h"

/* Выводы зонда: порт B, номера по ProbePin. */
static const uint8_t probe_pin_numbers[] = { 12, 13, 14, 15, 0 };

volatile uint32_t platform_tick;
volatile uint32_t platform_fault;
uint32_t SystemCoreClock;
static const char* clock_source = "HSI";
static int hse_failed;

/* Ожидание бита готовности без SysTick (тактирование ещё не настроено): ограничено числом опросов. */
static int wait_flag( volatile uint32_t* reg, uint32_t mask, uint32_t expected, uint32_t polls )
{
    while ( polls-- )
    {
        if ( ( *reg & mask ) == expected ) return 1;
    }
    return 0;
}

uint32_t platform_ticks( void )
{
    return platform_tick;
}

const char* platform_clock_source( void )
{
    return clock_source;
}

int platform_hse_failed( void )
{
    return hse_failed;
}

uint32_t platform_cpuid( void )
{
    return SCB->CPUID;
}

void SysTick_Handler( void )
{
    GPIOB->BSRR = 1U << probe_pin_numbers[PROBE_PIN_ISR];
    ++platform_tick;
    GPIOB->BSRR = 1U << ( probe_pin_numbers[PROBE_PIN_ISR] + 16U );
}

void platform_pin_write( ProbePin pin, int level )
{
    uint32_t bit = probe_pin_numbers[pin];
    GPIOB->BSRR  = level ? ( 1U << bit ) : ( 1U << ( bit + 16U ) );
}

void platform_pin_toggle( ProbePin pin )
{
    uint32_t bit = probe_pin_numbers[pin];
    platform_pin_write( pin, ( GPIOB->ODR & ( 1U << bit ) ) == 0 );
}

void platform_led_toggle( void )
{
    GPIOC->ODR ^= 1U << 13;
}

void platform_uart_write( const char* data, uint32_t length )
{
    while ( length-- )
    {
        while ( ( USART1->SR & USART_SR_TXE ) == 0 )
        {
        }
        USART1->DR = ( uint8_t ) *data++;
    }
}

int platform_uart_read( void )
{
    if ( USART1->SR & USART_SR_ORE )
    {
        ( void ) USART1->DR; /* сброс переполнения: чтение SR, затем DR */
    }
    if ( USART1->SR & USART_SR_RXNE ) return ( int ) ( USART1->DR & 0xFFU );
    return -1;
}

uint32_t platform_cycles( void )
{
    return DWT->CYCCNT;
}

void platform_pwm_set( uint32_t frequency_hz, uint32_t duty_permille )
{
    /* Таймер тактируется 1 МГц: PSC делит частоту ядра (APB2 без делителя). */
    uint32_t period = frequency_hz ? 1000000U / frequency_hz : 0U;
    if ( period < 2U || duty_permille > 1000U )
    {
        TIM1->CR1 &= ~TIM_CR1_CEN;
        return;
    }
    TIM1->CR1    = 0;
    TIM1->PSC    = SystemCoreClock / 1000000U - 1U;
    TIM1->ARR    = period - 1U;
    TIM1->CCR1   = period * duty_permille / 1000U;
    TIM1->CCMR1  = ( TIM1->CCMR1 & ~( TIM_CCMR1_OC1M | TIM_CCMR1_CC1S ) ) | ( 6U << TIM_CCMR1_OC1M_Pos ) | TIM_CCMR1_OC1PE;
    TIM1->CCER  |= TIM_CCER_CC1E;
    TIM1->BDTR  |= TIM_BDTR_MOE;
    TIM1->EGR    = TIM_EGR_UG;
    TIM1->CR1    = TIM_CR1_ARPE | TIM_CR1_CEN;
}

/* Общая часть запуска после выбора тактирования и включения тактов портов. */
static void platform_start( void )
{
    /* USART1 115200 8N1, передатчик и приёмник; APB2 без делителя. */
    USART1->BRR = ( SystemCoreClock + 115200U / 2U ) / 115200U;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;

    /* Счётчик тактов ядра для участков известной длины. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT       = 0;
    DWT->CTRL        |= DWT_CTRL_CYCCNTENA_Msk;

    /* Калибровочный ШИМ для анализатора: 1 кГц, 25 %. */
    platform_pwm_set( 1000U, 250U );

    SysTick_Config( SystemCoreClock / 1000U );
}
