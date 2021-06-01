cmake_minimum_required(VERSION 3.12)
project(sqsgenerator)
# set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
find_package(GMP REQUIRED)


message([GMP]:INCLUDE_DIRS: ${GMP_INCLUDE_DIR})
message([GMP]:LIBRARIES: ${GMP_LIB})
message([GMPXX]:INCLUDE_DIRS: ${GMPXX_INCLUDE_DIR})
message([GMPXX]:LIBRARIES: ${GMPXX_LIB})

include_directories(sqsgenerator/core/include)
include_directories(${GMP_INCLUDE_DIR})
include_directories(${GMPXX_INCLUDE_DIR})

add_subdirectory(test)
add_subdirectory(sqsgenerator/core/python)

add_executable(sqsgenerator.core
        sqsgenerator/core/include/types.h
        sqsgenerator/core/include/rank.h
        sqsgenerator/core/src/rank.cpp
        sqsgenerator/core/src/main.cpp
        sqsgenerator/core/include/moodycamel/concurrentqueue.h
        sqsgenerator/core/include/moodycamel/blockingconcurrentqueue.h
        sqsgenerator/core/include/moodycamel/lightweightsemaphore.h
        sqsgenerator/core/src/containers.cpp sqsgenerator/core/src/containers.cpp sqsgenerator/core/include/containers.h sqsgenerator/core/include/utils.h)


target_link_libraries(sqsgenerator.core pthread ${GMP_LIB} ${GMPXX_LIB})
