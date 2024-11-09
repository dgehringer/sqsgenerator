//
// Created by Dominik Gehringer on 24.10.24.
//

#ifndef SQSGEN_CORE_HELPERS_FOR_EACH_H
#define SQSGEN_CORE_HELPERS_FOR_EACH_H

#include <ranges>

namespace sqsgen::core::helpers {
  namespace ranges = std::ranges;
  namespace views = std::ranges::views;

  template <std::integral T> constexpr auto range(T end) { return views::iota(T(0), end); }

  template <std::integral T> constexpr auto range(std::pair<T, T>&& bounds) {
    return views::iota(std::get<0>(bounds), std::get<1>(bounds));
  }

  template <ranges::input_range R> constexpr auto range(R&& r) { return std::forward<R>(r); }

  template <class U, ranges::input_range R> constexpr auto enumerate(R&& r) {
    return views::zip(views::iota(U(0)), r);
  }

  template <template <class...> class> struct as {};

  template <> struct as<std::vector> {
    template <ranges::range R>
    static std::vector<std::decay_t<ranges::range_value_t<R>>> operator()(R&& r) {
      return {std::begin(r), std::end(r)};
    }
  };

  template <> struct as<std::unordered_set> {
    template <ranges::range R>
    static std::unordered_set<std::decay_t<ranges::range_value_t<R>>> operator()(R&& r) {
      return {std::begin(r), std::end(r)};
    }
  };

  template <> struct as<std::unordered_map> {

    template <ranges::range R>
    static auto operator()(R&& r) {
      using element_t = ranges::range_value_t<R>;
      using key_t = std::tuple_element_t<0, element_t>;
      using value_t = std::tuple_element_t<1, element_t>;
      using return_t = std::unordered_map<key_t, value_t>;
      return_t result{};
      for (const auto& [key, value] : r) result[key] = value;;
      return result;
    }
  };

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
