//
// Created by Dominik Gehringer on 11.11.24.
//

#ifndef SQSGEN_CORE_HELPER_STATIC_STRING_H
#define SQSGEN_CORE_HELPER_STATIC_STRING_H

#include <array>
namespace sqsgen::core::helpers {

  template<size_t N>
  struct string_literal {
    constexpr string_literal(const char (&str)[N]) {
      std::copy_n(str, N, data);
    }

    char data[N];
  };
}  // namespace sqsgen::core::helpers

#endif  // SQSGEN_CORE_HELPER_STATIC_STRING_H
