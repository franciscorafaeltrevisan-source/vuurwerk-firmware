#ifndef AIR_SOFT_H
#define AIR_SOFT_H

#include <stdbool.h>
#include <stdint.h>

#define AIRSOFT_MIN_FREQ_10HZ 11800000u
#define AIRSOFT_MAX_FREQ_10HZ 13700000u

/* AIR SOFT 2 profile for aviation AM.
 * 0x2B: HPF off, 3 kHz LPF on, de-emphasis on.
 * 0x2F/0x54/0x55: AM IF/gain profile based on F4HWN's proven AM path.
 * 0x43: 12.5 kHz mode with 2.5 kHz RF filter for both strong/weak signal,
 *       intentionally trading some top-end for less hiss/harshness.
 */
#define AIRSOFT_REG_2B 0x0400u
#define AIRSOFT_REG_2F 0x9990u
#define AIRSOFT_REG_54 0x9009u
#define AIRSOFT_REG_55 0x31A9u
#define AIRSOFT_REG_43 0x2408u

static inline bool AIRSOFT_ShouldApply(uint8_t modulation, uint32_t frequency_10Hz, uint8_t am_mode)
{
    return modulation == am_mode &&
           frequency_10Hz >= AIRSOFT_MIN_FREQ_10HZ &&
           frequency_10Hz <= AIRSOFT_MAX_FREQ_10HZ;
}

#endif
