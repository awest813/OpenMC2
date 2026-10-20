set(TEST_SOURCES
    test_main.cpp
    test_files.cpp
    test_lz.cpp
    test_pqueue.cpp
    test_strings.cpp
    test_timing.cpp
    test_stuff.cpp
    )

add_compile_definitions(DISABLE_GAMEOS_MAIN)

add_executable(mc2_tests ${TEST_SOURCES})
target_include_directories(mc2_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty" "${CMAKE_CURRENT_SOURCE_DIR}/../mclib")
target_link_libraries(mc2_tests mclib stuff gameos windows ZLIB::ZLIB ${SDL2_LIBRARIES} ${ADDITIONAL_LIBS})

add_test(NAME mc2_tests COMMAND mc2_tests)
