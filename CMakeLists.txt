cmake_minimum_required(VERSION 3.27)

include(${CMAKE_SOURCE_DIR}/cmake/RepoInfo.cmake)
include(FetchContent)
repo_info(${CMAKE_SOURCE_DIR})

project(sqsgenerator-wasm
        VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(
        SQSGEN_BUILD_COMMIT=${GIT_COMMIT_HASH}
        SQSGEN_BUILD_BRANCH=${GIT_BRANCH}
        SQSGEN_BUILD_NUMBER=${VERSION_PATCH}
        SQSGEN_MINOR_VERSION=${VERSION_MINOR}
        SQSGEN_MAJOR_VERSION=${VERSION_MAJOR}
)

find_package(glaze CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(boost_multiprecision REQUIRED CONFIG)
find_path(BS_THREAD_POOL_INCLUDE_DIRS "BS_thread_pool.hpp")
include_directories("${CMAKE_SOURCE_DIR}/include" ${BS_THREAD_POOL_INCLUDE_DIRS})

if (BUILD_WASM)
  message("Building WASM module")
  find_package(absl CONFIG REQUIRED)
  add_executable(wasm
    wasm/sqsgen.cpp
    src/optimization.cpp
    # src/configuration.cpp
    src/core/permutation.cpp
    src/core/structure.cpp
  )
  set_target_properties(wasm PROPERTIES
          SUFFIX ".mjs"
  )
  target_link_libraries(wasm PRIVATE
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
          glaze::glaze
  )
endif ()
