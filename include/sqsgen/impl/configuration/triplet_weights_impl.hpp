#ifndef SQSGEN_IMPL_CONFIGURATION_TRIPLET_WEIGHTS_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_TRIPLET_WEIGHTS_IMPL_HPP

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "sqsgen/configuration/triplet_weights.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace detail {

    // "(1, 1, 2)"; std::format of ranges is not available in every standard library
    inline std::string to_string(const shell_triple& shells) {
      return std::format("({}, {}, {})", shells[0], shells[1], shells[2]);
    }

    inline configuration_error triplet_weights_error(error_code code, std::string message) {
      return configuration_error{"triplet_weights", code, std::move(message)};
    }

    // Triplet weights of one (sub)lattice: selected classes with weights w_c > 0.
    //   no input -> every class that occurs, w_c = r_1 / d_c = r_1 / r_{s3}
    //               (s1 <= s2 <= s3, so s3 is the shell of the longest edge)
    //   input    -> exactly the listed classes; error if a shell does not exist, the class
    //               does not occur in the structure, it is listed twice, or the weight is
    //               negative. Classes with w = 0 are dropped.
    // triplet_classes: every class occurring within the shell radii, sorted
    //                  (as returned by make_triplet_list over all shells)
    // shell_radii[0] = 0 is the site itself, shell 1 is the nearest-neighbour shell.
    // where: prefix for error messages ("" or "sublattice i: ")
    template <class T> result_t<std::map<shell_triple, T>> validate_triplet_weights(
        const std::optional<std::vector<triplet_weight_input<T>>>& input,
        const core::triplet_class_mapping_t& triplet_classes, const std::vector<T>& shell_radii,
        std::string_view where) {
      using result = result_t<std::map<shell_triple, T>>;
      const auto fail = [&](error_code code, std::string message) -> result {
        return std::unexpected(triplet_weights_error(code, std::format("{}{}", where, message)));
      };

      if (shell_radii.size() < 2) return fail("no coordination shells available");
      const std::size_t num_shells = shell_radii.size() - 1;

      std::map<shell_triple, T> out;

      // defaults: w_c = r_1 / r_{s3}
      if (!input) {
        const T r_1 = shell_radii[1];
        for (const auto& shells : triplet_classes)
          out.emplace(shells, r_1 / shell_radii[shells[2]]);
        return out;  // may be empty if no triangle occurs within the shell radii
      }

      if (input->empty()) return fail("must not be empty; omit the key to use the defaults");

      std::set<shell_triple> seen;
      for (auto [shells, weight] : *input) {  // copy: the shells are sorted below
        std::ranges::sort(shells);            // canonical: s1 <= s2 <= s3
        if (shells[0] == 0) return fail("shell 0 is the site itself, the first shell is 1");
        if (shells[2] > num_shells)
          return fail(std::format("class {}: shell {} does not exist, there are {} shells",
                                  to_string(shells), shells[2], num_shells));
        // triplet_classes is sorted (make_triplet_list numbers the classes in sorted order)
        if (!std::ranges::binary_search(triplet_classes, shells))
          return fail(
              std::format("class {}: no such triangle occurs in the structure", to_string(shells)));
        if (!seen.insert(shells).second)
          return fail(std::format("class {} is listed more than once", to_string(shells)));
        if (!(weight >= T{0}))  // also rejects NaN
          return fail(std::format("weight of class {} must be >= 0", to_string(shells)));
        if (weight > T{0}) out.emplace(shells, weight);
      }
      if (out.empty()) return fail("no triplet class with weight > 0");
      return out;
    }

  }  // namespace detail

  template <class T, sublattice_mode Mode>
  result_t<triplet_weights_t<T, Mode>> validate_triplet_weights_input(
      const triplet_weights_input<T, Mode>& input,
      const sublattice_mode_nesting<core::triplet_class_mapping_t, Mode>& triplet_classes,
      const sublattice_mode_nesting<std::vector<T>, Mode>& shell_radii) {
    if constexpr (Mode == sublattice_mode::interact) {
      // key missing or null -> defaults
      return detail::validate_triplet_weights<T>(input ? *input : std::nullopt, triplet_classes,
                                                 shell_radii, "");
    } else {
      // split: one entry per sublattice; key missing -> defaults everywhere,
      // entry null -> defaults for that sublattice
      const std::size_t num_sublattices = triplet_classes.size();
      if (shell_radii.size() != num_sublattices)
        return std::unexpected(detail::triplet_weights_error(
            std::format("{} shell radii lists, but {} triplet class tables", shell_radii.size(),
                        num_sublattices)));
      if (input && input->size() != num_sublattices)
        return std::unexpected(detail::triplet_weights_error(std::format(
            "{} entries given, but there are {} sublattices", input->size(), num_sublattices)));

      triplet_weights_t<T, Mode> out;
      out.reserve(num_sublattices);
      for (std::size_t i = 0; i < num_sublattices; ++i) {
        auto weights = detail::validate_triplet_weights<T>(input ? (*input)[i] : std::nullopt,
                                                           triplet_classes[i], shell_radii[i],
                                                           std::format("sublattice {}: ", i));
        if (!weights) return std::unexpected(std::move(weights.error()));
        out.push_back(std::move(*weights));
      }
      return out;
    }
  }

}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_TRIPLET_WEIGHTS_IMPL_HPP
