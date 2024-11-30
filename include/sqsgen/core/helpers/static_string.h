//
// Created by Dominik Gehringer on 11.11.24.
//

#ifndef SQSGEN_CORE_HELPER_STATIC_STRING_H
#define SQSGEN_CORE_HELPER_STATIC_STRING_H

#include <array>
namespace sqsgen::core::helpers {

  template <size_t N> struct string_literal {
    static constexpr size_t length = N;
    constexpr string_literal(const char (&str)[N]) { std::copy_n(str, N, data); }

    template <size_t M> constexpr bool operator==(const string_literal<M>& rhs) const {
      if constexpr (N != M)
        return false;
      else {
        return [&]<std::size_t... Index>(std::index_sequence<Index...>) {
          return ((data[Index] == rhs.data[Index]) && ...);
        }(std::make_index_sequence<length>{});
      }
    }

    char data[N];
  };
}  // namespace sqsgen::core::helpers

#endif  // SQSGEN_CORE_HELPER_STATIC_STRING_H
