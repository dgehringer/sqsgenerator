add_compile_options("-fvisibility=hidden")

if (DEFINED SQSGEN_MAJOR_VERSION)
    add_compile_options("-DSQSGEN_MAJOR_VERSION=${SQSGEN_MAJOR_VERSION}")
endif ()

if (DEFINED SQSGEN_MINOR_VERSION)
    add_compile_options("-DSQSGEN_MINOR_VERSION=${SQSGEN_MINOR_VERSION}")
endif ()

if (DEFINED SQSGEN_BUILD_NUMBER)
    add_compile_options("-DSQSGEN_BUILD_NUMBER=${SQSGEN_BUILD_NUMBER}")
endif ()

if (DEFINED SQSGEN_BUILD_BRANCH)
    add_compile_options("-DSQSGEN_BUILD_BRANCH=${SQSGEN_BUILD_BRANCH}")
endif ()

if (DEFINED SQSGEN_BUILD_COMMIT)
    add_compile_options("-DSQSGEN_BUILD_COMMIT=${SQSGEN_BUILD_COMMIT}")
endif ()

find_path(RAPIDHASH_INCLUDE_DIRS "rapidhash.h")
find_path(BS_THREAD_POOL_INCLUDE_DIRS "BS_thread_pool.hpp")


pybind11_add_module(_core sqsgen.cpp utils.h)
target_include_directories(_core PRIVATE ${BS_THREAD_POOL_INCLUDE_DIRS})
target_include_directories(_core PRIVATE ${RAPIDHASH_INCLUDE_DIRS})
target_link_libraries(_core PRIVATE nlohmann_json::nlohmann_json spdlog::spdlog_header_only Boost::multiprecision Eigen3::Eigen ${Python3_LIBRARIES})
