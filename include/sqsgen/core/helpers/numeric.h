//
// Created by Dominik Gehringer on 24.10.24.
//

#ifndef SQSGEN_CORE_HELPERS_NUMERIC_H
#define SQSGEN_CORE_HELPERS_NUMERIC_H

#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace sqsgen::core::helpers {

  template <class T> bool is_close(T a, T b, T atol = 1.0e-5, T rtol = 1.0e-3) {
    return std::abs(a - b) <= (atol + rtol * std::abs(b));
  }

  template <class T> T absolute(T a) { return a > 0 ? a : -a; }

  template <typename Out, typename In>
    requires std::is_convertible_v<In, Out>
  Out factorial(In n) {
    if (n < 0) throw std::invalid_argument("n must be positive");
    if (n == 1) return 1;
    Out result = 1;
    In i = 1;
    while (i <= n) {
      result *= i;
      ++i;
    }
    return result;
  }
}  // namespace sqsgen::core::helpers

#endif  // SQSGEN_CORE_HELPERS_NUMERIC_H
