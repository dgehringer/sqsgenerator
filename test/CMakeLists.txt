cmake_minimum_required(VERSION 3.12)
project(sqsgenerator.core.test)
set(SQSGENERATOR_SRC_DIR ../sqsgenerator/core/src)
set(SQSGENERATOR_INC_DIR ../sqsgenerator/core/include)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
set(CMAKE_CXX_COMPILER g++)



include_directories(${SQSGENERATOR_INC_DIR})
include_directories(include)

include(GoogleTest)
add_executable(sqsgenerator.core.test.rank
        core/src/test_rank.cpp
        ${SQSGENERATOR_SRC_DIR}/rank.cpp)

target_link_libraries(sqsgenerator.core.test.rank gtest gtest_main pthread)


gtest_add_tests(TARGET      sqsgenerator.core.test.rank
                TEST_SUFFIX .noArgs
                TEST_LIST   noArgsTests)

