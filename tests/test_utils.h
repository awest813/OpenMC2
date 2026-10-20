#pragma once

#include <cstdint>
#include <vector>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace test_utils {

inline int process_id()
{
#ifdef _WIN32
    return _getpid();
#else
    return (int)getpid();
#endif
}

// deterministic pseudo random bytes (xorshift), so failures are reproducible
inline std::vector<unsigned char> random_bytes(size_t count, uint32_t seed)
{
    std::vector<unsigned char> data(count);
    uint32_t x = seed ? seed : 0x9e3779b9u;
    for (size_t i = 0; i < count; ++i) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        data[i] = (unsigned char)(x & 0xff);
    }
    return data;
}

} // namespace test_utils
