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

    return 0;
}
