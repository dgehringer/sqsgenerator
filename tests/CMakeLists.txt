set(SQSGEN_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(SQSGEN_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include/sqsgen)
set(SQSGEN_TEST_SOURCE_DIR ${PROJECT_SOURCE_DIR}/tests)

find_package(GTest CONFIG REQUIRED)
file(GLOB tests "${SQSGEN_TEST_SOURCE_DIR}/*.cpp")
list(REMOVE_ITEM tests "${SQSGEN_TEST_SOURCE_DIR}/main.cpp")


include_directories(${SQSGEN_INCLUDE_DIR})

list(APPEND SQSGEN_TEST_LIBS "mp++")
list(APPEND SQSGEN_TEST_LIBS "Eigen3::Eigen")
list(APPEND SQSGEN_TEST_LIBS "pybind11::embed")
list(APPEND SQSGEN_TEST_LIBS "nlohmann_json::nlohmann_json")
list(APPEND SQSGEN_TEST_LIBS "spdlog::spdlog_header_only")
list(APPEND SQSGEN_TEST_LIBS "GTest::gtest_main")
list(APPEND SQSGEN_TEST_LIBS "${Python3_LIBRARIES}")

message(STATUS "SQSGEN_TEST_LIBS=${SQSGEN_TEST_LIBS}")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fexperimental-library)
endif ()

if (MSVC)
    add_compile_options(/permissive-)
endif ()


enable_testing()

add_executable(test_permutation
        "${SQSGEN_TEST_SOURCE_DIR}/main.cpp"
        "${SQSGEN_TEST_SOURCE_DIR}/test_permutation.cpp"
)
target_link_libraries(test_permutation ${SQSGEN_TEST_LIBS})
add_test(NAME test_permutation COMMAND test_permutation)

add_executable(test_structure
        "${SQSGEN_TEST_SOURCE_DIR}/main.cpp"
        "${SQSGEN_TEST_SOURCE_DIR}/test_structure.cpp"
        helpers.h
)
target_link_libraries(test_structure ${SQSGEN_TEST_LIBS})
add_test(NAME test_structure COMMAND test_structure)
add_custom_command(TARGET test_structure POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SQSGEN_TEST_SOURCE_DIR}/assets $<TARGET_FILE_DIR:test_structure>/assets)


add_executable(test_shuffle
        "${SQSGEN_TEST_SOURCE_DIR}/main.cpp"
        "${SQSGEN_TEST_SOURCE_DIR}/test_shuffle.cpp"
        helpers.h
)
target_link_libraries(test_shuffle ${SQSGEN_TEST_LIBS})
add_test(NAME test_shuffle COMMAND test_shuffle)


add_executable(test_parser
        "${SQSGEN_TEST_SOURCE_DIR}/main.cpp"
        "${SQSGEN_TEST_SOURCE_DIR}/test_parser.cpp"
)
target_include_directories(test_parser PRIVATE ${RAPIDHASH_INCLUDE_DIRS})
target_link_libraries(test_parser ${SQSGEN_TEST_LIBS} "${Python3_LIBRARIES}")
add_test(NAME test_parser COMMAND test_parser)
