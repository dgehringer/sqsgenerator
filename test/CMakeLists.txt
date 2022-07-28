cmake_minimum_required(VERSION 3.14)
project(sqsgenerator.core.test)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(SQSGENERATOR_SRC_DIR ../sqsgenerator/core/src)
set(SQSGENERATOR_INC_DIR ../sqsgenerator/core/include)



find_package(GTest REQUIRED)
find_package (Python3 COMPONENTS Interpreter Development NumPy)

message([PYTHON]:INCLUDE_DIRS: ${Python3_INCLUDE_DIRS})
message([PYTHON]:LIBRARY_DIRS: ${Python3_LIBRARY_DIRS})
message([PYTHON]:LIBRARIES: ${Python3_LIBRARIES})
message([PYTHON]:EXECUTABLE: ${Python3_EXECUTABLE})
message([PYTHON_NUMPY]: VERSION: ${Python3_NumPy_VERSION})
message([PYTHON_NUMPY]: INCLUDE_DIRS: ${Python3_NumPy_INCLUDE_DIRS})

set(SQSGENERATOR_TEST_LIBRARIES gtest gtest_main pthread ${Boost_LIBRARIES} ${Python3_LIBRARIES} stdc++fs)

include_directories(${SQSGENERATOR_INC_DIR})
include_directories(include)

include(GoogleTest)
enable_testing()
add_executable(test.sqsgenerator.core.rank
        core/src/test_rank.cpp
        ${SQSGENERATOR_SRC_DIR}/rank.cpp
        ${SQSGENERATOR_SRC_DIR}/utils.cpp)
target_link_libraries(test.sqsgenerator.core.rank ${SQSGENERATOR_TEST_LIBRARIES})


gtest_add_tests(TARGET      test.sqsgenerator.core.rank
                TEST_SUFFIX .noArgs
                TEST_LIST   noArgsTests)


add_executable(test.sqsgenerator.core.atomistics
        core/src/test_atomistics.cpp
        ${SQSGENERATOR_SRC_DIR}/utils.cpp
        ${SQSGENERATOR_SRC_DIR}/structure_utils.cpp
        ${SQSGENERATOR_SRC_DIR}/atomistics.cpp)
target_link_libraries(test.sqsgenerator.core.atomistics ${SQSGENERATOR_TEST_LIBRARIES})
gtest_add_tests(TARGET      test.sqsgenerator.core.atomistics
                TEST_SUFFIX .noArgs
                TEST_LIST   noArgsTests)

add_executable(test.sqsgenerator.core.structure_utils
        core/src/test_structure_utils.cpp
        ${SQSGENERATOR_SRC_DIR}/utils.cpp
        ${SQSGENERATOR_SRC_DIR}/structure_utils.cpp)

target_link_libraries(test.sqsgenerator.core.structure_utils ${SQSGENERATOR_TEST_LIBRARIES})
gtest_add_tests(TARGET test.sqsgenerator.core.structure_utils
                TEST_SUFFIX .noArgs
                TEST_LIST   noArgsTests)

add_custom_command(TARGET test.sqsgenerator.core.structure_utils POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/test/resources $<TARGET_FILE_DIR:test.sqsgenerator.core.structure_utils>/resources)
