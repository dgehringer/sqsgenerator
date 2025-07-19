cmake_minimum_required(VERSION 3.27)

include(${CMAKE_SOURCE_DIR}/../cmake/RepoInfo.cmake)
repo_info("${CMAKE_SOURCE_DIR}/..")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WITH_MPI)
    find_package(MPI REQUIRED)
    FetchContent_Declare(
            mpl
            GIT_REPOSITORY https://github.com/rabauke/mpl.git
            GIT_TAG v0.3.0
    )
    FetchContent_MakeAvailable(mpl)
    add_compile_options(-DWITH_MPI)
endif ()

if (MSVC)
    add_definitions(-DUNICODE)
    add_definitions(-D_UNICODE)
    add_compile_options(/Zc:twoPhase- /Zc:__cplusplus /MT /permissive-)
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    message(STATUS "Detected clang: adding -fexperimental-library")
    add_compile_options(-fexperimental-library)
endif ()

find_package(argparse CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(boost_multiprecision REQUIRED CONFIG)
find_path(BS_THREAD_POOL_INCLUDE_DIRS "BS_thread_pool.hpp")
include_directories("${CMAKE_SOURCE_DIR}/../include" ${BS_THREAD_POOL_INCLUDE_DIRS})

add_executable(sqsgen main.cpp progress.h helpers.h cli/termcolor.h)
target_link_libraries(sqsgen PRIVATE nlohmann_json::nlohmann_json spdlog::spdlog_header_only Eigen3::Eigen Boost::multiprecision argparse::argparse)
if (WITH_MPI)
    target_link_libraries(sqsgen PRIVATE ${MPI_LIBRARIES} mpl)
endif ()
