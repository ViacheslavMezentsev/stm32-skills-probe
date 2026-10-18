#include "rtt.h"

static char rtt_up_buffer[PROBE_RTT_UP_SIZE] __attribute__( ( section( ".rtt.buffers" ) ) );
static char rtt_down_buffer[PROBE_RTT_DOWN_SIZE] __attribute__( ( section( ".rtt.buffers" ) ) );

ProbeRttControlBlock _SEGGER_RTT __attribute__( ( section( ".rtt.cb" ), aligned( 8 ) ) );
volatile uint32_t probe_rtt_dropped;

void probe_rtt_init( void )
{
    /* Секция .rtt без загрузки: поля заполняются явно, строка ID — последней, чтобы отладчик
       не нашёл недозаполненный блок. */
    _SEGGER_RTT.max_up   = 1;
    _SEGGER_RTT.max_down = 1;

    _SEGGER_RTT.up[0].name         = "Terminal";
    _SEGGER_RTT.up[0].buffer       = rtt_up_buffer;
    _SEGGER_RTT.up[0].size         = sizeof( rtt_up_buffer );
    _SEGGER_RTT.up[0].write_offset = 0;
    _SEGGER_RTT.up[0].read_offset  = 0;
    _SEGGER_RTT.up[0].flags        = 0; /* SEGGER_RTT_MODE_NO_BLOCK_SKIP */

    _SEGGER_RTT.down[0].name         = "Terminal";
    _SEGGER_RTT.down[0].buffer       = rtt_down_buffer;
    _SEGGER_RTT.down[0].size         = sizeof( rtt_down_buffer );
    _SEGGER_RTT.down[0].write_offset = 0;
    _SEGGER_RTT.down[0].read_offset  = 0;
    _SEGGER_RTT.down[0].flags        = 0;

    static const char id[] = "SEGGER RTT";
    for ( int i = ( int ) sizeof( id ) - 1; i >= 0; --i )
    {
        __asm__ volatile( "" ::: "memory" );
        _SEGGER_RTT.id[i] = id[i];
    }
}

int probe_rtt_write( const char* data, uint32_t length )
{
    ProbeRttBuffer* up = &_SEGGER_RTT.up[0];
    uint32_t write     = up->write_offset;
    uint32_t read      = up->read_offset;
    uint32_t free      = read > write ? read - write - 1U : up->size - ( write - read ) - 1U;
    if ( length > free )
    {
        ++probe_rtt_dropped;
        return 0;
    }
    for ( uint32_t i = 0; i < length; ++i )
    {
        up->buffer[write] = data[i];
        if ( ++write == up->size ) write = 0;
    }
    __asm__ volatile( "dmb" ::: "memory" ); /* данные видны раньше нового смещения */
    up->write_offset = write;
    return 1;
}

int probe_rtt_read( void )
{
    ProbeRttBuffer* down = &_SEGGER_RTT.down[0];
    uint32_t read        = down->read_offset;
    if ( read == down->write_offset ) return -1;
    int value         = ( uint8_t ) down->buffer[read];
    down->read_offset = ( read + 1U == down->size ) ? 0U : read + 1U;
    return value;
}
