
#ifndef SQSGEN_CORE_HELPERS_INST_H
#define SQSGEN_CORE_HELPERS_INST_H

// --- single-axis "for each value" macros (forward accumulated args) ---
#define SQSGEN_FOR_PREC(M, ...) M(__VA_ARGS__, float) M(__VA_ARGS__, double)

#define SQSGEN_FOR_SMODE(M, ...)                    \
  M(__VA_ARGS__, sqsgen::sublattice_mode::interact) \
  M(__VA_ARGS__, sqsgen::sublattice_mode::split)

#define SQSGEN_FOR_AMODE(M, ...)                   \
  M(__VA_ARGS__, sqsgen::core::array_mode::nested) \
  M(__VA_ARGS__, sqsgen::core::array_mode::packed)

// --- flat cartesian-product macros: call with your INST(...) pattern ---

// PREC only
#define SQSGEN_INSTANTIATE_PREC(INST) \
  INST(float)                         \
  INST(double)

// PREC × SMODE
#define SQSGEN_INSTANTIATE_PREC_SMODE(INST)       \
  INST(float, sqsgen::sublattice_mode::interact)  \
  INST(float, sqsgen::sublattice_mode::split)     \
  INST(double, sqsgen::sublattice_mode::interact) \
  INST(double, sqsgen::sublattice_mode::split)

// PREC × AMODE
#define SQSGEN_INSTANTIATE_PREC_AMODE(INST)      \
  INST(float, sqsgen::core::array_mode::nested)  \
  INST(float, sqsgen::core::array_mode::packed)  \
  INST(double, sqsgen::core::array_mode::nested) \
  INST(double, sqsgen::core::array_mode::packed)

// PREC × SMODE × AMODE
#define SQSGEN_INSTANTIATE_PREC_SMODE_AMODE(INST)                                   \
  INST(float, sqsgen::sublattice_mode::interact, sqsgen::core::array_mode::nested)  \
  INST(float, sqsgen::sublattice_mode::interact, sqsgen::core::array_mode::packed)  \
  INST(float, sqsgen::sublattice_mode::split, sqsgen::core::array_mode::nested)     \
  INST(float, sqsgen::sublattice_mode::split, sqsgen::core::array_mode::packed)     \
  INST(double, sqsgen::sublattice_mode::interact, sqsgen::core::array_mode::nested) \
  INST(double, sqsgen::sublattice_mode::interact, sqsgen::core::array_mode::packed) \
  INST(double, sqsgen::sublattice_mode::split, sqsgen::core::array_mode::nested)    \
  INST(double, sqsgen::sublattice_mode::split, sqsgen::core::array_mode::packed)

#endif
