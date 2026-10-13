cmake_minimum_required(VERSION 3.21)
project(Schemer VERSION 1.0.0 LANGUAGES CXX)

add_library(Schemer INTERFACE)

target_include_directories(Schemer INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(Schemer INTERFACE cxx_std_20)
add_library(Schemer::Schemer ALIAS Schemer)

include(FetchContent)
FetchContent_Declare(
        eigen
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG 3.4.0
)
FetchContent_MakeAvailable(eigen)

target_link_libraries(Schemer INTERFACE Eigen3::Eigen)

option(BUILD_TESTING "Build the testing tree for Schemer" ${PROJECT_IS_TOP_LEVEL})

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()