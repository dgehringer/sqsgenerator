cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(sqsgenerator)
set(CMAKE_CXX_STANDARD 20)
include(FetchContent)

set(CPM_DOWNLOAD_VERSION 0.40.2)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake")
    file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()


include(${CPM_DOWNLOAD_LOCATION})
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


find_package(GMP REQUIRED)
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
CPMAddPackage("gh:bluescarni/mppp@1.0.4")
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
CPMAddPackage("gh:pybind/pybind11@2.13.6")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include" "${rapidhash_SOURCE_DIR}")


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


message(STATUS "SQSGEN_LIBS=${SQSGEN_LIBS}")

add_executable(sqsgenerator main.cpp
        include/sqsgen/core/atom.h
        include/sqsgen/types.h
        include/sqsgen/core/helpers/fold.h
        include/sqsgen/core/structure.h
        include/sqsgen/core/helpers/templates.h
        include/sqsgen/io/json.h
        include/sqsgen/core/helpers/numeric.h
        include/sqsgen/core/helpers/for_each.h
        include/sqsgen/core/helpers/hash.h
        include/sqsgen/config.h
        include/sqsgen/io/config/structure.h
        include/sqsgen/io/config/composition.h
        include/sqsgen/core/helpers/sorted_vector.h
        include/sqsgen/io/parsing.h
        include/sqsgen/io/config/shell_radii.h
        include/sqsgen/io/config/shell_weights.h
        include/sqsgen/io/config/arrays.h
        include/sqsgen/io/config/shared.h
        include/sqsgen/io/config/combined.h)

target_link_libraries(sqsgenerator PRIVATE ${SQSGEN_LIBS})
target_link_libraries(sqsgenerator PRIVATE ${Python3_LIBRARIES})
target_link_libraries(sqsgenerator PRIVATE pybind11::pybind11)
