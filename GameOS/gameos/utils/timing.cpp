#include "timing.h"

#ifdef PLATFORM_WINDOWS
#include<windows.h>
#include<time.h>
#else
#include<time.h>
#endif

#include <stdint.h>
#include <cassert>

namespace timing {

#ifdef PLATFORM_WINDOWS
	static LARGE_INTEGER Frequency;
#endif
#ifdef _DEBUG
	static bool initialized = false;
#endif

	void init()
	{
#ifdef PLATFORM_WINDOWS
		QueryPerformanceFrequency(&Frequency);
#endif
#ifdef _DEBUG
		initialized = true;
#endif
	}

	void sleep(unsigned int nanosec) {
#ifdef PLATFORM_WINDOWS
		Sleep(nanosec / 1000000);
#else
		// tv_nsec must be below one second
		timespec ts;
		ts.tv_sec = nanosec / 1000000000u;
		ts.tv_nsec = nanosec % 1000000000u;
		nanosleep(&ts, NULL);
#endif
	}

	uint64_t gettickcount()
	{
#ifdef PLATFORM_WINDOWS
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);
		return t.QuadPart;
#else
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
		uint64_t nanosec = (uint64_t)ts.tv_sec * 1000000000u + ts.tv_nsec;
		return nanosec;
#endif
	}

	double ticks2ms(uint64_t ticks)
	{
#ifdef _DEBUG
		assert(initialized);
#endif
#ifdef PLATFORM_WINDOWS
		double dt = (double)(ticks) / Frequency.QuadPart;
		return 1000 * dt;
#else
		return (double)ticks / 1e+6;
#endif
	}

	double ticks2sec(uint64_t ticks)
	{
#ifdef _DEBUG
		assert(initialized);
#endif
#ifdef PLATFORM_WINDOWS
		return (double)(ticks) / Frequency.QuadPart;
#else
		return (double) ticks / 1e+9;
#endif
	}

    uint64_t get_wall_time_ms()
    {
        // or clock_gettime(CLOCKREALTIME, ts);
        struct timespec ts;
        timespec_get(&ts, TIME_UTC);
		uint64_t milliseconds = (uint64_t)ts.tv_sec * 1000u;
		milliseconds += ts.tv_nsec / 1000000;
        return milliseconds;
    }
}
