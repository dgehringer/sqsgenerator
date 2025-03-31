cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(sqsgenerator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(CPM_DOWNLOAD_VERSION 0.40.8)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if (NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake")
    file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif ()


include(${CPM_DOWNLOAD_LOCATION})
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


find_package(GMP REQUIRED)

if (BUILD_TESTS OR BUILD_PYTHON)
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    CPMAddPackage("gh:pybind/pybind11@2.13.6")
endif ()

CPMAddPackage(
        NAME mppp
        GITHUB_REPOSITORY bluescarni/mppp
        GIT_TAG v2.0.0
        OPTIONS
        "MPPP_BUILD_STATIC_LIBRARY ON"
)
set_property(TARGET mp++ PROPERTY POSITION_INDEPENDENT_CODE ON)

CPMAddPackage("gh:nlohmann/json@3.11.3")
CPMAddPackage(
        NAME Eigen
        GITLAB_REPOSITORY libeigen/eigen
        GIT_TAG 3.4.0
        OPTIONS
        "EIGEN_BUILD_DOC OFF"           # Do not build documentation
        "EIGEN_BUILD_PKGCONFIG OFF"     # Optional, disables pkg-config
        "BUILD_TESTING OFF"             # Disable building and running tests
)
CPMAddPackage("gh:gabime/spdlog@1.15.0")
CPMAddPackage("gh:Nicoshev/rapidhash#rapidhash_v1.0")


CPMAddPackage(
        NAME BS_thread_pool
        GITHUB_REPOSITORY bshoshany/thread-pool
        VERSION 5.0.0
        EXCLUDE_FROM_ALL
        SYSTEM
)
add_library(BS_thread_pool INTERFACE)
target_include_directories(BS_thread_pool INTERFACE ${BS_thread_pool_SOURCE_DIR}/include)

if (WITH_MPI)
    find_package(MPI REQUIRED)
    CPMAddPackage("gh:rabauke/mpl@0.3.0")
    include_directories("${mpl_SOURCE_DIR}" "${MPI_INCLUDE_PATH}")
    add_compile_options(-DWITH_MPI)
endif ()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include" "${rapidhash_SOURCE_DIR}")

if (MSVC)
    add_compile_options(/permissive- /Zc:__cplusplus)
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    message(STATUS "Detected clang: adding -fexperimental-library")
    add_compile_options(-fexperimental-library)
endif ()

if (BUILD_TESTS)
    message(STATUS "BUILD_TESTS=${BUILD_TESTS}")
    add_subdirectory(tests)
endif ()

if (BUILD_BENCHMARKS)
    message(STATUS "BUILD_BENCHMARKS=${BUILD_BENCHMARKS}")
    add_subdirectory(bench)
endif ()

list(APPEND SQSGEN_LIBS "mp++")
list(APPEND SQSGEN_LIBS "Eigen3::Eigen")
list(APPEND SQSGEN_LIBS "nlohmann_json")
list(APPEND SQSGEN_LIBS "spdlog::spdlog_header_only")
list(APPEND SQSGEN_LIBS "BS_thread_pool")

if (WITH_MPI)
    message(STATUS "WITH_MPI=${WITH_MPI}")
    list(APPEND SQSGEN_LIBS "${MPI_LIBRARIES}")
endif ()

if (WITH_MPI AND BUILD_PYTHON)
    message(FATAL_ERROR "MPI and Python bindings are not supported together")
endif ()

if (BUILD_PYTHON)
    message(STATUS "BUILD_PYTHON=${BUILD_PYTHON}")
    list(APPEND SQSGEN_LIBS "pybind11::module")
    list(APPEND SQSGEN_LIBS "${Python3_LIBRARIES}")
    add_subdirectory("python/extension")
endif ()

message(STATUS "SQSGEN_LIBS=${SQSGEN_LIBS}")

if (BUILD_CLI)
    add_executable(sqsgen "main.cpp")
    target_link_libraries(sqsgen ${SQSGEN_LIBS})
endif ()
