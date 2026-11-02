#ifndef F4AM_H
#define F4AM_H

#include <stdbool.h>
#include <stdint.h>

#define F4AM_MIN_FREQ_10HZ 11800000u
#define F4AM_MAX_FREQ_10HZ 13700000u

/* F4HWN AM demodulator path for aviation band. */
#define F4AM_REG_42_AM     0x6F5Cu
#define F4AM_REG_2A_AM     0x7434u
#define F4AM_REG_2B_AM     0x0500u
#define F4AM_REG_2F_AM     0x9990u
#define F4AM_REG_54_AM     0x9009u
#define F4AM_REG_55_AM     0x31A9u

#define F4AM_REG_42_NORMAL 0x6B5Au
#define F4AM_REG_2A_NORMAL 0x7400u
#define F4AM_REG_2B_NORMAL 0x0000u
#define F4AM_REG_2F_NORMAL 0x9890u

/* REG48 AF Rx Gain-2: 0.5 dB/step. Keep stock 58 normally;
 * use 62 (+2 dB) only on F4AM aviation reception. */
#define F4AM_RX_GAIN2_AM     62u
#define F4AM_RX_GAIN2_NORMAL 58u

static inline bool F4AM_ShouldApply(uint8_t modulation, uint32_t frequency_10hz, uint8_t am_mode)
{
    return modulation == am_mode &&
           frequency_10hz >= F4AM_MIN_FREQ_10HZ &&
           frequency_10hz <= F4AM_MAX_FREQ_10HZ;
}

#endif
