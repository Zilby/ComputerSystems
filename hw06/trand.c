
#include <stdlib.h>
#include "trand.h"

static int64_t tr_state = 541;
static int64_t tr_M     = 2147483647;
static int64_t tr_A     = 49979693;
static int64_t tr_B     = 2001030507;

void
trand_seed(int64_t ss)
{
    tr_state = 541 ^ ss;
}

int32_t
trand()
{
    tr_state = (tr_state * tr_A + tr_B) % tr_M;
    return abs((int32_t) tr_state);
}

int32_t
trand_range(int32_t bot, int32_t top) {
    return bot + trand() % (top - bot);
}
