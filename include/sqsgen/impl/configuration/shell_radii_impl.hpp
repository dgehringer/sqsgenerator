//
// Created by Dominik Gehringer on 02.12.24.
//

#ifndef SQSGEN_IMPL_CONFIGURATION_SHELL_RADII_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_SHELL_RADII_IMPL_HPP

#include <cmath>
#include <cstddef>
#include <expected>
#include <format>
#include <ranges>
#include <stdexcept>
#include <variant>
#include <vector>

#include "sqsgen/configuration/shell_radii.h"
#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/core/helpers/variant.h"
#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace helpers = sqsgen::core::helpers;

  template <class T> T round_digits(T value, int n) {
    T multiplier = std::pow(10.0, n);
    return std::round(value * multiplier) / multiplier;
  }

  template <class T> int digits(T val) {
    auto num_digits = -static_cast<int>(std::round(std::log10(val)));
    if (num_digits <= 0) throw std::invalid_argument("a tolerance must be between 0 and 1");
    return num_digits;
  }

  template <class T> result_t<std::vector<T>> validate_radii(const std::vector<T>& radii, T atol) {
    auto num_digits = digits(atol);
    if (radii.empty())
      return std::unexpected(configuration_error{"shell_radii", error_code::invalid_size,
                                                 "shell radii vector cannot be empty"});
    core::sorted_vector<T> unique_radii(radii | std::ranges::views::transform([=](auto shell) {
                                          return round_digits<T>(shell, num_digits);
                                        }));

    if (helpers::is_close<T>(unique_radii.front(), 0.0)) {
      if (unique_radii.size() == 1)
        return std::unexpected(
            configuration_error{"shell_radii", error_code::invalid_size,
                                "you need to specify at least one shell radius"});
    } else
      unique_radii.insert(0.0);

    std::vector<T> out;
    out.reserve(unique_radii.size());
    out.push_back(unique_radii.front());
    T prev = unique_radii.front();
    for (std::size_t index = 1; index < unique_radii.size(); ++index) {
      auto radius = unique_radii.at(index);
      if (helpers::is_close(prev + atol, radius) || radius < prev + atol) {
        return std::unexpected(configuration_error{
            "shell_radii", error_code::bad_argument,
            std::format("two shells are too close (not even {:f} apart) or the second one "
                        "is smaller than the first. Those are {:f} and {:f}",
                        atol, prev, radius)});
      }
      out.push_back(radius);
      prev = radius;
    }
    return out;
  }

  template <class T, sublattice_mode Mode>
  std::expected<shell_radii_t<T, Mode>, configuration_error> validate_shell_radii_input(
      const shell_radii_input<T, Mode>& input, const structure_t<T, Mode>& structure, T atol,
      T rtol, T bin_width, T peak_isolation) {
    std::variant<shell_radii_detection, shell_radii_spec<T, Mode>> input_or_default
        = input.value_or(shell_radii_detection::peak);
    if constexpr (Mode == sublattice_mode::interact) {
      return std::visit(
          helpers::overloaded{[&](shell_radii_detection mode)
                                  -> result_t<shell_radii_t<T, sublattice_mode::interact>> {
                                if (mode == shell_radii_detection::naive) {
                                  return core::distances_naive(structure, atol, rtol);
                                }
                                if (mode == shell_radii_detection::peak) {
                                  return core::distances_histogram(structure, bin_width,
                                                                   peak_isolation);
                                }
                                throw std::invalid_argument("Unknown shell detection mode");
                              },
                              [&](const std::vector<T>& radii)
                                  -> result_t<shell_radii_t<T, sublattice_mode::interact>> {
                                auto shell_radii = validate_radii<T>(radii, atol);
                                if (!shell_radii)
                                  return std::unexpected(std::move(shell_radii).error());
                                return std::move(shell_radii).value();
                              }},
          input_or_default);
    } else if constexpr (Mode == sublattice_mode::split) {
      using output_t = shell_radii_t<T, Mode>;
      return std::visit(
          helpers::overloaded{
              [&](shell_radii_detection mode) -> result_t<output_t> {
                if (mode == shell_radii_detection::naive) {
                  return structure | std::ranges::views::transform([&](const auto& structure) {
                           return core::distances_naive(structure, atol, rtol);
                         })
                         | std::ranges::to<output_t>();
                }
                if (mode == shell_radii_detection::peak) {
                  return structure | std::ranges::views::transform([&](const auto& structure) {
                           return core::distances_histogram(structure, bin_width, peak_isolation);
                         })
                         | std::ranges::to<output_t>();
                }
                return std::unexpected(configuration_error{"shell_radii", error_code::unknown,
                                                           "Unknown shell detection mode"});
              },
              [&](const shell_radii_spec<T, Mode>& radii) -> result_t<output_t> {
                if (radii.size() != structure.size()) {
                  return std::unexpected(configuration_error{
                      "shell_radii", error_code::invalid_size,
                      std::format("the number of shell radii vectors ({}) does not match "
                                  "the number of sublattices ({}).",
                                  radii.size(), structure.size())});
                }
                output_t out;
                out.reserve(structure.size());
                for (const auto& r : radii) {
                  auto validated = validate_radii(r, atol);
                  if (!validated) return std::unexpected(std::move(validated).error());
                  out.push_back(std::move(validated).value());
                }
                return out;
              }},
          input_or_default);
    } else
      static_assert(Mode == sublattice_mode::interact || Mode == sublattice_mode::split,
                    "unhandled sublattice_mode");
  }

}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_SHELL_RADII_IMPL_HPP
