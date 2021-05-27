cmake_minimum_required(VERSION 3.19)
project(sqsgenerator.core.test)

set(CMAKE_CXX_COMPILER g++)
set(SQSGENERATOR_SRC_DIR ../sqsgenerator/core/src)
set(SQSGENERATOR_INC_DIR ../sqsgenerator/core/include)

include_directories(${SQSGENERATOR_SRC_DIR})
include_directories(include)

add_executable(sqsgenerator.core.test.rank
        core/src/test_rank.cpp
        ${SQSGENERATOR_SRC_DIR}/rank.cpp)

target_link_libraries(sqsgenerator.core.test.rank gtest gtest_main pthread gmp gmpxx)