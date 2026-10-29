// upright — tests/up_pool_access.h
// the tests' window into a pool: its one render with one voice's share
// beside it (that voice's bridge force and its pickup's voltage, each at the
// level it is summed at: the force the board takes from it), for
// measurement (a stolen voice's fade, its damper) and for the pool's sum by
// hand. the pool's output is the same bytes either way.
#pragma once

#include "core/up_pool.h"

namespace up {

struct PoolTestAccess {
    // out, bridge and pickup may each be null
    static void render(PoolBase& p, float* out, int n, int which, float* bridge, float* pickup) {
        p.renderWith<true>(out, n, which, bridge, pickup);
    }
};

}  // namespace up
