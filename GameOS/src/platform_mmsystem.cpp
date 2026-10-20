#ifndef PLATFORM_WINDOWS

#include "platform_windef.h"
#include "platform_mmsystem.h"


#include <stdint.h>
#include <time.h>

DWORD timeGetTime()
{
    // wraps around every ~49.7 days like the Windows function (in integer math: converting an
    // out of range double to DWORD is undefined behaviour)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    uint64_t milliseconds = (uint64_t)ts.tv_sec * 1000u + ts.tv_nsec / 1000000;
    return (DWORD)milliseconds;
}

#endif // PLATFORM_WINDOWS