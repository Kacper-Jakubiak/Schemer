cmake_minimum_required(VERSION 3.21)

include(FetchContent)
FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.4.11
)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_MakeAvailable(doctest)

add_executable(schemer_tests test_main.cpp)
target_link_libraries(schemer_tests PRIVATE Schemer::Schemer doctest::doctest)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(schemer_tests)