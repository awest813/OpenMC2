#include "doctest/doctest.h"

#include "lz.h"

#include "test_utils.h"

#include <vector>

namespace {

std::vector<unsigned char> roundtrip(std::vector<unsigned char> src)
{
    // zlib's worst case expansion is tiny, leave plenty of room
    std::vector<unsigned char> compressed(src.size() * 2 + 1024);
    const size_t compressed_size = LZCompress(compressed.data(), src.data(), src.size());
    REQUIRE(compressed_size > 0);
    REQUIRE(compressed_size <= compressed.size());

    std::vector<unsigned char> decompressed(src.size() + 1024);
    const size_t decompressed_size = LZDecomp(decompressed.data(), compressed.data(), compressed_size);
    decompressed.resize(decompressed_size);
    return decompressed;
}

} // namespace

TEST_CASE("LZ: random data round-trips")
{
    // sizes around the 16 KiB internal chunk size are the interesting ones
    const size_t sizes[] = { 1, 7, 1000, 16383, 16384, 16385, 32768, 100000 };
    for (size_t size : sizes) {
        CAPTURE(size);
        const std::vector<unsigned char> src = test_utils::random_bytes(size, (uint32_t)size);
        CHECK(roundtrip(src) == src);
    }
}

TEST_CASE("LZ: compressible data shrinks and round-trips")
{
    std::vector<unsigned char> src(200000);
    for (size_t i = 0; i < src.size(); ++i)
        src[i] = (unsigned char)("MechCommander"[i % 13]);

    std::vector<unsigned char> compressed(src.size() * 2 + 1024);
    const size_t compressed_size = LZCompress(compressed.data(), src.data(), src.size());
    CHECK(compressed_size > 0);
    CHECK(compressed_size < src.size() / 10);

    CHECK(roundtrip(src) == src);
}
