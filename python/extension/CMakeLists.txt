

pybind11_add_module(_sqsgen sqsgen.cpp)
target_link_libraries(_sqsgen PUBLIC ${SQSGEN_LIBS})