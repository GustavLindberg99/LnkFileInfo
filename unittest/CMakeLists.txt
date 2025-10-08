cmake_minimum_required(VERSION 3.14)
project(lnkfileinfo_unittest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
    unittest
    unittest.cpp
)
target_link_libraries(
    unittest
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unittest)

include_directories(${CMAKE_SOURCE_DIR}/..)
add_compile_definitions(TEST_LNK_FILES_DIR="${CMAKE_SOURCE_DIR}/TestLnkFiles")
