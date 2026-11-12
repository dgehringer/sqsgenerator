//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_CONFIGURATION_SHARED_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_SHARED_IMPL_HPP

#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include "sqsgen/core/atom.h"
#include "sqsgen/core/helpers/variant.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace helpers = sqsgen::core::helpers;

  namespace detail {

    namespace traits {
      template <class A, sublattice_mode Mode> struct per_sublattice {
        using type = A;
      };
      template <class A> struct per_sublattice<A, sublattice_mode::split> {
        using type = std::ranges::range_value_t<A>;
      };
      template <class A, sublattice_mode Mode> using per_sublattice_t =
          typename per_sublattice<A, Mode>::type;
    }  // namespace traits

    template <sublattice_mode Mode, class Fn, class... Args>
    using lifted_t = result_t<sublattice_mode_nesting<
        typename std::invoke_result_t<Fn&, std::optional<std::string_view>,
                                      const traits::per_sublattice_t<Args, Mode>&...>::value_type,
        Mode>>;

    template <sublattice_mode Mode, class Fn, class... Args>
    lifted_t<Mode, Fn, Args...> lift(std::string_view key, Fn&& fn, const Args&... args) {
      static_assert(sizeof...(Args) > 0, "lift needs at least one argument");
      if constexpr (Mode == sublattice_mode::interact) {
        return fn(std::nullopt, args...);
      } else {
        const std::array<std::size_t, sizeof...(Args)> sizes{std::ranges::size(args)...};
        if (!std::ranges::all_of(sizes, [&](std::size_t s) { return s == sizes[0]; })) {
          std::string listed;
          for (const std::size_t s : sizes)
            listed += std::format("{}{}", listed.empty() ? "" : ", ", s);
          return std::unexpected(configuration_error{
              std::string(key), error_code::invalid_size,
              std::format("inputs differ in the number of sublattices ({})", listed)});
        }

        typename lifted_t<Mode, Fn, Args...>::value_type out;
        out.reserve(sizes[0]);
        for (std::size_t i = 0; i < sizes[0]; ++i) {
          // `where` must outlive the call: fn receives a string_view into it
          const std::string where = std::format("sublattice {}: ", i);
          auto one = fn(std::optional<std::string_view>{where}, args[i]...);
          if (!one) return std::unexpected(std::move(one.error()));
          out.push_back(std::move(one.value()));
        }
        return out;
      }
    }

    // Apply fn to each sublattice of a sublattice_mode_nesting, keeping the nesting shape:
    // interact -> fn(data); split -> one fn(element) per sublattice. The inverse direction of
    // lift(): turns validated per-sublattice data back into per-sublattice inputs.
    template <sublattice_mode Mode, class Data, class Fn>
    auto map_sublattices(const Data& data, Fn&& fn) {
      if constexpr (Mode == sublattice_mode::interact) {
        return fn(data);
      } else {
        using out_t = std::invoke_result_t<Fn&, const std::ranges::range_value_t<Data>&>;
        std::vector<out_t> out;
        out.reserve(std::ranges::size(data));
        for (const auto& element : data) out.push_back(fn(element));
        return out;
      }
    }

    template <sublattice_mode Mode, class A> std::size_t num_sublattices(const A& a) {
      if constexpr (Mode == sublattice_mode::interact)
        return 1;
      else
        return std::ranges::size(a);
    }

    template <sublattice_mode Mode, class Input>
    typename Input::value_type distribute(const Input& input, std::size_t num_sublattices) {
      using nested = typename Input::value_type;
      if constexpr (Mode == sublattice_mode::interact)
        return input.value_or(std::nullopt);
      else
        return input.value_or(nested(num_sublattices));
    }

    inline result_t<specie_t> validate_species(std::string_view key,
                                               const std::variant<int, std::string>& input,
                                               std::optional<std::string_view> where
                                               = std::nullopt) {
      return std::visit(
          helpers::overloaded{
              [&](int z) -> result_t<specie_t> {
                const auto atom = core::atom::from_z(z);
                if (!atom)
                  return std::unexpected(configuration_error{
                      std::string(key), error_code::bad_argument,
                      std::format("{}unknown ordinal {}", where.value_or(""), z)});
                else
                  return atom.value().Z;
              },
              [&](const std::string& symbol) -> result_t<specie_t> {
                const auto atom = core::atom::from_symbol(symbol);
                if (!atom)
                  return std::unexpected(configuration_error{
                      std::string(key), error_code::bad_argument,
                      std::format("{}unknown symbol {}", where.value_or(""), symbol)});
                else
                  return atom.value().Z;
              }},
          input);
    }
  }  // namespace detail
}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_SHARED_IMPL_HPP
