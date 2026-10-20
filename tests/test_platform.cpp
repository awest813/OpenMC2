// GameOS file functions and the Win32 API shims used on non-Windows platforms

#include "doctest/doctest.h"

#include "gameos.hpp"
#include "toolos.hpp"

#include <filesystem>
#include <fstream>
#include <string>

#ifndef PLATFORM_WINDOWS
#include <sys/stat.h>
#include <utime.h>
#endif

namespace fs = std::filesystem;

namespace {

void write_text(const char* path, const std::string& text)
{
    std::ofstream f(path, std::ios::binary);
    f << text;
}

std::string read_text(const char* path)
{
    std::ifstream f(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

} // namespace

TEST_CASE("gos_OpenFile: READWRITE clears the file and can write and read")
{
    write_text("rw.txt", "old contents that must go away");

    HGOSFILE f = NULL;
    gos_OpenFile(&f, "rw.txt", READWRITE);
    REQUIRE(f != NULL);
    CHECK(gos_WriteFile(f, "hello", 5) == 5);
    CHECK(gos_SeekFile(f, FROMSTART, 1) == 1);
    CHECK(gos_SeekFile(f, FROMCURRENT, 2) == 3);
    CHECK(gos_SeekFile(f, FROMEND, 0) == 5);
    gos_SeekFile(f, FROMSTART, 0);
    char buf[6] = {};
    CHECK(gos_ReadFile(f, buf, 5) == 5);
    CHECK(std::string(buf) == "hello");
    gos_CloseFile(f);

    CHECK(read_text("rw.txt") == "hello");
}

TEST_CASE("gos_OpenFile: APPEND keeps the existing contents")
{
    write_text("append.txt", "abc");

    HGOSFILE f = NULL;
    gos_OpenFile(&f, "append.txt", APPEND);
    REQUIRE(f != NULL);
    CHECK(gos_WriteFile(f, "def", 3) == 3);
    gos_CloseFile(f);

    CHECK(read_text("append.txt") == "abcdef");
}

TEST_CASE("gos_DoesFileExist, gos_CreateDirectory and gos_GetFile")
{
    CHECK_FALSE(gos_DoesFileExist("no_such_file.txt"));

    CHECK(gos_CreateDirectory("made/by/gos"));
    CHECK(fs::is_directory("made/by/gos"));
    CHECK(gos_CreateDirectory("made/by/gos")); // already there is fine

    write_text("made/by/gos/data.bin", std::string("\x01\x02\x00\x03", 4));
    CHECK(gos_DoesFileExist("made/by/gos/data.bin"));

    BYTE* data = NULL;
    SIZE_T size = 0;
    gos_GetFile("made/by/gos/data.bin", &data, &size);
    REQUIRE(data != NULL);
    CHECK(size == 4);
    CHECK(data[0] == 1);
    CHECK(data[2] == 0);
    CHECK(data[3] == 3);
    gos_Free(data);
}

#ifndef PLATFORM_WINDOWS

TEST_CASE("Win32 shim: CopyFile keeps the timestamps on the copy, not the source")
{
    write_text("copy_src.txt", "copy me");
    struct utimbuf ut;
    ut.actime = 1000000000;  // 2001-09-09
    ut.modtime = 1000000000;
    REQUIRE(utime("copy_src.txt", &ut) == 0);

    CHECK(CopyFile("copy_src.txt", "copy_dst.txt", FALSE));
    CHECK(read_text("copy_dst.txt") == "copy me");

    struct stat src, dst;
    REQUIRE(stat("copy_src.txt", &src) == 0);
    REQUIRE(stat("copy_dst.txt", &dst) == 0);
    CHECK(dst.st_mtime == 1000000000);
    CHECK(src.st_mtime == 1000000000);

    CHECK_FALSE(CopyFile("copy_src.txt", "copy_dst.txt", TRUE)); // fail if exists
}

TEST_CASE("Win32 shim: FindFirstFile reports size and modification time")
{
    write_text("find_me.dat", std::string(12345, 'x'));
    struct utimbuf ut;
    ut.actime = 1600000000;
    ut.modtime = 1600000000;
    REQUIRE(utime("find_me.dat", &ut) == 0);

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA("find_me.dat", &fd);
    REQUIRE(h != INVALID_HANDLE_VALUE);
    CHECK(std::string(fd.cFileName) == "find_me.dat");
    CHECK(fd.nFileSizeHigh == 0);
    CHECK(fd.nFileSizeLow == 12345);

    const uint64_t filetime = ((uint64_t)fd.ftLastWriteTime.dwHighDateTime << 32) | fd.ftLastWriteTime.dwLowDateTime;
    const uint64_t unix_seconds = filetime / 10000000ull - 11644473600ull;
    CHECK(unix_seconds == 1600000000ull);
    FindClose(h);
}

TEST_CASE("Win32 shim: MessageBox answers without waiting")
{
    CHECK(MessageBox(NULL, "test", "test", MB_OK) == IDOK);
    // "insert the CD" retry loops stop on IDCANCEL
    CHECK(MessageBox(NULL, "test", "test", MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL);
    CHECK(MessageBox(NULL, "test", "test", MB_YESNO) == IDNO);
}

#endif // !PLATFORM_WINDOWS
