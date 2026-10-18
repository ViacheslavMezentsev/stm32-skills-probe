#include "probe.h"

#include "log.h"
#include "platform.h"
#include "rtt.h"
#include "version.h"

volatile ProbeTelemetry probe_telemetry;
volatile uint32_t probe_counter;
volatile uint32_t probe_state;
volatile uint32_t probe_frame_period_ms = 100;
volatile uint32_t probe_work_period_ms  = 1000;
volatile uint32_t probe_work_iterations = 4000;
volatile uint32_t probe_last_compute;

void ( *volatile probe_sample_hook )( void ) = probe_sample;

namespace
{
    uint32_t last_ms;
    uint32_t next_frame_ms;
    uint32_t next_work_ms;

    /* Синус ×1000 по таблице четверти периода (шаг 1/64 периода), без математической библиотеки. */
    const int16_t quarter_sine[17] = { 0, 98, 195, 290, 383, 471, 556, 634, 707, 773, 831, 882, 924, 957, 981, 995, 1000 };

    int32_t sine_milli( uint32_t phase64 )
    {
        uint32_t p = phase64 & 63U;
        if ( p <= 16U ) return quarter_sine[p];
        if ( p <= 32U ) return quarter_sine[32U - p];
        if ( p <= 48U ) return -quarter_sine[p - 32U];
        return -quarter_sine[64U - p];
    }

    const char* core_name( uint32_t cpuid )
    {
        switch ( ( cpuid >> 4 ) & 0xFFFU )
        {
            case 0xC23U:
                return "Cortex-M3";
            case 0xC24U:
                return "Cortex-M4";
            case 0xC27U:
                return "Cortex-M7";
            default:
                return "Cortex-M?";
        }
    }
}    // namespace

extern "C" __attribute__( ( noinline ) ) uint32_t probe_compute( uint32_t input )
{
    /* Небольшая детерминированная функция: xorshift от входа. */
    uint32_t x  = input * 2654435761U + 1U;
    x          ^= x << 13;
    x          ^= x >> 17;
    x          ^= x << 5;
    return x;
}

extern "C" __attribute__( ( noinline ) ) void probe_sample( void )
{
    platform_pin_toggle( PROBE_PIN_SAMPLE );
    probe_telemetry.samples = ( uint16_t ) ( probe_telemetry.samples + 1U );
    probe_last_compute      = probe_compute( probe_telemetry.samples );
}

extern "C" void probe_emit_frame( void )
{
    uint32_t now               = platform_ticks();
    uint32_t number            = probe_telemetry.frames + 1U;
    probe_telemetry.frames     = number;
    probe_telemetry.tick       = now;
    probe_telemetry.sine_milli = sine_milli( now * 64U / 2000U );
    probe_telemetry.ramp       = ( float ) ( now % 1000U ) / 1000.0F;
    platform_pin_toggle( PROBE_PIN_FRAME );
    sys_log( "frame %u tick=%u counter=%u sine=%d samples=%u", number, now, probe_counter, probe_telemetry.sine_milli,
        probe_telemetry.samples );
}

extern "C" void probe_burst( uint32_t count )
{
    probe_telemetry.mode = 1;
    for ( uint32_t i = 0; i < count; ++i )
        probe_emit_frame();
}

extern "C" void probe_gate( uint32_t count )
{
    platform_pin_write( PROBE_PIN_GATE, 1 );
    probe_burst( count );
    platform_pin_write( PROBE_PIN_GATE, 0 );
}

extern "C" __attribute__( ( noinline ) ) uint32_t probe_work( uint32_t iterations )
{
    platform_pin_write( PROBE_PIN_WORK, 1 );
    uint32_t start = platform_cycles();
    for ( uint32_t i = 0; i < iterations; ++i )
        __asm__ volatile( "nop" );
    uint32_t cycles = platform_cycles() - start;
    platform_pin_write( PROBE_PIN_WORK, 0 );
    probe_telemetry.work_cycles     = cycles;
    probe_telemetry.work_iterations = iterations;
    return cycles;
}

extern "C" void probe_init( void )
{
    probe_rtt_init();
    probe_telemetry.flags = ( uint8_t ) ( ( platform_clock_source()[2] == 'E' ? 1U : 0U ) | ( platform_hse_failed() ? 2U : 0U ) );
    uint32_t cpuid        = platform_cpuid();

    sys_log( "boot: start" );
    sys_log( "board: %s", PROBE_BOARD_NAME );
    sys_log( "CPU: %s (CPUID 0x%08X)", core_name( cpuid ), cpuid );
    sys_log( "clock: %s %u Hz%s", platform_clock_source(), SystemCoreClock, platform_hse_failed() ? " (HSE timeout)" : "" );
    sys_log( "Firmware: v%u.%u.%u build %u.%u", probe_version.major, probe_version.minor, probe_version.patch, probe_version.build,
        probe_version.revision );
    sys_log( "pins: isr=PB12 work=PB13 gate=PB14 frame=PB15 sample=PB0 pwm=PA8 tx=PA9 rx=PA10" );
    sys_log( "rtt: _SEGGER_RTT at 0x%08X up=%u down=%u", ( unsigned ) ( uintptr_t ) &_SEGGER_RTT, PROBE_RTT_UP_SIZE, PROBE_RTT_DOWN_SIZE );
    sys_log( "telemetry: probe_telemetry at 0x%08X, probe_counter at 0x%08X", ( unsigned ) ( uintptr_t ) &probe_telemetry,
        ( unsigned ) ( uintptr_t ) &probe_counter );
    probe_state = 0xA5U;
    sys_log( "boot: done" );

    last_ms       = platform_ticks();
    next_frame_ms = last_ms + probe_frame_period_ms;
    next_work_ms  = last_ms + probe_work_period_ms;
}

extern "C" void probe_loop( void )
{
    ++probe_counter;
    probe_commands_poll();

    uint32_t now = platform_ticks();
    if ( now != last_ms )
    {
        last_ms = now;
        probe_sample_hook();
    }
    if ( probe_frame_period_ms && ( int32_t ) ( now - next_frame_ms ) >= 0 )
    {
        next_frame_ms = now + probe_frame_period_ms;
        probe_emit_frame();
    }
#if PROBE_EXIT_MS
    if ( now >= PROBE_EXIT_MS )
    {
        sys_log( "exit: %u ms", now );
        probe_semihosting_exit();
    }
#endif
    if ( probe_work_period_ms && ( int32_t ) ( now - next_work_ms ) >= 0 )
    {
        next_work_ms = now + probe_work_period_ms;
        probe_work( probe_work_iterations );
        platform_led_toggle();
    }
}
