//
// Created by Dominik Gehringer on 24.10.24.
//

#ifndef SQSGEN_CORE_HELPERS_NUMERIC_H
#define SQSGEN_CORE_HELPERS_NUMERIC_H

namespace sqsgen::core::helpers {
  namespace ranges = std::ranges;
  namespace views = std::ranges::views;
  template <class T> bool is_close(T a, T b, T atol = 1.0e-5, T rtol = 1.0e-3) {
    return std::abs(a - b) <= (atol + rtol * std::abs(b));
  }

  template <class T> T absolute(T a) { return a > 0 ? a : -a; }

  template <typename Out, typename In>
    requires std::is_convertible_v<In, Out>
  Out factorial(In n) {
    if (n < 0) throw std::invalid_argument("n must be positive");
    if (n == 1) return 1;
    return helpers::fold_left(views::iota(In(1), In(n + 1)), Out(1),
                              [](auto a, auto b) { return a * b; });
  }
}  // namespace sqsgen::core::helpers

#endif  // SQSGEN_CORE_HELPERS_NUMERIC_H
