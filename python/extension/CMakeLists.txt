

pybind11_add_module(_core sqsgen.cpp utils.h)
target_link_libraries(_core PUBLIC ${SQSGEN_LIBS})