cmake_minimum_required(VERSION 3.14)
project(python_bindings VERSION ${sqsgenerator_VERSION_MAJOR}.${sqsgenerator_VERSION_MINOR})

find_package (Python3 COMPONENTS Interpreter Development NumPy)

message([PYTHON]:INCLUDE_DIRS: ${Python3_INCLUDE_DIRS})
message([PYTHON]:LIBRARY_DIRS: ${Python3_LIBRARY_DIRS})
message([PYTHON]:LIBRARIES: ${Python3_LIBRARIES})
message([PYTHON]:EXECUTABLE: ${Python3_EXECUTABLE})
message([PYTHON_NUMPY]: VERSION: ${Python3_NumPy_VERSION})
message([PYTHON_NUMPY]: INCLUDE_DIRS: ${Python3_NumPy_INCLUDE_DIRS})

if(USE_MPI)
    if (MPI_FOUND)
        set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
    endif()
endif()

include_directories(${SQSGENERATOR_INCLUDE_DIR} include)
include_directories(${Python3_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Python3_LIBRARY_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

# https://stackoverflow.com/questions/28887680/linking-boost-library-with-boost-use-static-lib-off-on-windows
if (WIN32)

  # add permissive flags
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")

  include(FetchContent)
  # on Windows MSVC compiler is missing 128 unsigned integers, we therefore use https://github.com/calccrypto/uint128_t
  FetchContent_Declare(
     uint128t
     GIT_REPOSITORY https://github.com/calccrypto/uint128_t.git
     GIT_TAG ddc03839ea7efb78fa504ae8ecc2304e5ba80c3c
  )

  FetchContent_MakeAvailable(uint128t)

  include_directories(${FETCHCONTENT_BASE_DIR}/uint128t-src)

  add_library(core SHARED
        module_core.cpp
        data.cpp
        iteration.cpp
        helpers.cpp
        ${SQSGENERATOR_SOURCE_DIR}/sqs.cpp
        ${SQSGENERATOR_SOURCE_DIR}/settings.cpp
        ${SQSGENERATOR_SOURCE_DIR}/atomistics.cpp
        ${SQSGENERATOR_SOURCE_DIR}/result.cpp
        ${SQSGENERATOR_SOURCE_DIR}/rank.cpp
        ${SQSGENERATOR_SOURCE_DIR}/utils.cpp
        ${SQSGENERATOR_SOURCE_DIR}/structure_utils.cpp
        ${FETCHCONTENT_BASE_DIR}/uint128t-src/uint128_t.cpp
  )

else()
    add_library(core SHARED
        module_core.cpp
        data.cpp
        iteration.cpp
        helpers.cpp
        ${SQSGENERATOR_SOURCE_DIR}/sqs.cpp
        ${SQSGENERATOR_SOURCE_DIR}/settings.cpp
        ${SQSGENERATOR_SOURCE_DIR}/atomistics.cpp
        ${SQSGENERATOR_SOURCE_DIR}/result.cpp
        ${SQSGENERATOR_SOURCE_DIR}/rank.cpp
        ${SQSGENERATOR_SOURCE_DIR}/utils.cpp
        ${SQSGENERATOR_SOURCE_DIR}/structure_utils.cpp
    )
endif()


if (WIN32)
  set_target_properties(core PROPERTIES SUFFIX .pyd)
  target_link_libraries(core ${Python3_LIBRARIES} ${Boost_LIBRARIES})
else()
    set_target_properties(core PROPERTIES SUFFIX .so)
    target_link_libraries(core pthread ${Python3_LIBRARIES} ${Boost_LIBRARIES})
endif()


set_target_properties(core PROPERTIES PREFIX "")
if (USE_MPI)
    target_link_libraries(core ${MPI_CXX_LIBRARIES})
endif()
