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

option(BUILD_CLI "Build the native debug/run CLI (needs the 'cli' vcpkg feature)" OFF)
option(BUILD_MPI "Build the CLI with distributed (MPI) support (C API only)" OFF)

# MPI is not available for Emscripten/WASM: never build the two together.
if (BUILD_WASM AND BUILD_MPI)
  message(FATAL_ERROR
          "BUILD_WASM and BUILD_MPI are mutually exclusive: MPI is not supported for WASM/Emscripten builds")
endif ()

# The CLI is the only target allowed to build with MPI; MPI has no standalone target.
if (BUILD_MPI AND NOT BUILD_CLI)
  message(FATAL_ERROR "BUILD_MPI requires BUILD_CLI: the CLI is the only MPI-capable target")
endif ()

find_package(glaze CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(unordered_dense CONFIG REQUIRED)
find_package(boost_multiprecision REQUIRED CONFIG)
find_path(BS_THREAD_POOL_INCLUDE_DIRS "BS_thread_pool.hpp")
include_directories("${CMAKE_SOURCE_DIR}/include" ${BS_THREAD_POOL_INCLUDE_DIRS})

if (BUILD_WASM)
  message("Building WASM module")
  find_package(absl CONFIG REQUIRED)
  add_executable(wasm
    wasm/parser.cpp
    src/configuration.cpp
    src/configuration/misc.cpp
    src/configuration/arrays.cpp
    src/configuration/structure.cpp
    src/configuration/composition.cpp
    src/configuration/shell_radii.cpp
    src/configuration/shell_weights.cpp
  )
  set_target_properties(wasm PROPERTIES
          SUFFIX ".mjs"
  )
  target_link_libraries(wasm PRIVATE
          absl::hash
          absl::log
          absl::str_format
          Eigen3::Eigen
          Boost::multiprecision
          embind
          glaze::glaze
              unordered_dense::unordered_dense
  )
else ()
  message("Building native modules")
  find_package(absl CONFIG REQUIRED)
  add_executable(parse
    cli/parse.cpp
  )
  target_link_libraries(parse PRIVATE
          absl::hash
          absl::log
          absl::str_format
          Eigen3::Eigen
          Boost::multiprecision
          glaze::glaze
          unordered_dense::unordered_dense
  )
  target_compile_options(parse PRIVATE -Wall -Wextra)
endif ()

if (BUILD_CLI)
  message("Building CLI")
  find_package(argparse CONFIG REQUIRED)
  add_executable(sqsgen
    src/main.cpp
  )
  target_link_libraries(sqsgen PRIVATE
          absl::hash
          absl::log
          absl::str_format
          argparse::argparse
          Eigen3::Eigen
          Boost::multiprecision
          glaze::glaze
          unordered_dense::unordered_dense
  )
  target_compile_options(sqsgen PRIVATE -Wall -Wextra)

  # The CLI is the only MPI-capable target: link MPI into it instead of a separate executable.
  if (BUILD_MPI)
    message("  with MPI support")
    set(MPI_CXX_SKIP_MPICXX ON)
    find_package(MPI REQUIRED)
    target_link_libraries(sqsgen PRIVATE MPI::MPI_CXX)
    target_compile_definitions(sqsgen PRIVATE SQSGEN_WITH_MPI)
  endif ()
endif ()

option(BUILD_PYTHON "Build the Python extension module (nanobind, stable ABI >= 3.12)" OFF)
if (BUILD_PYTHON)
  # nanobind ships inside the active virtualenv (installed with pip), not vcpkg.
  # Ask the interpreter where its CMake package lives and target the limited API:
  # a STABLE_ABI module pins Py_LIMITED_API to 0x030C0000, so one .abi3 build runs
  # on CPython 3.12 and newer. Support for Python < 3.12 is dropped deliberately.
  find_package(Python 3.12 REQUIRED
          COMPONENTS Interpreter Development.Module Development.SABIModule)
  execute_process(
          COMMAND "${Python_EXECUTABLE}" -c "import nanobind; print(nanobind.cmake_dir())"
          OUTPUT_VARIABLE nanobind_ROOT OUTPUT_STRIP_TRAILING_WHITESPACE
          COMMAND_ERROR_IS_FATAL ANY)
  find_package(nanobind CONFIG REQUIRED)

  nanobind_add_module(_core STABLE_ABI NB_STATIC src/python.cpp)
  target_link_libraries(_core PRIVATE Eigen3::Eigen Boost::multiprecision)
  target_compile_features(_core PRIVATE cxx_std_23)
endif ()

option(BUILD_TESTS "Build the unit test suite" OFF)
if (BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif ()
