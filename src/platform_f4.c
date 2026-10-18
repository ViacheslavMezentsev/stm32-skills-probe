/* Платформа зонда для STM32F411CE и STM32F401CC (WeAct BlackPill), только CMSIS. */
#include "stm32f4xx.h"

#include "platform_common.h"

#define HSI_HZ 16000000U
#define HSE_HZ 25000000U /* кварц BlackPill */

static void clock_init( void )
{
    SystemCoreClock = HSI_HZ;
#ifdef PROBE_HSE
    /* HSE без PLL: 25 МГц не требуют тактов ожидания Flash (до 30 МГц при 2,7–3,6 В). */
    RCC->CR |= RCC_CR_HSEON;
    if ( wait_flag( &RCC->CR, RCC_CR_HSERDY, RCC_CR_HSERDY, 200000U ) )
    {
        RCC->CFGR = ( RCC->CFGR & ~RCC_CFGR_SW ) | RCC_CFGR_SW_HSE;
        if ( wait_flag( &RCC->CFGR, RCC_CFGR_SWS, RCC_CFGR_SWS_HSE, 200000U ) )
        {
            SystemCoreClock = HSE_HZ;
            clock_source    = "HSE";
            return;
        }
    }
    RCC->CR    &= ~RCC_CR_HSEON;
    hse_failed  = 1;
#endif
}

static void gpio_output( GPIO_TypeDef* port, uint32_t pin )
{
    port->MODER    = ( port->MODER & ~( 3U << ( pin * 2U ) ) ) | ( 1U << ( pin * 2U ) );
    port->OTYPER  &= ~( 1U << pin );
    port->OSPEEDR |= 3U << ( pin * 2U ); /* быстрые фронты для анализатора */
    port->PUPDR   &= ~( 3U << ( pin * 2U ) );
}

static void gpio_alternate( GPIO_TypeDef* port, uint32_t pin, uint32_t function )
{
    port->MODER           = ( port->MODER & ~( 3U << ( pin * 2U ) ) ) | ( 2U << ( pin * 2U ) );
    port->OSPEEDR        |= 3U << ( pin * 2U );
    port->AFR[pin >> 3U]  = ( port->AFR[pin >> 3U] & ~( 0xFU << ( ( pin & 7U ) * 4U ) ) ) | ( function << ( ( pin & 7U ) * 4U ) );
}

void platform_init( void )
{
    clock_init();

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_TIM1EN;
    ( void ) RCC->APB2ENR;

    /* Светодиод PC13 (горит при 0) выключен; выводы зонда на порту B. */
    GPIOC->BSRR = 1U << 13;
    gpio_output( GPIOC, 13 );
    for ( uint32_t i = 0; i < sizeof( probe_pin_numbers ); ++i )
    {
        GPIOB->BSRR = 1U << ( probe_pin_numbers[i] + 16U );
        gpio_output( GPIOB, probe_pin_numbers[i] );
    }

    gpio_alternate( GPIOA, 8, 1 );  /* TIM1_CH1 */
    gpio_alternate( GPIOA, 9, 7 );  /* USART1_TX */
    gpio_alternate( GPIOA, 10, 7 ); /* USART1_RX */

    platform_start();
}
