/* Версия сборки в секции .fw_version: её читает check_startup_log.py ({version}, {build}, {revision}). */
#include "version.h"

__attribute__( ( section( ".fw_version" ), used ) )
const ProbeVersion probe_version = { PROBE_VERSION_MAJOR, PROBE_VERSION_MINOR, PROBE_VERSION_PATCH, 0, PROBE_BUILD, PROBE_REVISION };
