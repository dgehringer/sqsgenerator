cmake_minimum_required(VERSION 3.27)

include(${CMAKE_SOURCE_DIR}/../../cmake/RepoInfo.cmake)

repo_info("${CMAKE_SOURCE_DIR}/../..")

project(sqsgenerator-wasm
        VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
        LANGUAGES CXX)


set(VCPKG_C_FLAGS "-fPIC -fno-threadsafe-statics" CACHE STRING "")
set(VCPKG_CXX_FLAGS "-fPIC -fno-threadsafe-statics" CACHE STRING "")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
        FMT_USE_CONSTEXPR=0
        FMT_DISABLE_COMPILETIME_FORMAT=1
        "FMT_STRING(str)=str"
)
set_target_properties(core PROPERTIES COMPILE_FLAGS "-fPIC -Os")
set_target_properties(core PROPERTIES LINK_FLAGS "-O3 --emit-tsd core.d.ts -s EXPORT_ES6=1 -WASM=2 -fwasm-exceptions -sPROXY_TO_PTHREAD -sEXIT_RUNTIME=0 -sASSERTIONS=2 -s MODULARIZE=1 --bind -fPIC -pthread --closure 1 -EXPORT_NAME=core")
target_link_libraries(core PRIVATE nlohmann_json::nlohmann_json Eigen3::Eigen Boost::multiprecision embind)
