/*
 * Запуск зонда: таблица векторов (ядро + PROBE_IRQ_COUNT прерываний устройства) и Reset_Handler.
 * Прерывания устройства зонд не использует (USART опрашивается), поэтому все они — Default_Handler.
 * Обработчики отказов отдельные: их имена ищут сценарии stm32-gdbtest (fault_handlers профиля).
 */
#include <stdint.h>

extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss;
extern int main( void );
extern void SysTick_Handler( void );

volatile uint32_t probe_fault_vector; /* номер исключения, попавшего в Default_Handler */

void Default_Handler( void )
{
    uint32_t ipsr;
    __asm__ volatile( "mrs %0, ipsr" : "=r"( ipsr ) );
    probe_fault_vector = ipsr;
    for ( ;; )
    {
    }
}

void NMI_Handler( void )
{
    for ( ;; )
    {
    }
}

void HardFault_Handler( void )
{
    for ( ;; )
    {
    }
}

void MemManage_Handler( void )
{
    for ( ;; )
    {
    }
}

void BusFault_Handler( void )
{
    for ( ;; )
    {
    }
}

void UsageFault_Handler( void )
{
    for ( ;; )
    {
    }
}

void Reset_Handler( void )
{
    uint32_t* src = &_sidata;
    for ( uint32_t* dst = &_sdata; dst < &_edata; )
        *dst++ = *src++;
    for ( uint32_t* dst = &_sbss; dst < &_ebss; )
        *dst++ = 0;
    main();
    for ( ;; )
    {
    }
}

__attribute__( ( section( ".isr_vector" ), used ) ) void ( *const probe_vectors[16 + PROBE_IRQ_COUNT] )( void ) = {
    [0]                           = ( void ( * )( void ) ) & _estack,
    [1]                           = Reset_Handler,
    [2]                           = NMI_Handler,
    [3]                           = HardFault_Handler,
    [4]                           = MemManage_Handler,
    [5]                           = BusFault_Handler,
    [6]                           = UsageFault_Handler,
    [11]                          = Default_Handler, /* SVCall */
    [12]                          = Default_Handler, /* DebugMon */
    [14]                          = Default_Handler, /* PendSV */
    [15]                          = SysTick_Handler,
    [16 ... 15 + PROBE_IRQ_COUNT] = Default_Handler,
};
