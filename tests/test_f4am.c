#include <assert.h>
#include <stdint.h>
#include "../f4am.h"

int main(void)
{
    const uint8_t FM = 0;
    const uint8_t AM = 1;
    const uint8_t USB = 2;

    assert(F4AM_ShouldApply(AM, 11800000u, AM));
    assert(F4AM_ShouldApply(AM, 12452500u, AM));
    assert(F4AM_ShouldApply(AM, 13700000u, AM));
    assert(!F4AM_ShouldApply(AM, 11799999u, AM));
    assert(!F4AM_ShouldApply(AM, 13700001u, AM));
    assert(!F4AM_ShouldApply(FM, 12452500u, AM));
    assert(!F4AM_ShouldApply(USB, 12452500u, AM));

    assert(F4AM_REG_42_AM == 0x6F5Cu);
    assert(F4AM_REG_2A_AM == 0x7434u);
    assert(F4AM_REG_2B_AM == 0x0500u);
    assert(F4AM_REG_2F_AM == 0x9990u);
    assert(F4AM_REG_54_AM == 0x9009u);
    assert(F4AM_REG_55_AM == 0x31A9u);

    assert(F4AM_REG_42_NORMAL == 0x6B5Au);
    assert(F4AM_REG_2A_NORMAL == 0x7400u);
    assert(F4AM_REG_2B_NORMAL == 0x0000u);
    assert(F4AM_REG_2F_NORMAL == 0x9890u);

    assert(F4AM_RX_GAIN2_AM == 62u);
    assert(F4AM_RX_GAIN2_NORMAL == 58u);

    /* BK4819 REG43 bit 2 adds +6 dB after demodulation. */
    assert(F4AM_POST_DEMOD_GAIN_BIT == (1u << 2));

    return 0;
}
