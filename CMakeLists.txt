cmake_minimum_required(VERSION 3.16)
project(armtrans)
set(CMAKE_CXX_STANDARD 20)
include(GNUInstallDirs)

add_executable(${PROJECT_NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/filewriter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/filewriter.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utilities.hpp)

option(ARMTRANS_BUILD_TESTS "Build test program for ARMTrans" OFF)
if(ARMTRANS_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.12.1)
    FetchContent_MakeAvailable(googletest)
    enable_testing()

    add_executable(${PROJECT_NAME}_test ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp)
    target_link_libraries(${PROJECT_NAME}_test ${PROJECT_NAME} gtest_main)

    include(GoogleTest)
    gtest_discover_tests(${PROJECT_NAME}_test)

    # Copy gtest binaries next to the executable
    add_custom_command(
            TARGET ${PROJECT_NAME}_test
            PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/bin ${CMAKE_BINARY_DIR})
endif()
