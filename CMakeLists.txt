cmake_minimum_required(VERSION 3.16)
project(armcomp)
set(CMAKE_CXX_STANDARD 20)
include(GNUInstallDirs)

add_library(${PROJECT_NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/filewriter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/filewriter.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utilities.hpp)

add_executable(${PROJECT_NAME}_compiler
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(${PROJECT_NAME}_compiler ${PROJECT_NAME})

# Copy Python scripts next to the executable
add_custom_command(
        TARGET ${PROJECT_NAME}
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/armsim ${CMAKE_BINARY_DIR})

option(ARMTRANS_BUILD_TESTS "Build test program for ARMTrans" OFF)
if(ARMTRANS_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.12.1)
    FetchContent_MakeAvailable(googletest)
    enable_testing()

    add_executable(${PROJECT_NAME}_test ${CMAKE_CURRENT_SOURCE_DIR}/test/test.cpp)
    target_link_libraries(${PROJECT_NAME}_test ${PROJECT_NAME} gtest_main)
    target_include_directories(${PROJECT_NAME}_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/)

    include(GoogleTest)
    gtest_discover_tests(${PROJECT_NAME}_test)

    # Copy gtest binaries next to the executable
    add_custom_command(
            TARGET ${PROJECT_NAME}_test
            PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/bin ${CMAKE_BINARY_DIR})
endif()
