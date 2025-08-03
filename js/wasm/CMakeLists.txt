cmake_minimum_required(VERSION 3.27)

include(${CMAKE_SOURCE_DIR}/../../cmake/RepoInfo.cmake)
include(FetchContent)

repo_info("${CMAKE_SOURCE_DIR}/../..")

project(sqsgenerator-wasm
        VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
        LANGUAGES CXX)

set(VCPKG_EXCLUDED_PACKAGES "abseil")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -fno-threadsafe-statics -fPIC -pthread -mbulk-memory -matomics -fwasm-exceptions")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -mbulk-memory -matomics")
set(VCPKG_C_FLAGS "-Os -fno-threadsafe-statics -fPIC" CACHE STRING "")
set(VCPKG_CXX_FLAGS "-Os -fno-threadsafe-statics -fPIC" CACHE STRING "")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FetchContent_Declare(
        abseil
        GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
        GIT_TAG 20250512.1
        CMAKE_ARGS
        "-DCMAKE_VERBOSE_MAKEFILE=ON"
)
FetchContent_MakeAvailable(abseil)


find_package(Eigen3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(boost_multiprecision REQUIRED CONFIG)
find_path(BS_THREAD_POOL_INCLUDE_DIRS "BS_thread_pool.hpp")
include_directories("${CMAKE_SOURCE_DIR}/../../include" ${BS_THREAD_POOL_INCLUDE_DIRS})

add_executable(core sqsgen.cpp)
add_compile_options(-fvisibility=hidden)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND EMSCRIPTEN)
    target_compile_options(core PRIVATE -fexperimental-library)
endif ()
set_target_properties(core PROPERTIES
        SUFFIX ".js"
)
target_compile_definitions(core PRIVATE
        SQSGEN_BUILD_COMMIT=${GIT_COMMIT_HASH}
        SQSGEN_BUILD_BRANCH=${GIT_BRANCH}
        SQSGEN_BUILD_NUMBER=${VERSION_PATCH}
        SQSGEN_MINOR_VERSION=${VERSION_MINOR}
        SQSGEN_MAJOR_VERSION=${VERSION_MAJOR}
)
set_target_properties(core PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS}")
set_target_properties(core PROPERTIES LINK_FLAGS "-O3 --emit-tsd core.d.ts -sUSE_PTHREADS -s EXPORT_ES6=1 -WASM=2 -s ALLOW_MEMORY_GROWTH=1 -sPROXY_TO_PTHREAD -sEXIT_RUNTIME=0 -sASSERTIONS=2 -s MODULARIZE=1 --bind -fPIC -pthread --closure 1 -EXPORT_NAME=core")
target_link_libraries(core PRIVATE
        nlohmann_json::nlohmann_json
        absl::hash
        absl::log
        absl::str_format
        absl::log_initialize
        absl::flat_hash_map
        absl::flat_hash_set
        Eigen3::Eigen
        Boost::multiprecision
        embind
)
