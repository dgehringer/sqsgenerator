cmake_minimum_required(VERSION 3.12)
project(sqsgenerator)
# set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(sqsgenerator/core/include)

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


target_link_libraries(sqsgenerator.core pthread gmp gmpxx)
