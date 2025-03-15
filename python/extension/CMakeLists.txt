
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

pybind11_add_module(_core sqsgen.cpp utils.h)
target_link_libraries(_core PUBLIC ${SQSGEN_LIBS})