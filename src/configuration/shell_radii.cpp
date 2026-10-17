
#include "sqsgen/configuration/shell_radii.h"

#include <cmath>
#include <cstddef>
#include <expected>
#include <ranges>
#include <stdexcept>
#include <variant>
#include <vector>

#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/core/helpers/string.h"
#include "sqsgen/core/helpers/variant.h"
#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace helpers = sqsgen::core::helpers;

  template <class T> T round_digits(double value, int n) {
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

    if (helpers::is_close(unique_radii.front(), 0.0)) {
      if (unique_radii.size() == 1)
        return std::unexpected(
            configuration_error{"shell_radii", error_code::invalid_size,
                                "you need to specify at least one shell radius"});
    } else
      unique_radii.insert(0.0);

    std::vector<T> out;
    out.reserve(unique_radii.size());
    T prev = unique_radii.front();
    for (std::size_t index = 1; index < unique_radii.size(); ++index) {
      auto radius = unique_radii[index];
      if (helpers::is_close(prev + atol, radius) || radius < prev + atol) {
        return std::unexpected(configuration_error{
            "shell_radii", error_code::bad_argument,
            helpers::format_string("two shells are too close (not even %f apart) or the second one "
                                   "is smaller than the first. Those are %f and %f",
                                   atol, prev, radius)});
        out.push_back(radius);
        prev = radius;
      }
    }
    return out;
  };

  template <class T, sublattice_mode Mode>
  std::expected<shell_radii<T, Mode>, configuration_error> validate_shell_radii_input(
      const shell_radii_input<T, Mode>& input, const structure_input<T, Mode>& structure, T atol,
      T rtol, T bin_width, T peak_isolation) {
    if constexpr (Mode == sublattice_mode::interact) {
      return std::visit(helpers::overloaded{
          [&](shell_radii_detection mode) -> result_t<shell_radii<T, sublattice_mode::interact>> {
            if (mode == shell_radii_detection::naive) {
              return core::distances_naive(structure, atol, rtol);
            }
            if (mode == shell_radii_detection::peak) {
              return core::distances_histogram(structure, bin_width, peak_isolation);
            }
            throw std::invalid_argument("Unknown shell detection mode");
          },
          [&](const std::vector<T>& radii) -> result_t<shell_radii<T, sublattice_mode::interact>> {
            return std::move(validate_radii(radii, atol));
          }});
    }
    if constexpr (Mode == sublattice_mode::split) {
      using output_t = shell_radii<T, Mode>;
      return std::visit(helpers::overloaded{
          [&](shell_radii_detection mode) -> result_t<output_t> {
            if (mode == shell_radii_detection::naive) {
              return structure | std::ranges::views::transform([&](const auto& structure) {
                       return core::distances_naive(structure, atol, rtol);
                     })
                     | std::ranges::to<output_t>;
            }
            if (mode == shell_radii_detection::peak) {
              return structure | std::ranges::views::transform([&](const auto& structure) {
                       return core::distances_histogram(structure, bin_width, peak_isolation);
                     })
                     | std::ranges::to<output_t>;
            }
            throw std::invalid_argument("Unknown shell detection mode");
          },
          [&](const shell_radii_spec<T, Mode>& radii) -> result_t<output_t> {
            if (radii.size() != structure.size()) {
              return std::unexpected(configuration_error{
                  "shell_radii", error_code::invalid_size,
                  helpers::format_string("the number of shell radii vectors (%zu) does not match "
                                         "the number of sublattices (%zu).",
                                         radii.size(), structure.size())});
            }
            output_t out;
            out.reserve(structure.size());
            for (const auto& r : radii) {
              auto validated = validate_radii(r);
              if (!validated) return std::move(validated).error();
              out.push_back(std::move(validated).value());
            }
            return out;
          }});
    }
  }

  template <> std::expected<shell_radii<float, sublattice_mode::interact>, configuration_error>
  validate_shell_radii_input<float, sublattice_mode::interact>(
      const shell_radii_input<float, sublattice_mode::interact>& input,
      const structure_input<float, sublattice_mode::interact>& structure, float atol, float rtol,
      float bin_width, float peak_isolation);

  template <> std::expected<shell_radii<float, sublattice_mode::split>, configuration_error>
  validate_shell_radii_input<float, sublattice_mode::split>(
      const shell_radii_input<float, sublattice_mode::split>& input,
      const structure_input<float, sublattice_mode::split>& structure, float atol, float rtol,
      float bin_width, float peak_isolation);

  template <> std::expected<shell_radii<double, sublattice_mode::interact>, configuration_error>
  validate_shell_radii_input<double, sublattice_mode::interact>(
      const shell_radii_input<double, sublattice_mode::interact>& input,
      const structure_input<double, sublattice_mode::interact>& structure, double atol, double rtol,
      double bin_width, double peak_isolation);

  template <> std::expected<shell_radii<double, sublattice_mode::split>, configuration_error>
  validate_shell_radii_input<double, sublattice_mode::split>(
      const shell_radii_input<double, sublattice_mode::split>& input,
      const structure_input<double, sublattice_mode::split>& structure, double atol, double rtol,
      double bin_width, double peak_isolation);

};  // namespace sqsgen::configuration
