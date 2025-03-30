//
// Created by Dominik Gehringer on 11.11.24.
//

#ifndef SQSGEN_CORE_HELPERS_STATIC_STRING_H
#define SQSGEN_CORE_HELPERS_STATIC_STRING_H

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

    constexpr std::basic_string_view<char> operator()() const { return {data, N}; }

    template <std::size_t M>
    constexpr string_literal<N + M - 1> operator+(const string_literal<M>& rhs) const {
      char new_data[N + M - 1];
      std::copy_n(data, N - 1, new_data);
      std::copy_n(rhs.data, M, new_data + N - 1);
      return string_literal<N + M - 1>{new_data};
    }

    [[nodiscard]] constexpr const char* c_str() const { return data; }

    char data[N];
  };

}  // namespace sqsgen::core::helpers

#endif  // SQSGEN_CORE_HELPERS_STATIC_STRING_H
