# Unit tests (Catch2).
#
# Included via add_subdirectory() from the top-level CMakeLists when BUILD_TESTS=ON.
# Configure/build with the dedicated preset:
#
#   cmake --preset native-tests
#   cmake --build build/tests
#   ctest --test-dir build/tests --output-on-failure
#
# The C++ standard, include directories and the vcpkg toolchain are inherited
# from the parent project; do not redefine them here.

find_package(Catch2 3 CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(boost_multiprecision CONFIG REQUIRED)
find_package(unordered_dense CONFIG REQUIRED)
find_package(glaze CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_path(BS_THREAD_POOL_INCLUDE_DIRS "BS_thread_pool.hpp")

include(Catch)

set(SQSGEN_DEP_LIBS
        Eigen3::Eigen
        Boost::multiprecision
        glaze::glaze
        unordered_dense::unordered_dense
)

set(SQSGEN_TEST_LIBS Catch2::Catch2WithMain ${SQSGEN_DEP_LIBS})


function(sqsgen_add_test name)
  add_executable(${name} ${ARGN})
  target_include_directories(${name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(${name} PRIVATE ${SQSGEN_TEST_LIBS})
  target_compile_options(${name} PRIVATE -Wall -Wextra)
  catch_discover_tests(${name})
endfunction()

sqsgen_add_test(test_permutation test_permutation.cpp)

sqsgen_add_test(test_structure test_structure.cpp)
add_custom_command(TARGET test_structure POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:test_structure>/assets)
