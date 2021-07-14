cmake_minimum_required(VERSION 3.12)
project(sqsgenerator)
# set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package(Boost REQUIRED COMPONENTS system python numpy)


message([BOOST]:INCLUDE_DIRS: ${Boost_INCLUDE_DIRS})
message([BOOST]:LIBRARIES: ${Boost_LIBRARIES})

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    message([OPENMP]:COMPILER FLAGS: ${OpenMP_CXX_FLAGS})
    message([OPENMP]:LINKER FLAGS: ${OpenMP_EXE_LINKER_FLAGS})
endif()

include_directories(sqsgenerator/core/include)
include_directories(${Boost_INCLUDE_DIRS})

add_subdirectory(test)
add_subdirectory(sqsgenerator/core/python)

add_executable(sqsgenerator.core
        sqsgenerator/core/src/main.cpp
        sqsgenerator/core/src/rank.cpp
        sqsgenerator/core/src/utils.cpp
        sqsgenerator/core/src/atomistics.cpp
        sqsgenerator/core/src/settings.cpp
        sqsgenerator/core/src/result.cpp)


target_link_libraries(sqsgenerator.core pthread ${Boost_LIBRARIES})
