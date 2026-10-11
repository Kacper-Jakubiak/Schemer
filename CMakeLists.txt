cmake_minimum_required(VERSION 3.21)
project(Schemer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(Schemer
        include/Config.h
        include/SimulationSetup.h
        include/SchemerBuilder.h
        include/SchemerRunner.h
        include/FractionalScheme.h
        include/Schemer.h

        src/SchemerBuilder_setters.cpp
        src/SchemerBuilder_build.cpp
        src/SchemerRunner.cpp
        src/FractionalScheme.cpp
)

target_include_directories(Schemer PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

add_library(Schemer::Schemer ALIAS Schemer)

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
        eigen
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG 3.4.0
)
FetchContent_MakeAvailable(eigen)

target_link_libraries(Schemer PUBLIC Eigen3::Eigen)
