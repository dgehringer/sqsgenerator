//
// Created by Dominik Gehringer on 23.10.24.
//

#ifndef SQSGEN_CORE_HELPERS_TEMPLATES_H
#define SQSGEN_CORE_HELPERS_TEMPLATES_H

#include <ranges>
#include <variant>

#include "sqsgen/core/helpers.h"

namespace sqsgen::core::helpers {
  namespace ranges = std::ranges;
  namespace views = std::ranges::views;

  template <ranges::input_range R, class Fn, class T = ranges::range_value_t<R>,
            ranges::input_range U = std::invoke_result_t<Fn, T>>
  constexpr auto flat_map(R&& r, Fn&& fn) {
    return r | views::transform(std::forward<Fn>(fn)) | views::join;
  }

  template <class... Args, class Fn> constexpr auto map(std::tuple<Args...>&& tuple, Fn&& fn) {
    return [&]<std::size_t... Index>(std::index_sequence<Index...>, Fn&& f) {
      ;
      return std::make_tuple(f(std::forward<std::tuple_element_t<Index, std::tuple<Args...>>>(
          std::get<Index>(tuple)))...);
    }(std::make_index_sequence<sizeof...(Args)>{}, std::forward<Fn>(fn));
  }

  template <class... Args> constexpr auto tail(std::tuple<Args...> const& tup) {
    static_assert(sizeof...(Args) >= 1, "Tail must have at least one element");
    return [&]<std::size_t... Index>(std::index_sequence<Index...>) {
      return std::make_tuple(std::get<Index + 1>(tup)...);
    }(std::make_index_sequence<sizeof...(Args) - 1>{});
  }

  template <std::integral T> constexpr auto range(T end) { return views::iota(0, end); }

  template <std::integral T> constexpr auto range(std::pair<T, T>&& bounds) {
    return views::iota(std::get<0>(bounds), std::get<1>(bounds));
  }

  template <ranges::input_range R> constexpr auto range(R&& r) { return std::forward<R>(r); }


  template <class Fn, std::size_t I = 0, class... Args, class ...IndexArgs>
  constexpr void for_each(Fn&& fn, Args&&... args) {
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

#endif  // SQSGEN_CORE_HELPERS_TEMPLATES_H
