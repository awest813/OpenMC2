#include "doctest/doctest.h"

#include "gameos.hpp"
#include "mclib.h"
#include "file.h"
#include "packet.h"

#include "test_utils.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// all paths are relative to the scratch directory test_main.cpp switches into

TEST_CASE("File: write then read back")
{
    const std::vector<unsigned char> data = test_utils::random_bytes(5000, 7);

    {
        File f;
        REQUIRE(f.create("file_roundtrip.bin") == NO_ERR);
        CHECK(f.write((MemoryPtr)data.data(), data.size()) == (long)data.size());
        f.close();
    }

    File f;
    REQUIRE(f.open("file_roundtrip.bin") == NO_ERR);
    CHECK(f.fileSize() == data.size());
    std::vector<unsigned char> read_back(data.size());
    CHECK(f.read(read_back.data(), (long)read_back.size()) == (long)read_back.size());
    CHECK(read_back == data);
    f.close();
}

TEST_CASE("File: Windows style paths from data files are converted and lowercased")
{
    // data files reference assets as e.g. "Data\Missions\Mc2_01.fit"
    fs::create_directories("winpath_dir/sub");
    {
        std::ofstream out("winpath_dir/sub/asset.txt", std::ios::binary);
        out << "hello";
    }

    File f;
    REQUIRE(f.open("WinPath_Dir\\SUB\\Asset.TXT") == NO_ERR);
    char buffer[6] = {};
    CHECK(f.read((MemoryPtr)buffer, 5) == 5);
    CHECK(std::string(buffer) == "hello");
    f.close();
}

TEST_CASE("PacketFile: raw, zlib and empty packets round-trip")
{
    const std::vector<unsigned char> raw = test_utils::random_bytes(3000, 1);
    std::vector<unsigned char> compressible(40000);
    for (size_t i = 0; i < compressible.size(); ++i)
        compressible[i] = (unsigned char)(i % 17);

    {
        PacketFile pak;
        REQUIRE(pak.create("test.pak") == NO_ERR);
        pak.reserve(3);
        // writePacket returns the number of bytes stored
        CHECK(pak.writePacket(0, (MemoryPtr)raw.data(), (unsigned int)raw.size(), STORAGE_TYPE_RAW) == (int)raw.size());
        CHECK(pak.writePacket(1, (MemoryPtr)compressible.data(), (unsigned int)compressible.size(), STORAGE_TYPE_ZLIB) > 0);
        pak.writePacket(2, nullptr, 0, STORAGE_TYPE_NUL);
        pak.close();
    }

    // the zlib packet should actually be stored compressed
    CHECK(fs::file_size("test.pak") < raw.size() + compressible.size() / 4);

    PacketFile pak;
    REQUIRE(pak.open("test.pak") == NO_ERR);
    REQUIRE(pak.getNumPackets() == 3);

    REQUIRE(pak.seekPacket(0) == NO_ERR);
    CHECK(pak.getStorageType() == STORAGE_TYPE_RAW);
    REQUIRE(pak.getPacketSize() == (int)raw.size());
    std::vector<unsigned char> buffer(raw.size());
    pak.readPacket(0, buffer.data());
    CHECK(buffer == raw);

    REQUIRE(pak.seekPacket(1) == NO_ERR);
    CHECK(pak.getStorageType() == STORAGE_TYPE_ZLIB);
    REQUIRE(pak.getPacketSize() == (int)compressible.size());
    buffer.assign(compressible.size(), 0);
    pak.readPacket(1, buffer.data());
    CHECK(buffer == compressible);

    // STORAGE_TYPE_NUL has the same value as ANY_PACKET_TYPE, so writePacket stores it as an empty RAW packet
    pak.seekPacket(2);
    CHECK(pak.getPacketSize() == 0);

    pak.close();
}
