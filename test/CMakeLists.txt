cmake_minimum_required(VERSION 3.12)
project(sqsgenerator.core.test)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(SQSGENERATOR_SRC_DIR ../sqsgenerator/core/src)
set(SQSGENERATOR_INC_DIR ../sqsgenerator/core/include)
set(SQSGENERATOR_TEST_LIBRARIES gtest gtest_main pthread ${Boost_LIBRARIES})

find_package(GTest REQUIRED)


include_directories(${SQSGENERATOR_INC_DIR})
include_directories(include)

include(GoogleTest)
enable_testing()
add_executable(sqsgenerator.core.test.rank
        core/src/test_rank.cpp
        ${SQSGENERATOR_SRC_DIR}/rank.cpp
        ${SQSGENERATOR_SRC_DIR}/utils.cpp)
target_link_libraries(sqsgenerator.core.test.rank ${SQSGENERATOR_TEST_LIBRARIES})


gtest_add_tests(TARGET      sqsgenerator.core.test.rank
                TEST_SUFFIX .noArgs
                TEST_LIST   noArgsTests)


add_executable(sqsgenerator.core.test.atomistics
        core/src/test_atomistics.cpp
        ${SQSGENERATOR_SRC_DIR}/structure_utils.cpp
        ${SQSGENERATOR_SRC_DIR}/atomistics.cpp)
target_link_libraries(sqsgenerator.core.test.atomistics ${SQSGENERATOR_TEST_LIBRARIES})
gtest_add_tests(TARGET      sqsgenerator.core.test.atomistics
                TEST_SUFFIX .noArgs
                TEST_LIST   noArgsTests)

add_executable(sqsgenerator.core.test.structure_utils
        core/src/test_structure_utils.cpp
        ${SQSGENERATOR_SRC_DIR}/structure_utils.cpp)
target_link_libraries(sqsgenerator.core.test.structure_utils ${SQSGENERATOR_TEST_LIBRARIES} stdc++fs)
add_custom_command(TARGET sqsgenerator.core.test.structure_utils POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/test/resources $<TARGET_FILE_DIR:sqsgenerator.core.test.structure_utils>/resources)

gtest_add_tests(TARGET sqsgenerator.core.test.structure_utils
                TEST_SUFFIX .noArgs
                TEST_LIST   noArgsTests)
