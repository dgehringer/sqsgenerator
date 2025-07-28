cmake_minimum_required(VERSION 3.27)

include(${CMAKE_SOURCE_DIR}/../cmake/RepoInfo.cmake)

repo_info("${CMAKE_SOURCE_DIR}/..")
set(SKBUILD_PROJECT_VERSION
        "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
        CACHE INTERNAL "" FORCE)
set(SKBUILD_PROJECT_VERSION_FULL ${SKBUILD_PROJECT_VERSION})

# copy all templates
file(GLOB TEMPLATE_JSON_FILES "${CMAKE_SOURCE_DIR}/../templates/*.json")
file(COPY ${TEMPLATE_JSON_FILES} DESTINATION "${CMAKE_SOURCE_DIR}/sqsgenerator/templates")

project(
        ${SKBUILD_PROJECT_NAME}
        VERSION ${SKBUILD_PROJECT_VERSION}
        LANGUAGES CXX)

set(VCPKG_CRT_LINKAGE "dynamic")
set(VCPKG_LIBRARY_LINKAGE "static")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
set(BUILD_SHARED_LIBS OFF)
if (WIN32)
    include(FetchContent)
    FetchContent_Declare(
            abseil
            GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
            GIT_TAG 20250512.1
            CMAKE_ARGS
            "-DCMAKE_VERBOSE_MAKEFILE=ON -DBUILD_SHARED_LIBS=OFF"
    )
    FetchContent_MakeAvailable(abseil)
endif ()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_definitions(-DUNICODE)
    add_definitions(-D_UNICODE)
    add_compile_options(/Zc:twoPhase- /Zc:__cplusplus /permissive-, /DNOMINMAX)
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    message(STATUS "Detected clang: adding -fexperimental-library")
    add_compile_options(-fexperimental-library)
endif ()

if (CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(-w)
endif ()

if (NOT WIN32)
    find_package(absl CONFIG REQUIRED)
endif ()
find_package(Python3 COMPONENTS Interpreter Development.Module REQUIRED)
find_package(pybind11 CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(boost_multiprecision REQUIRED CONFIG)
find_path(BS_THREAD_POOL_INCLUDE_DIRS "BS_thread_pool.hpp")
include_directories("${CMAKE_SOURCE_DIR}/../include" ${BS_THREAD_POOL_INCLUDE_DIRS})

add_compile_options(-DWITH_PYTHON)

pybind11_add_module(_core extension/sqsgen.cpp extension/utils.h)

target_link_libraries(_core PRIVATE pybind11::headers)
target_compile_definitions(_core PRIVATE
        SQSGEN_BUILD_COMMIT=${GIT_COMMIT_HASH}
        SQSGEN_BUILD_BRANCH=${GIT_BRANCH}
        SQSGEN_BUILD_NUMBER=${VERSION_PATCH}
        SQSGEN_MINOR_VERSION=${VERSION_MINOR}
        SQSGEN_MAJOR_VERSION=${VERSION_MAJOR}
)
target_include_directories(_core PRIVATE ${BS_THREAD_POOL_INCLUDE_DIRS})
target_include_directories(_core PRIVATE ${RAPIDHASH_INCLUDE_DIRS})
target_link_libraries(_core PRIVATE
        nlohmann_json::nlohmann_json
        absl::hash
        absl::raw_hash_map
        absl::raw_hash_set
        absl::log
        absl::str_format
        absl::log_initialize
        absl::flat_hash_map
        absl::flat_hash_set
        Boost::multiprecision
        Eigen3::Eigen ${Python3_LIBRARIES})
install(TARGETS _core DESTINATION sqsgenerator/core)

if (WIN32)
    install(TARGETS _core LIBRARY DESTINATION sqsgenerator/core)
endif ()
