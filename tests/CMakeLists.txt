set(SQSGEN_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(SQSGEN_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include/sqsgen)
set(SQSGEN_TEST_SOURCE_DIR ${PROJECT_SOURCE_DIR}/tests)

CPMAddPackage(
        NAME googletest
        GITHUB_REPOSITORY google/googletest
        GIT_TAG v1.15.2
        OPTIONS "INSTALL_GTEST OFF" "gtest_force_shared_crt"
)

file(GLOB tests "${SQSGEN_TEST_SOURCE_DIR}/*.cpp")
list(REMOVE_ITEM tests "${SQSGEN_TEST_SOURCE_DIR}/main.cpp")


include_directories(${SQSGEN_INCLUDE_DIR})

enable_testing()

add_executable(test_permutation
        "${SQSGEN_TEST_SOURCE_DIR}/main.cpp"
        "${SQSGEN_TEST_SOURCE_DIR}/test_permutation.cpp"
        "${SQSGEN_INCLUDE_DIR}/core/helpers.h"
        "${SQSGEN_INCLUDE_DIR}/types.h"
        "${SQSGEN_INCLUDE_DIR}/core/permutation.h"
        "${SQSGEN_SOURCE_DIR}/core/permutation.cpp"
        test_structure.cpp
)
target_link_libraries(test_permutation mp++ gtest_main Eigen3::Eigen)
add_test(NAME test_permutation COMMAND test_permutation)

add_executable(test_structure
        "${SQSGEN_TEST_SOURCE_DIR}/main.cpp"
        "${SQSGEN_TEST_SOURCE_DIR}/test_structure.cpp"
        "${SQSGEN_INCLUDE_DIR}/core/helpers.h"
        "${SQSGEN_INCLUDE_DIR}/types.h"
        "${SQSGEN_INCLUDE_DIR}/core/permutation.h"
        "${SQSGEN_SOURCE_DIR}/core/permutation.cpp"
)
target_link_libraries(test_permutation mp++ gtest_main Eigen3::Eigen)
add_test(NAME test_structure COMMAND test_structure)


