/* Платформа зонда для STM32F103C8 (Blue Pill), только CMSIS. */
#include "stm32f1xx.h"

#include "platform_common.h"

#define HSI_HZ 8000000U
#define HSE_HZ 8000000U /* кварц Blue Pill */

static void clock_init( void )
{
    SystemCoreClock = HSI_HZ;
#ifdef PROBE_HSE
    /* HSE без PLL: частота та же, что у HSI, но от кварца — точнее для калибровки времени. */
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

/* Поле CNF/MODE вывода: 4 бита в CRL (0–7) или CRH (8–15). */
static void gpio_config( GPIO_TypeDef* port, uint32_t pin, uint32_t mode )
{
    volatile uint32_t* reg = pin < 8U ? &port->CRL : &port->CRH;
    uint32_t shift         = ( pin & 7U ) * 4U;
    *reg                   = ( *reg & ~( 0xFU << shift ) ) | ( mode << shift );
}

#define GPIO_OUT_PP_50MHZ 0x3U /* CNF 00, MODE 11 */
#define GPIO_AF_PP_50MHZ  0xBU /* CNF 10, MODE 11 */
#define GPIO_IN_FLOATING  0x4U /* CNF 01, MODE 00 */

void platform_init( void )
{
    clock_init();

    RCC->APB2ENR |=
        RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN | RCC_APB2ENR_USART1EN | RCC_APB2ENR_TIM1EN;
    ( void ) RCC->APB2ENR;

    /* Светодиод PC13 (горит при 0) выключен; выводы зонда на порту B. PB3 (SWO) не трогается. */
    GPIOC->BSRR = 1U << 13;
    gpio_config( GPIOC, 13, 0x2U ); /* выход 2 МГц: PC13 слаботочный */
    for ( uint32_t i = 0; i < sizeof( probe_pin_numbers ); ++i )
    {
        GPIOB->BSRR = 1U << ( probe_pin_numbers[i] + 16U );
        gpio_config( GPIOB, probe_pin_numbers[i], GPIO_OUT_PP_50MHZ );
    }

    gpio_config( GPIOA, 8, GPIO_AF_PP_50MHZ );  /* TIM1_CH1 */
    gpio_config( GPIOA, 9, GPIO_AF_PP_50MHZ );  /* USART1_TX */
    gpio_config( GPIOA, 10, GPIO_IN_FLOATING ); /* USART1_RX */

    platform_start();
}
