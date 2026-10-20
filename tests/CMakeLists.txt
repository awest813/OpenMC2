set(TEST_SOURCES
    test_main.cpp
    test_audio.cpp
    test_files.cpp
    test_input.cpp
    test_lz.cpp
    test_platform.cpp
    test_pqueue.cpp
    test_strings.cpp
    test_struct_layout.cpp
    test_timing.cpp
    test_stuff.cpp
    )

add_compile_definitions(DISABLE_GAMEOS_MAIN)

add_executable(mc2_tests ${TEST_SOURCES})
target_include_directories(mc2_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty" "${CMAKE_CURRENT_SOURCE_DIR}/../mclib")
target_link_libraries(mc2_tests mclib stuff gameos windows ZLIB::ZLIB ${SDL2_LIBRARIES} SDL2_mixer::SDL2_mixer GLEW::GLEW OpenGL::GL ${ADDITIONAL_LIBS})

add_test(NAME mc2_tests COMMAND mc2_tests)
