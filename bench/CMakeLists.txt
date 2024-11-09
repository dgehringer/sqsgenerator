
set(SQSGEN_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(SQSGEN_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include/sqsgen)
set(SQSGEN_BENCH_SOURCE_DIR ${PROJECT_SOURCE_DIR}/bench)

CPMAddPackage("gh:martinus/nanobench@4.3.11")

list(APPEND SQSGEN_BENCH_LIBS "mp++")
list(APPEND SQSGEN_BENCH_LIBS "Eigen3::Eigen")
list(APPEND SQSGEN_BENCH_LIBS "nlohmann_json")
list(APPEND SQSGEN_BENCH_LIBS "nanobench")

include_directories(${SQSGEN_INCLUDE_DIR})
add_executable(bench_clear_array "${SQSGEN_BENCH_SOURCE_DIR}/clear_array.cpp")
target_link_libraries(bench_clear_array  ${SQSGEN_BENCH_LIBS})

include_directories(${SQSGEN_INCLUDE_DIR})
add_executable(bench_count_bonds "${SQSGEN_BENCH_SOURCE_DIR}/count_bonds.cpp")
target_link_libraries(bench_count_bonds  ${SQSGEN_BENCH_LIBS})
