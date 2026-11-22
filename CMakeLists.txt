cmake_minimum_required(VERSION 3.21)
project(InterpolationLab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(InterpLib STATIC
    src/Interpolation.cpp
    src/InterpolationTask.cpp
 "include/ScopeLogger.h")
target_include_directories(InterpLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(InterpDemo demo/main.cpp "include/ScopeLogger.h")
target_link_libraries(InterpDemo PRIVATE InterpLib)

add_executable(AntipatternDemo antipattern/main.cpp "include/ScopeLogger.h")


find_package(GTest CONFIG REQUIRED)
enable_testing()

add_executable(InterpTests tests/InterpolationTests.cpp "include/ScopeLogger.h")
target_link_libraries(InterpTests PRIVATE InterpLib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(InterpTests)