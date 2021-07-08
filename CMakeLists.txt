cmake_minimum_required(VERSION 3.12)
project(sqsgenerator)
# set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

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
        sqsgenerator/core/include/types.hpp
        sqsgenerator/core/include/rank.hpp
        sqsgenerator/core/src/rank.cpp
        sqsgenerator/core/include/moodycamel/concurrentqueue.h
        sqsgenerator/core/include/moodycamel/blockingconcurrentqueue.h
        sqsgenerator/core/include/moodycamel/lightweightsemaphore.h
        sqsgenerator/core/src/containers.cpp
        sqsgenerator/core/include/containers.hpp
        sqsgenerator/core/include/utils.hpp
        sqsgenerator/core/src/atomistics.cpp
        sqsgenerator/core/include/atomistics.hpp
        sqsgenerator/core/include/structure_utils.hpp
        sqsgenerator/core/include/sqs.hpp sqsgenerator/core/include/settings.h)


target_link_libraries(sqsgenerator.core pthread ${Boost_LIBRARIES})
