cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(sqsgenerator)
set(CMAKE_CXX_STANDARD 23)
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
CPMAddPackage("gh:fmtlib/fmt#11.0.2")
CPMAddPackage("gh:bluescarni/mppp@1.0.4")
CPMAddPackage("gh:nlohmann/json@3.11.3")
CPMAddPackage("gh:gabime/spdlog@1.14.1")
CPMAddPackage(
        GITLAB_REPOSITORY libeigen/eigen
        GIT_TAG 3.4.0
        OPTIONS "EIGEN_BUILD_DOC OFF"
        "EIGEN_BUILD_PKGCONFIG OF"
        "EIGEN_BUILD_DOC OFF"
        "EIGEN_BUILD_TESTING OFF"
)


include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

if (BUILD_TESTS)
    message(STATUS "BUILD_TESTS=${BUILD_TESTS}")
    add_subdirectory(tests)
endif ()

add_executable(sqsgenerator main.cpp
        include/sqsgen/core/atom.h
        include/sqsgen/types.h
        include/sqsgen/core/helpers/fold.h
        include/sqsgen/core/structure.h
        include/sqsgen/core/helpers/templates.h
        include/sqsgen/io/json.h)

target_link_libraries(sqsgenerator PRIVATE mp++ Eigen3::Eigen nlohmann_json)

