// Conversion of save game structures written by older 64-bit Linux builds ('long' members) to the
// current layout. The fixtures in tests/data were produced by tools/save_layout/gen_save_layout.py:
// save_v0_linux64.bin holds each structure as the OLD code laid it out, filled with a distinct
// pattern per field; save_v1_expected.bin holds the same values laid out by the NEW code.

#include "doctest/doctest.h"

#include "gameos.hpp"
#include "mclib.h"
#include "packet.h"
#include "savegame.h"

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#ifndef MC2_TEST_DATA_DIR
#error MC2_TEST_DATA_DIR must point at tests/data
#endif

namespace {

std::vector<unsigned char> read_fixture(const char* name)
{
    const std::string path = std::string(MC2_TEST_DATA_DIR) + "/" + name;
    std::ifstream f(path, std::ios::binary);
    REQUIRE_MESSAGE(f.good(), "missing fixture " << path);
    return std::vector<unsigned char>(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

} // namespace

TEST_CASE("Save conversion: old 64-bit Linux structures convert field by field")
{
    const std::vector<unsigned char> old_data = read_fixture("save_v0_linux64.bin");
    const std::vector<unsigned char> expected = read_fixture("save_v1_expected.bin");

    size_t old_pos = 0, new_pos = 0;
    for (int t = 0; t < NUM_SAVE_DATA_TYPES; ++t) {
        const SaveLayout& layout = getLegacySaveLayout((SaveDataType)t);
        CAPTURE(layout.name);
        REQUIRE(old_pos + layout.oldSize <= old_data.size());
        REQUIRE(new_pos + layout.newSize <= expected.size());

        std::vector<unsigned char> converted(layout.newSize, 0);
        convertLegacySaveData(layout, old_data.data() + old_pos, converted.data());
        CHECK(memcmp(converted.data(), expected.data() + new_pos, layout.newSize) == 0);

        old_pos += layout.oldSize;
        new_pos += layout.newSize;
    }
    CHECK(old_pos == old_data.size());
    CHECK(new_pos == expected.size());
}

TEST_CASE("Save conversion: old and new layouts have distinct sizes")
{
    // object packets are told apart by their size when loading
    std::vector<uint32_t> sizes;
    for (int t = 0; t < NUM_SAVE_DATA_TYPES; ++t) {
        const SaveLayout& layout = getLegacySaveLayout((SaveDataType)t);
        CHECK(layout.oldSize != layout.newSize);
        CHECK(isSaveDataSize((SaveDataType)t, layout.oldSize));
        CHECK(isSaveDataSize((SaveDataType)t, layout.newSize));
        sizes.push_back(layout.oldSize);
        sizes.push_back(layout.newSize);
    }
    for (size_t i = 0; i < sizes.size(); ++i)
        for (size_t j = i + 1; j < sizes.size(); ++j)
            CHECK(sizes[i] != sizes[j]);
}

TEST_CASE("Save conversion: readSaveData loads both layouts from a save file")
{
    const std::vector<unsigned char> old_data = read_fixture("save_v0_linux64.bin");
    const std::vector<unsigned char> expected = read_fixture("save_v1_expected.bin");

    // the first structure (MoverData) in both layouts, written like the game writes it (zlib packets)
    const SaveLayout& layout = getLegacySaveLayout(SAVE_MoverData);
    {
        PacketFile file;
        REQUIRE(file.create("conversion.sav") == NO_ERR);
        file.reserve(2);
        file.writePacket(0, (MemoryPtr)old_data.data(), layout.oldSize, STORAGE_TYPE_ZLIB);
        file.writePacket(1, (MemoryPtr)expected.data(), layout.newSize, STORAGE_TYPE_ZLIB);
        file.close();
    }

    PacketFile file;
    REQUIRE(file.open("conversion.sav") == NO_ERR);
    for (long packet = 0; packet < 2; ++packet) {
        CAPTURE(packet);
        std::vector<unsigned char> loaded(layout.newSize, 0);
        CHECK(readSaveData(&file, packet, SAVE_MoverData, loaded.data(), loaded.size()));
        CHECK(memcmp(loaded.data(), expected.data(), layout.newSize) == 0);
    }
    file.close();
}
