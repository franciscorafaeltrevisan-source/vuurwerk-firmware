#include <assert.h>
#include <stdint.h>
#include "../air_soft.h"

int main(void)
{
    const uint8_t FM  = 0;
    const uint8_t AM  = 1;
    const uint8_t USB = 2;

    assert(AIRSOFT_ShouldApply(AM, 11800000u, AM));
    assert(AIRSOFT_ShouldApply(AM, 12452500u, AM));
    assert(AIRSOFT_ShouldApply(AM, 13700000u, AM));

    assert(!AIRSOFT_ShouldApply(AM, 11799999u, AM));
    assert(!AIRSOFT_ShouldApply(AM, 13700001u, AM));
    assert(!AIRSOFT_ShouldApply(FM, 12452500u, AM));
    assert(!AIRSOFT_ShouldApply(USB, 12452500u, AM));

    /* AIR SOFT 2: preserve de-emphasis + 3 kHz audio LPF, then
       tighten the AM IF/RF path to reduce hiss and harsh treble. */
    assert(AIRSOFT_REG_2B == 0x0400u);
    assert(AIRSOFT_REG_2F == 0x9990u);
    assert(AIRSOFT_REG_54 == 0x9009u);
    assert(AIRSOFT_REG_55 == 0x31A9u);
    assert(AIRSOFT_REG_43 == 0x2408u);

    return 0;
}
