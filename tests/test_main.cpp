// Unit test runner. Provides the globals that the game (or a tool) normally defines
// and that mclib expects to exist.

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"

#include "gameos.hpp"
#include "mclib.h"

#include "test_utils.h"

#include <filesystem>
#include <string>

UserHeapPtr systemHeap = NULL;
FastFile** fastFiles = NULL;
long numFastFiles = 0;
long maxFastFiles = 0;

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
    // same setup order as the game (mechcmd2.cpp): heap list first, then the heaps
    globalHeapList = new HeapList;
    globalHeapList->init();

    systemHeap = new UserHeap();
    systemHeap->init(32 * 1024 * 1024, "SYSTEM");

    Environment.checkCDForFiles = false;

    // File::open lowercases paths, so run in a lowercase scratch directory and use relative paths
    const fs::path original_dir = fs::current_path();
    const fs::path scratch_dir = fs::temp_directory_path() / ("mc2_tests_" + std::to_string(test_utils::process_id()));
    fs::remove_all(scratch_dir);
    fs::create_directories(scratch_dir);
    fs::current_path(scratch_dir);

    doctest::Context context;
    context.applyCommandLine(argc, argv);
    const int result = context.run();

    fs::current_path(original_dir);
    std::error_code ec;
    fs::remove_all(scratch_dir, ec);

    systemHeap->destroy();
    delete systemHeap;
    systemHeap = NULL;

    globalHeapList->destroy();
    delete globalHeapList;
    globalHeapList = NULL;

    return result;
}
