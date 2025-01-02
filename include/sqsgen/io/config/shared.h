//
// Created by Dominik Gehringer on 02.01.25.
//

#ifndef SQSGEN_IO_CONFIG_SHARED_H
#define SQSGEN_IO_CONFIG_SHARED_H

#include "sqsgen/io/parsing.h"

namespace sqsgen::io::config {

  namespace detail {

    template <class Fn, class... Ranges> struct parse_result_from_fn {
      using type
          = std::decay_t<std::invoke_result_t<Fn, std::decay_t<ranges::range_value_t<Ranges>>...>>;
    };

    template <class> struct parse_result_unwrap {};

    template <class... Args>
      requires io::detail::has_args<1, Args...>::value
    struct parse_result_unwrap<parse_result<Args...>> {
      using type = std::tuple_element_t<0, std::tuple<Args...>>;
    };

    template <class Fn, class... Ranges> struct parse_result_inner_type_from_fn {
      using type =
          typename parse_result_unwrap<typename parse_result_from_fn<Fn, Ranges...>::type>::type;
    };
  }  // namespace detail

  template <string_literal key, class Fn, class... Ranges>
  parse_result<std::vector<typename detail::parse_result_inner_type_from_fn<Fn, Ranges...>::type>>
  lift(Fn&& fn, Ranges&&... ranges) {
    using inner_t = typename detail::parse_result_inner_type_from_fn<Fn, Ranges...>::type;
    auto arglen = ranges::size(std::get<0>(std::forward_as_tuple(ranges...)));
    bool all_same_size = ((ranges::size(ranges) == arglen) && ...);
    if (!all_same_size)
      return parse_error::from_msg<key, CODE_OUT_OF_RANGE>("The sizes do not match");
    auto ptr = std::make_tuple(ranges::begin(ranges)...);
    std::vector<inner_t> results;
    for (auto i = 0u; i < arglen; ++i) {
      auto r = [&]<std::size_t... I>(std::index_sequence<I...>) {
        return fn((*std::get<I>(ptr))...);
      }(std::make_index_sequence<sizeof...(Ranges)>{});
      if (r.failed()) return r.error();
      results.emplace_back(std::move(r.result()));
      [&]<std::size_t... I>(std::index_sequence<I...>) {
        return (ranges::next(std::get<I>(ptr)), ...);
      }(std::make_index_sequence<sizeof...(Ranges)>{});
    }
    return results;
  }

  template <string_literal key, class Document, class InteractFn, class SplitFn>
    requires std::is_same_v<std::invoke_result_t<InteractFn>, std::invoke_result_t<SplitFn>>
  parse_result<typename detail::parse_result_inner_type_from_fn<InteractFn>::type> parse_for_mode(InteractFn&& interact_fn,
                                                                          SplitFn&& split_fn,
                                                                          Document const& doc) {
    using return_t = typename detail::parse_result_inner_type_from_fn<InteractFn>::type;
    return get_optional<"sublattice_mode", SublatticeMode>(doc)
        .value_or(parse_result<SublatticeMode>{SUBLATTICE_MODE_INTERACT})
        .and_then([&](auto&& mode) -> parse_result<return_t> {
          if (mode == SUBLATTICE_MODE_INTERACT) return interact_fn();
          if (mode == SUBLATTICE_MODE_SPLIT) return split_fn();
          return parse_error::from_msg<key, CODE_BAD_VALUE>(
              R"(Invalid sublattice mode. Must be either "interact" or "split")");
        });
  }
}  // namespace sqsgen::io::config

#endif  // SQSGEN_IO_CONFIG_SHARED_H
