#include "doctest/doctest.h"

#include "gameos.hpp"
#include "utils/viewport.h"

using presentation::Rect;
using presentation::compute_viewport;

TEST_CASE("Viewport: same aspect ratio fills the drawable")
{
    const Rect r = compute_viewport(800, 600, 1600, 1200, false);
    CHECK(r.x == 0); CHECK(r.y == 0); CHECK(r.w == 1600); CHECK(r.h == 1200);
}

TEST_CASE("Viewport: 4:3 on a 16:9 display is pillarboxed")
{
    const Rect r = compute_viewport(1024, 768, 1920, 1080, false);
    CHECK(r.h == 1080);
    CHECK(r.w == 1440);
    CHECK(r.x == 240);
    CHECK(r.y == 0);
}

TEST_CASE("Viewport: 16:9 in a 4:3 window is letterboxed")
{
    const Rect r = compute_viewport(1920, 1080, 1024, 768, false);
    CHECK(r.w == 1024);
    CHECK(r.h == 576);
    CHECK(r.x == 0);
    CHECK(r.y == 96);
}

TEST_CASE("Viewport: stretching, and degenerate sizes, use the whole drawable")
{
    Rect r = compute_viewport(1024, 768, 1920, 1080, true);
    CHECK(r.w == 1920); CHECK(r.h == 1080); CHECK(r.x == 0); CHECK(r.y == 0);
    r = compute_viewport(0, 0, 640, 480, false);
    CHECK(r.w == 640); CHECK(r.h == 480);
}

TEST_CASE("Viewport: window coordinates map into the viewport, also on HiDPI")
{
    // 4:3 game in a 1920x1080 drawable; window is half the size (2x display scaling)
    const Rect vp = compute_viewport(800, 600, 1920, 1080, false);
    float x = -1, y = -1;
    presentation::window_to_viewport(480.0f, 270.0f, 960, 540, 1920, 1080, vp, &x, &y); // window centre
    CHECK(x == doctest::Approx(0.5f));
    CHECK(y == doctest::Approx(0.5f));
    // in the left black bar: clamped to the left edge
    presentation::window_to_viewport(10.0f, 270.0f, 960, 540, 1920, 1080, vp, &x, &y);
    CHECK(x == 0.0f);

    float sx = 0, sy = 0;
    presentation::window_to_logical_scale(960, 540, 1920, 1080, vp, 800, 600, &sx, &sy);
    // 1 window unit = 2 drawable pixels = 2 * 800/1440 logical pixels
    CHECK(sx == doctest::Approx(2.0f * 800.0f / 1440.0f));
    CHECK(sy == doctest::Approx(2.0f * 600.0f / 1080.0f));
}
