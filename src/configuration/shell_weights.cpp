
#include "sqsgen/configuration/shell_weights.h"

#include <cassert>
#include <cstddef>
#include <map>
#include <optional>
#include <string>

#include "sqsgen/core/helpers/string.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace helpers = sqsgen::core::helpers;

  result_t<std::size_t> parse_shell(const std::string& key,
                                    std::optional<std::size_t> index = std::nullopt) {
    std::string path = index.has_value()
                           ? helpers::format_string("shell_weights.%zu", index.value())
                           : "shell_weights";
    try {
      return static_cast<std::size_t>(std::stoul(key));
    } catch (const std::invalid_argument& e) {
      return std::unexpected(
          configuration_error{helpers::format_string("%s.%s", path, key), error_code::bad_argument,
                              helpers::format_string("Could not parse shell index: %s", key)});
    } catch (const std::out_of_range& e) {
      return std::unexpected(
          configuration_error{helpers::format_string("%s.%s", path, key), error_code::out_of_range,
                              helpers::format_string("Could not parse shell index: %s", key)});
    }
  }

  template <class T> result_t<std::map<std::size_t, T>> parse_shell_weights(
      const std::map<std::string, T>& weight_input, std::size_t num_shells,
      std::optional<std::size_t> index = std::nullopt) {
    std::string path = index.has_value()
                           ? helpers::format_string("shell_weights.%zu", index.value())
                           : "shell_weights";
    if (weight_input.empty())
      return std::unexpected(
          configuration_error{path, error_code::invalid_size, "shell weights cannot be empty"});
    std::map<std::size_t, T> out;
    for (const auto& [key, weight] : weight_input) {
      auto shell = parse_shell(key, index);
      if (!shell) return std::unexpected(std::move(shell).error());
      if (shell.value() == 0)
        return std::unexpected(configuration_error{
            helpers::format_string("%s.%s", path, key), error_code::bad_argument,
            "There is no point in including self-interactions"});
      if (shell.value() >= num_shells)
        return std::unexpected(configuration_error{
            helpers::format_string("%s.%s", path, key), error_code::invalid_size,
            helpers::format_string("There are %zu shells but you specified a shell index of %zu",
                                   num_shells, shell.value())});
      out.insert(shell.value(), weight);
    }
    return out;
  }

  template <class T> std::map<std::size_t, T> default_shell_weights(std::size_t num_shells) {
    std::map<std::size_t, T> out;
    for (std::size_t index = 1; index < num_shells + 1; ++index)
      out.insert(index, T(1.0) / static_cast<T>(index));

    return out;
  }

  template <class T, sublattice_mode Mode>
  result_t<shell_weights_t<T, Mode>> validate_shell_weight_input(
      const shell_weights_input<T, Mode>& input, const shell_radii_t<T, Mode>& shell_radii) {
    if constexpr (Mode == sublattice_mode::interact) {
      auto num_shells = shell_radii.size() - 1;
      if (!input.has_value())
        return default_shell_weights<T>(num_shells);
      else
        return parse_shell_weights(input.value(), num_shells);
    } else if constexpr (Mode == sublattice_mode::split) {
      auto num_sublattices = shell_radii.size();
      shell_weights_t<T, Mode> out;
      out.reserve(num_sublattices);
      if (input.has_value()) {
        const auto& data = input.value();
        if (data.size() != shell_radii.size()) {
          return std::unexpected(configuration_error{
              "shell_weights", error_code::invalid_size,
              helpers::format_string("the number of shell weight definitions (%zu) does not match "
                                     "the number of sublattices (%zu).",
                                     data.size(), shell_radii.size())});
        }
        for (std::size_t index = 0; index < num_sublattices; ++index) {
          auto num_shells = shell_radii[index].size() - 1;
          auto validated = parse_shell_weights(data[index], num_shells, std::make_optional(index));
          if (!validated) return std::unexpected(std::move(validated).error());
          out.push_back(std::move(validated).value());
        }
      } else {
        for (const auto& shell_radii_vector : shell_radii) {
          assert(shell_radii_vector.size() >= 1 && "checked upstream in shell_radii validator");
          auto num_shells = shell_radii_vector.size() - 1;
          out.push_back(default_shell_weights<T>(num_shells));
        }
      }
      return out;
    } else
      static_assert(Mode == sublattice_mode::interact || Mode == sublattice_mode::split,
                    "unhandled sublattice_mode");
  }
}  // namespace sqsgen::configuration
