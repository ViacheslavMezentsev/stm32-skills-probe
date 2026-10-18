#include "log.h"

#include <stdarg.h>

#include "platform.h"
#include "rtt.h"

#if defined( STM32F103xB )
#include "stm32f1xx.h"
#else
#include "stm32f4xx.h"
#endif

volatile uint32_t probe_log_sinks = PROBE_SINK_UART | PROBE_SINK_RTT | PROBE_SINK_ITM
#ifdef PROBE_LOG_SEMIHOSTING
                                    | PROBE_SINK_SEMI
#endif
    ;
volatile uint32_t probe_log_lines;

typedef struct
{
    char* data;
    uint32_t length;
    uint32_t capacity;
} Line;

static void put( Line* line, char c )
{
    if ( line->length + 1U < line->capacity ) line->data[line->length++] = c;
}

static void put_text( Line* line, const char* text )
{
    while ( *text )
        put( line, *text++ );
}

static void put_number( Line* line, uint32_t value, uint32_t base, int upper, int width, char fill, int negative )
{
    char digits[12];
    int count          = 0;
    const char* symbol = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    do
    {
        digits[count++]  = symbol[value % base];
        value           /= base;
    } while ( value && count < ( int ) sizeof( digits ) );
    if ( negative )
    {
        if ( fill == '0' ) put( line, '-' );
        --width;
    }
    for ( int i = count; i < width; ++i )
        put( line, fill );
    if ( negative && fill != '0' ) put( line, '-' );
    while ( count )
        put( line, digits[--count] );
}

static void format_line( Line* line, const char* format, va_list args )
{
    for ( ; *format; ++format )
    {
        if ( *format != '%' )
        {
            put( line, *format );
            continue;
        }
        ++format;
        char fill = ' ';
        int width = 0;
        if ( *format == '0' )
        {
            fill = '0';
            ++format;
        }
        while ( *format >= '0' && *format <= '9' )
            width = width * 10 + ( *format++ - '0' );
        switch ( *format )
        {
            case 's':
                put_text( line, va_arg( args, const char* ) );
                break;
            case 'c':
                put( line, ( char ) va_arg( args, int ) );
                break;
            case 'u':
                put_number( line, va_arg( args, unsigned ), 10, 0, width, fill, 0 );
                break;
            case 'd':
            {
                int value = va_arg( args, int );
                put_number( line, value < 0 ? 0U - ( unsigned ) value : ( unsigned ) value, 10, 0, width, fill, value < 0 );
                break;
            }
            case 'x':
            case 'X':
                put_number( line, va_arg( args, unsigned ), 16, *format == 'X', width, fill, 0 );
                break;
            case '%':
                put( line, '%' );
                break;
            default:
                return;
        }
    }
}

static void semihosting_write0( const char* text )
{
#ifdef PROBE_LOG_SEMIHOSTING
    register uint32_t r0 __asm__( "r0" )    = 0x04U; /* SYS_WRITE0 */
    register const char* r1 __asm__( "r1" ) = text;
    __asm__ volatile( "bkpt 0xAB" : "+r"( r0 ) : "r"( r1 ) : "memory" );
#else
    ( void ) text;
#endif
}

void sys_log( const char* format, ... )
{
    char buffer[128];
    Line line = { buffer, 0, sizeof( buffer ) - 1U }; /* место под '\n' */

    uint32_t ms = platform_ticks();
    put( &line, '[' );
    put_number( &line, ms / 1000U, 10, 0, 4, ' ', 0 );
    put( &line, '.' );
    put_number( &line, ms % 1000U, 10, 0, 3, '0', 0 );
    put_text( &line, "] " );

    va_list args;
    va_start( args, format );
    format_line( &line, format, args );
    va_end( args );
    buffer[line.length++] = '\n';

    uint32_t sinks = probe_log_sinks;
    if ( sinks & PROBE_SINK_UART ) platform_uart_write( buffer, line.length );
    if ( sinks & PROBE_SINK_RTT ) probe_rtt_write( buffer, line.length );
    if ( sinks & PROBE_SINK_ITM )
    {
        /* ITM_SendChar сама проверяет, включены ли ITM и порт 0; без отладчика — пропуск. */
        if ( ( ITM->TCR & ITM_TCR_ITMENA_Msk ) && ( ITM->TER & 1U ) )
        {
            for ( uint32_t i = 0; i < line.length; ++i )
                ITM_SendChar( ( uint32_t ) buffer[i] );
        }
    }
    if ( sinks & PROBE_SINK_SEMI )
    {
        buffer[line.length] = '\0';
        semihosting_write0( buffer );
    }
    ++probe_log_lines;
}
