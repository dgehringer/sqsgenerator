//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_CONFIGURATION_PAIR_WEIGHTS_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_PAIR_WEIGHTS_IMPL_HPP

#include <cassert>
#include <cstddef>
#include <format>
#include <map>
#include <optional>
#include <span>
#include <string>

#include "sqsgen/configuration/pair_weights.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  inline result_t<std::size_t> parse_weight(const std::string& key,
                                            std::optional<std::size_t> index = std::nullopt) {
    std::string path
        = index.has_value() ? std::format("pair_weights.{}", index.value()) : "pair_weights";
    try {
      return static_cast<std::size_t>(std::stoul(key));
    } catch (const std::invalid_argument& e) {
      return std::unexpected(
          configuration_error{std::format("{}.{}", path, key), error_code::bad_argument,
                              std::format("Could not parse shell index: {}", key)});
    } catch (const std::out_of_range& e) {
      return std::unexpected(
          configuration_error{std::format("{}.{}", path, key), error_code::out_of_range,
                              std::format("Could not parse shell index: {}", key)});
    }
  }

  template <class T>
  result_t<std::map<shell_t, T>> parse_pair_weights(const std::map<std::string, T>& weight_input,
                                                    std::size_t num_shells,
                                                    std::optional<std::size_t> index
                                                    = std::nullopt) {
    std::string path
        = index.has_value() ? std::format("pair_weights.{}", index.value()) : "pair_weights";
    if (weight_input.empty())
      return std::unexpected(
          configuration_error{path, error_code::invalid_size, "shell weights cannot be empty"});
    std::map<shell_t, T> out;
    for (const auto& [key, weight] : weight_input) {
      auto shell = parse_weight(key, index);
      if (!shell) return std::unexpected(std::move(shell).error());
      if (shell.value() == 0)
        return std::unexpected(
            configuration_error{std::format("{}.{}", path, key), error_code::bad_argument,
                                "There is no point in including self-interactions"});
      if (shell.value() >= num_shells)
        return std::unexpected(configuration_error{
            std::format("{}.{}", path, key), error_code::invalid_size,
            std::format("There are {} shells but you specified a shell index of {}", num_shells,
                        shell.value())});
      out.emplace(static_cast<shell_t>(shell.value()), weight);
    }
    return out;
  }

  // w_i = r_1 / r_i for every shell in shell_radii (index 0 = the site itself, r_0 = 0)
  template <class T> std::map<shell_t, T> default_pair_weights(std::span<const T> shell_radii) {
    assert(shell_radii.size() >= 2 && shell_radii[0] == T{0} && shell_radii[1] > T{0});
    const T r_1 = shell_radii[1];
    std::map<shell_t, T> out;
    for (std::size_t i = 1; i < shell_radii.size(); ++i)
      out.emplace(static_cast<shell_t>(i), r_1 / shell_radii[i]);
    return out;
  }

  template <class T, sublattice_mode Mode>
  result_t<pair_weights_t<T, Mode>> validate_pair_weights_input(
      const pair_weights_input<T, Mode>& input, const shell_radii_t<T, Mode>& shell_radii) {
    if constexpr (Mode == sublattice_mode::interact) {
      auto num_shells = shell_radii.size() - 1;
      if (!input.has_value())
        return default_pair_weights<T>(shell_radii);
      else
        return parse_pair_weights(input.value(), num_shells);
    } else if constexpr (Mode == sublattice_mode::split) {
      auto num_sublattices = shell_radii.size();
      pair_weights_t<T, Mode> out;
      out.reserve(num_sublattices);
      if (input.has_value()) {
        const auto& data = input.value();
        if (data.size() != shell_radii.size()) {
          return std::unexpected(configuration_error{
              "pair_weights", error_code::invalid_size,
              std::format("the number of shell weight definitions ({}) does not match "
                          "the number of sublattices ({}).",
                          data.size(), shell_radii.size())});
        }
        for (std::size_t index = 0; index < num_sublattices; ++index) {
          auto num_shells = shell_radii[index].size() - 1;
          auto validated = parse_pair_weights(data[index], num_shells, std::make_optional(index));
          if (!validated) return std::unexpected(std::move(validated).error());
          out.push_back(std::move(validated).value());
        }
      } else {
        for (const auto& shell_radii_vector : shell_radii) {
          assert(shell_radii_vector.size() >= 1 && "checked upstream in shell_radii validator");
          out.push_back(default_pair_weights<T>(shell_radii_vector));
        }
      }
      return out;
    } else
      static_assert(Mode == sublattice_mode::interact || Mode == sublattice_mode::split,
                    "unhandled sublattice_mode");
  }

}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_PAIR_WEIGHTS_IMPL_HPP
