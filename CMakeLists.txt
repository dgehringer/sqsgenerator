cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(sqsgenerator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if (EMSCRIPTEN AND BUILD_WASM)
    set(VCPKG_C_FLAGS "-fPIC -fno-threadsafe-statics" CACHE STRING "")
    set(VCPKG_CXX_FLAGS "-fPIC -fno-threadsafe-statics" CACHE STRING "")
endif ()


if (CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(-w)  # Disables all warnings
    # OR for specific warnings:
    # add_compile_options(-Wno-unused-variable -Wno-deprecated-declarations)
endif ()


if (WITH_MPI AND BUILD_PYTHON)
    message(FATAL_ERROR "MPI and Python bindings are not supported together")
endif ()


if (BUILD_PYTHON)
    message(STATUS "BUILD_PYTHON=${BUILD_PYTHON}")
endif ()

if (BUILD_WASM)
    if (BUILD_PYTHON OR BUILD_TESTS OR WITH_MPI)
        message(FATAL_ERROR "WASM build is not compatible with Python, tests, or MPI")
    endif ()
    message(STATUS "BUILD_WASM=${BUILD_WASM}")
    add_subdirectory("js/wasm")
endif ()

if (BUILD_TESTS)
    message(STATUS "BUILD_TESTS=${BUILD_TESTS}")
    add_subdirectory(tests)
endif ()

if (BUILD_BENCHMARKS)
    message(STATUS "BUILD_BENCHMARKS=${BUILD_BENCHMARKS}")
    add_subdirectory(bench)
endif ()


if (BUILD_CLI)
    find_package(argparse CONFIG REQUIRED)

    add_compile_options("-DSQSGEN_MAJOR_VERSION=${SQSGEN_MAJOR_VERSION}")
    add_compile_options("-DSQSGEN_MINOR_VERSION=${SQSGEN_MINOR_VERSION}")
    add_compile_options("-DSQSGEN_BUILD_NUMBER=${SQSGEN_BUILD_NUMBER}")
    add_compile_options("-DSQSGEN_BUILD_BRANCH=${SQSGEN_BUILD_BRANCH}")
    add_compile_options("-DSQSGEN_BUILD_COMMIT=${SQSGEN_BUILD_COMMIT}")

    add_executable(sqsgen "cli/main.cpp" "cli/progress.h" "cli/helpers.h" "cli/termcolor.h")
    target_link_libraries(sqsgen PRIVATE nlohmann_json::nlohmann_json spdlog::spdlog_header_only Eigen3::Eigen Boost::multiprecision argparse::argparse)
    if (WITH_MPI)
        target_link_libraries(sqsgen PRIVATE ${MPI_LIBRARIES} mpl)
    endif ()
endif ()
