//
// Created by Dominik Gehringer on 24.10.24.
//

#ifndef SQSGEN_CORE_HELPERS_FOR_EACH_H
#define SQSGEN_CORE_HELPERS_FOR_EACH_H

#include <ranges>

namespace sqsgen::core::helpers {
  namespace ranges = std::ranges;
  namespace views = std::ranges::views;

  template <ranges::range R> constexpr auto to_vector(R&& r) {
    using elem_t = std::decay_t<ranges::range_value_t<R>>;
    return std::vector<elem_t>{r.begin(), r.end()};
  }

  template <std::integral T> constexpr auto range(T end) { return views::iota(T(0), end); }

  template <std::integral T> constexpr auto range(std::pair<T, T>&& bounds) {
    return views::iota(std::get<0>(bounds), std::get<1>(bounds));
  }

  template <ranges::input_range R> constexpr auto range(R&& r) { return std::forward<R>(r); }

  template <ranges::input_range R, class U = std::size_t> constexpr auto enumerate(R&& r) {
    return views::zip(views::iota(U(0)), r);
  }

  template <class Fn, std::size_t I = 0, class... Args> void for_each(Fn&& fn, Args&&... args) {
    static_assert(I < sizeof...(Args), "For each must be at least one element");
    auto arg = std::get<I>(std::forward_as_tuple(std::forward<Args>(args)...));
    auto rng = range(std::forward<decltype(arg)>(arg));
    for (auto v : rng) {
      if constexpr (I == sizeof...(Args) - 1) {
        fn(v);
      } else if constexpr (I < sizeof...(Args) - 1) {
        auto curried = [v, &fn](auto... a) { fn(v, a...); };
        for_each<decltype(curried), I + 1>(std::forward<decltype(curried)>(curried),
                                           std::forward<Args>(args)...);
      }
    }
  }
}  // namespace sqsgen::core::helpers

#endif  // SQSGEN_CORE_HELPERS_FOR_EACH_H
