#include "doctest/doctest.h"

#include "gameos.hpp"
#include "toolos.hpp"
#include "utils/timing.h"

#include <chrono>
#include <thread>

TEST_CASE("Timing: gos_GetHiResTime is in seconds and has sub-millisecond resolution")
{
    const auto wall_start = std::chrono::steady_clock::now();
    const double start = gos_GetHiResTime();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    const double elapsed = gos_GetHiResTime() - start;
    const double wall_elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - wall_start).count();

    CHECK(elapsed > 0.045);
    CHECK(elapsed < wall_elapsed + 0.01);

    // two consecutive reads should usually differ by far less than a millisecond
    double min_step = 1.0;
    double prev = gos_GetHiResTime();
    for (int i = 0; i < 1000; ++i) {
        const double now = gos_GetHiResTime();
        CHECK(now >= prev);
        if (now > prev && now - prev < min_step)
            min_step = now - prev;
        prev = now;
    }
    CHECK(min_step < 0.001);
}

TEST_CASE("Timing: timeGetTime counts milliseconds")
{
    const DWORD start = timeGetTime();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    const DWORD elapsed = timeGetTime() - start; // unsigned arithmetic also handles wrap around
    CHECK(elapsed >= 45);
    CHECK(elapsed < 1000);
}

TEST_CASE("Timing: timing::sleep handles durations of a second or more")
{
    timing::init();
    const uint64_t start = timing::gettickcount();
    timing::sleep(1100u * 1000u * 1000u); // 1.1 s
    const double elapsed_ms = timing::ticks2ms(timing::gettickcount() - start);
    CHECK(elapsed_ms >= 1050.0);
    CHECK(elapsed_ms < 5000.0);
    CHECK(timing::ticks2sec(timing::gettickcount() - start) >= 1.05);
}
