cmake_minimum_required(VERSION 3.16)
project(sqsgenerator)
# set(CMAKE_CXX_COMPILER /usr/bin/g++)


include_directories(sqsgenerator/core/include)

add_subdirectory(test)

add_executable(sqsgenerator.core
        sqsgenerator/core/include/rigtorp/MPMCQueue.h
        sqsgenerator/core/include/types.h
        sqsgenerator/core/include/rank.h
        sqsgenerator/core/src/rank.cpp
        sqsgenerator/core/include/xoroshiro256.h
        sqsgenerator/core/src/xoroshiro256.cpp
        sqsgenerator/core/include/utils.h
        sqsgenerator/core/src/utils.cpp
        sqsgenerator/core/src/main.cpp sqsgenerator/core/include/moodycamel/concurrentqueue.h sqsgenerator/core/include/moodycamel/blockingconcurrentqueue.h sqsgenerator/core/include/moodycamel/lightweightsemaphore.h)


target_link_libraries(sqsgenerator.core pthread gmp gmpxx)
