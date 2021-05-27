cmake_minimum_required(VERSION 3.19)
project(sqsgenerator)
# set(CMAKE_CXX_COMPILER /usr/bin/g++)


include_directories(sqsgenerator/core/include)

add_subdirectory(test)

add_executable(sqsgenerator.core
        sqsgenerator/core/include/rank.h
        sqsgenerator/core/include/types.h
        sqsgenerator/core/include/rigtorp/MPMCQueue.h
        sqsgenerator/core/src/rank.cpp
        sqsgenerator/core/src/main.cpp)


target_link_libraries(sqsgenerator.core pthread gmp gmpxx)
