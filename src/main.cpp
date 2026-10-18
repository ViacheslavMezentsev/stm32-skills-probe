#include "platform.h"
#include "probe.h"

int main( void )
{
    platform_init();
    probe_init();
    for ( ;; )
        probe_loop();
}
