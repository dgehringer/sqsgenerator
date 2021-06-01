cmake_minimum_required(VERSION 3.12)
project(sqsgenerator)
# set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(Boost REQUIRED COMPONENTS system python numpy)

message([BOOST]:INCLUDE_DIRS: ${Boost_INCLUDE_DIRS})
message([BOOST]:LIBRARIES: ${Boost_LIBRARIES})


include_directories(sqsgenerator/core/include)
include_directories(${Boost_INCLUDE_DIRS})

add_subdirectory(test)
add_subdirectory(sqsgenerator/core/python)

add_executable(sqsgenerator.core
        sqsgenerator/core/include/types.h
        sqsgenerator/core/include/rank.h
        sqsgenerator/core/src/rank.cpp
        sqsgenerator/core/include/moodycamel/concurrentqueue.h
        sqsgenerator/core/include/moodycamel/blockingconcurrentqueue.h
        sqsgenerator/core/include/moodycamel/lightweightsemaphore.h
        sqsgenerator/core/src/containers.cpp
        sqsgenerator/core/include/containers.h
        sqsgenerator/core/include/utils.h
        sqsgenerator/core/src/main.cpp)


target_link_libraries(sqsgenerator.core pthread ${Boost_LIBRARIES})
