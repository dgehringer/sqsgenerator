cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(sqsgenerator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(MPPP_BUILD_STATIC_LIBRARY ON CACHE INTERNAL "Build mp++ as a static library")
FetchContent_Declare(
        mp++
        GIT_REPOSITORY https://github.com/bluescarni/mppp.git
        GIT_TAG v2.0.0
)
FetchContent_MakeAvailable(mp++)
set_property(TARGET mp++ PROPERTY POSITION_INDEPENDENT_CODE ON)


find_package(PkgConfig REQUIRED)
pkg_check_modules(gmp REQUIRED IMPORTED_TARGET gmp)
find_package(Eigen3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

if (WITH_MPI)
    find_package(MPI REQUIRED)
    CPMAddPackage("gh:rabauke/mpl@0.3.0")
    include_directories("${mpl_SOURCE_DIR}" "${MPI_INCLUDE_PATH}")
    add_compile_options(-DWITH_MPI)
endif ()

if (MSVC)
    add_compile_options(/permissive- /Zc:__cplusplus)
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    message(STATUS "Detected clang: adding -fexperimental-library")
    add_compile_options(-fexperimental-library)
endif ()

if (MSVC)
    add_compile_options(/permissive- /EHsc /nologo /W4 /Zc:twoPhase-)
endif ()


if (BUILD_TESTS)
    message(STATUS "BUILD_TESTS=${BUILD_TESTS}")
    add_subdirectory(tests)
endif ()

if (BUILD_BENCHMARKS)
    message(STATUS "BUILD_BENCHMARKS=${BUILD_BENCHMARKS}")
    add_subdirectory(bench)
endif ()


if (WITH_MPI)
    message(STATUS "WITH_MPI=${WITH_MPI}")
    list(APPEND SQSGEN_LIBS "${MPI_LIBRARIES}")
endif ()

if (WITH_MPI AND BUILD_PYTHON)
    message(FATAL_ERROR "MPI and Python bindings are not supported together")
endif ()

if (BUILD_PYTHON)
    message(STATUS "BUILD_PYTHON=${BUILD_PYTHON}")
    find_package(Python3 COMPONENTS Interpreter Development)
    find_package(pybind11 CONFIG REQUIRED)
    add_subdirectory("python/extension")
endif ()


if (BUILD_CLI)
    add_executable(sqsgen "main.cpp")
    target_link_libraries(sqsgen ${SQSGEN_LIBS})
endif ()
