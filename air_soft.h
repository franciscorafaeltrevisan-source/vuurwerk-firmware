#ifndef AIR_SOFT_H
#define AIR_SOFT_H

#include <stdbool.h>
#include <stdint.h>

#define AIRSOFT_MIN_FREQ_10HZ 11800000u
#define AIRSOFT_MAX_FREQ_10HZ 13700000u

static inline bool AIRSOFT_ShouldApply(uint8_t modulation, uint32_t frequency_10Hz, uint8_t am_mode)
{
    return modulation == am_mode &&
           frequency_10Hz >= AIRSOFT_MIN_FREQ_10HZ &&
           frequency_10Hz <= AIRSOFT_MAX_FREQ_10HZ;
}

#endif
