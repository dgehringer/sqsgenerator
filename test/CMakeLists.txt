cmake_minimum_required(VERSION 3.12)
project(sqsgenerator.core.test)
set(SQSGENERATOR_SRC_DIR ../sqsgenerator/core/src)
set(SQSGENERATOR_INC_DIR ../sqsgenerator/core/include)

find_package(GTest REQUIRED)
set(CMAKE_CXX_COMPILER g++)

include_directories(${SQSGENERATOR_INC_DIR})
include_directories(include)

include(GoogleTest)
enable_testing()
add_executable(sqsgenerator.core.test.rank core/src/test_rank.cpp ${SQSGENERATOR_SRC_DIR}/rank.cpp)
target_link_libraries(sqsgenerator.core.test.rank gtest gtest_main pthread)


gtest_add_tests(TARGET      sqsgenerator.core.test.rank
                TEST_SUFFIX .noArgs
                TEST_LIST   noArgsTests)


add_executable(sqsgenerator.core.test.atomistics core/src/test_atomistics.cpp ${SQSGENERATOR_SRC_DIR}/atomistics.cpp)
target_link_libraries(sqsgenerator.core.test.atomistics gtest gtest_main pthread)
gtest_add_tests(TARGET      sqsgenerator.core.test.atomistics
                TEST_SUFFIX .noArgs
                TEST_LIST   noArgsTests)