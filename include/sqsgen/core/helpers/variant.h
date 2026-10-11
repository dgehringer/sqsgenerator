
#ifndef SQSGEN_CORE_HELPERS_VARIANT_H
#define SQSGEN_CORE_HELPERS_VARIANT_H

namespace sqsgen::core::helpers {
  template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
  };

};  // namespace sqsgen::core::helpers

#endif  // SQSGEN_CORE_HELPERS_VARIANT_H
