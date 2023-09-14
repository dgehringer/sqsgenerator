cmake_minimum_required(VERSION 3.14)
project(sqsgenerator VERSION 0.3)

execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE)

# Get the latest commit hash
execute_process(
        COMMAND git rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE)
message([REPO]: ${GIT_COMMIT_HASH}@${GIT_BRANCH})
message([CMAKE_LIBRARY_PATH]: ${CMAKE_LIBRARY_PATH})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(SQSGENERATOR_SOURCE_DIR ${PROJECT_SOURCE_DIR}/sqsgenerator/core/src)
set(SQSGENERATOR_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/sqsgenerator/core/include)

# by default we create a release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

configure_file(${PROJECT_SOURCE_DIR}/sqsgenerator/core/include/version.hpp.in ${PROJECT_SOURCE_DIR}/sqsgenerator/core/include/version.hpp)

find_package(Boost REQUIRED COMPONENTS system python numpy log_setup log)
message([BOOST]:INCLUDE_DIRS: ${Boost_INCLUDE_DIRS})
message([BOOST]:LIBRARIES: ${Boost_LIBRARIES})
message([BOOST]:LIBRARY_DIRS: ${Boost_LIBRARY_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
set(CMAKE_CXX_LIBRARIES ${Boost_LIBRARIES})


if(NOT Boost_USE_STATIC_LIBS)
    # There is an issue find FindBoost.cmake
    # https://gitlab.kitware.com/cmake/cmake/-/issues/20638: That's why log setup is added
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_LOG_DYN_LINK")
else()
    message([BOOST]:USING_STATIC_LIBS=ON)
endif()

# OpenMP flags
find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_CXX_LIBRARIES "${CMAKE_CXX_LIBRARIES} ${OpenMP_CXX_LIBRARIES}")
    message ([OPENMP]:COMPILER FLAGS: ${OpenMP_CXX_FLAGS})
    message ([OPENMP]:LIBRARIES: ${OpenMP_CXX_LIBRARIES})
endif()

message([MPI]:USE_MPI: ${USE_MPI})

if (USE_MPI)
    find_package(MPI)
    if (MPI_FOUND)
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MPI_CXX_COMPILE_OPTIONS} -DUSE_MPI")
        set (CMAKE_CXX_LIBRARIES "${CMAKE_CXX_LIBRARIES} ${MPI_CXX_LIBRARIES}")
        message([MPI]:VERSION: ${MPI_CXX_VERSION})
        message([MPI]:COMPILER: ${MPI_CXX_COMPILER})
        message([MPI]:COMPILER FLAGS: ${MPI_CXX_COMPILER_FLAGS})
        message([MPI]:LIBRARIES: ${MPI_CXX_LIBRARIES})
        set(CMAKE_CXX_COMPILER_OLD ${CMAKE_CXX_COMPILER})
        message([MPI]:CHANGED_COMPILER: " from \"" ${CMAKE_CXX_COMPILER_OLD} "\" to \"" ${MPI_CXX_COMPILER} "\"")
        include_directories(SYSTEM ${MPI_INCLUDE_PATH})
    endif()
endif()


######################################################
include_directories(sqsgenerator/core/include)
include_directories(${Boost_INCLUDE_DIRS})

# remove tests for the conda-forge build 
# add_subdirectory(test)
add_subdirectory(sqsgenerator/core/python)

if (MPI_FOUND)
    set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
endif()

message("[CMAKE_CXX_COMPILER]: " ${CMAKE_CXX_COMPILER})
message("[CMAKE_CXX_FLAGS]: " ${CMAKE_CXX_FLAGS})
message("[CMAKE_CXX_LIBRARIES]: " ${CMAKE_CXX_LIBRARIES})

add_executable(sqsgenerator.core
        sqsgenerator/core/src/settings.cpp
        sqsgenerator/core/src/sqs.cpp
        sqsgenerator/core/src/rank.cpp
        sqsgenerator/core/src/utils.cpp
        sqsgenerator/core/src/atomistics.cpp
        sqsgenerator/core/src/result.cpp
        sqsgenerator/core/src/structure_utils.cpp)

target_link_libraries(sqsgenerator.core ${Boost_LIBRARIES})
if (USE_MPI)
    target_link_libraries(sqsgenerator.core ${MPI_CXX_LIBRARIES})
endif()
