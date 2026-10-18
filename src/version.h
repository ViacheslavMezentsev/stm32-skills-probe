#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Формат секции .fw_version: u8 major, minor, patch, reserved; u16 build, revision (little-endian). */
typedef struct
{
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint8_t reserved;
    uint16_t build;
    uint16_t revision;
} ProbeVersion;

extern const ProbeVersion probe_version;

#ifdef __cplusplus
}
#endif
