/*
 * Команды зонда: строки по UART (PA10) или каналу RTT вниз, завершаются '\n' или '\r'.
 *   ping                 -> pong <n>
 *   status               -> состояние и настройки
 *   rate <мс>            -> период кадров (0 — выкл.)
 *   burst <n>            -> n кадров подряд
 *   gate <n>             -> поднять PB14, n кадров, опустить PB14
 *   work <итерации>      -> участок известной длины сейчас, ответ с тактами DWT
 *   period <мс>          -> период участка известной длины (0 — выкл.)
 *   sinks <маска>        -> каналы журнала: 1 UART, 2 RTT, 4 ITM, 8 семихостинг
 *   pwm <Гц> <промилле>  -> ШИМ на PA8
 */
#include "log.h"
#include "platform.h"
#include "probe.h"
#include "rtt.h"

namespace
{
    /* Своя строка у каждого источника: байты UART и RTT не перемешиваются. */
    struct Parser
    {
        char line[48];
        uint32_t length;
    };

    Parser uart_parser;
    Parser rtt_parser;
    uint32_t pongs;

    bool starts( const char* text, const char* word )
    {
        while ( *word )
        {
            if ( *text++ != *word++ ) return false;
        }
        return *text == ' ' || *text == '\0';
    }

    /* Беззнаковое число после n-го пробела; fallback, если его нет. */
    uint32_t argument( const char* text, int index, uint32_t fallback )
    {
        for ( int found = 0; *text; ++text )
        {
            if ( *text == ' ' && ++found == index )
            {
                ++text;
                if ( *text < '0' || *text > '9' ) return fallback;
                uint32_t value = 0;
                while ( *text >= '0' && *text <= '9' )
                    value = value * 10U + ( uint32_t ) ( *text++ - '0' );
                return value;
            }
        }
        return fallback;
    }

    void execute( const char* text )
    {
        if ( starts( text, "ping" ) )
            sys_log( "pong %u", ++pongs );
        else if ( starts( text, "status" ) )
            sys_log( "status frames=%u rate=%u work=%u/%u sinks=%u rtt_dropped=%u", probe_telemetry.frames, probe_frame_period_ms,
                probe_work_iterations, probe_work_period_ms, probe_log_sinks, probe_rtt_dropped );
        else if ( starts( text, "rate" ) )
        {
            probe_frame_period_ms = argument( text, 1, probe_frame_period_ms );
            sys_log( "rate %u", probe_frame_period_ms );
        }
        else if ( starts( text, "burst" ) )
            probe_burst( argument( text, 1, 10 ) );
        else if ( starts( text, "gate" ) )
            probe_gate( argument( text, 1, 10 ) );
        else if ( starts( text, "work" ) )
        {
            uint32_t iterations = argument( text, 1, probe_work_iterations );
            uint32_t cycles     = probe_work( iterations );
            sys_log( "work iterations=%u cycles=%u", iterations, cycles );
        }
        else if ( starts( text, "period" ) )
        {
            probe_work_period_ms = argument( text, 1, probe_work_period_ms );
            sys_log( "period %u", probe_work_period_ms );
        }
        else if ( starts( text, "sinks" ) )
        {
            probe_log_sinks = argument( text, 1, probe_log_sinks ) & 0xFU;
            sys_log( "sinks %u", probe_log_sinks );
        }
        else if ( starts( text, "pwm" ) )
        {
            uint32_t hz   = argument( text, 1, 1000 );
            uint32_t duty = argument( text, 2, 250 );
            platform_pwm_set( hz, duty );
            sys_log( "pwm %u %u", hz, duty );
        }
        else if ( text[0] )
            sys_log( "error: unknown command" );
    }

    void feed( Parser& parser, int value )
    {
        if ( value < 0 ) return;
        if ( value == '\n' || value == '\r' )
        {
            parser.line[parser.length] = '\0';
            if ( parser.length ) execute( parser.line );
            parser.length = 0;
        }
        else if ( parser.length + 1U < sizeof( parser.line ) )
            parser.line[parser.length++] = ( char ) value;
    }
}    // namespace

extern "C" void probe_commands_poll( void )
{
    feed( uart_parser, platform_uart_read() );
    feed( rtt_parser, probe_rtt_read() );
}
