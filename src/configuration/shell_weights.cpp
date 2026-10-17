
#include "sqsgen/configuration/shell_weights.h"

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "sqsgen/core/helpers/string.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace helpers = sqsgen::core::helpers;

  result_t<std::size_t> parse_shell(const std::string& key,
                                    std::optional<std::size_t> index = std::nullopt) {
    auto path = index.has_value() ? helpers::format_string("shell_weights.%zu", index.value())
                                  : "shell_weights";
    try {
      return static_cast<std::size_t>(std::stoul(key));
    } catch (std::invalid_argument const& e) {
      return std::unexpected(
          configuration_error{helpers::format_string("%s.%s", path, key), error_code::bad_argument,
                              helpers::format_string("Could not parse shell index: %s", key)});
    } catch (std::out_of_range const& e) {
      return std::unexpected(
          configuration_error{helpers::format_string("%s.%s", path, key), error_code::out_of_range,
                              helpers::format_string("Could not parse shell index: %s", key)});
    }
  }

  template <class T>
  result_t<shell_weights_t<T>> parse_shell_weights(const std::map<std::string, T>& weight_input,
                                                   std::size_t num_shells,
                                                   std::optional<std::size_t> index
                                                   = std::nullopt) {
    auto path = index.has_value() ? helpers::format_string("shell_weights.%zu", index.value())
                                  : "shell_weights";
    if (weight_input.empty())
      return std::unexpected(
          configuration_error{path, error_code::invalid_size, "shell weights cannot be empty"});
    shell_weights_t<T> out;
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
            helpers::format_string("There are %zu shells but you specified a shell index of %i",
                                   num_shells, shell.value())});
      out.insert(shell.value(), weight);
    }
    return out;
  }

  template <class T> shell_weights_t<T> default_shell_weights(std::size_t num_shells) {
    shell_weights_t<T> out;
    for (std::size_t index = 1; index < num_shells + 1; ++index)
      out.insert(index, T(1.0) / static_cast<T>(index));

    return out;
  }

  template <class T, sublattice_mode Mode>
  result_t<shell_weights<T, Mode>> validate_shell_weight_input(
      const shell_weights_input<T, Mode>& input, const shell_radii<T, Mode>& shell_radii) {
    if constexpr (Mode == sublattice_mode::interact) {
      auto num_shells = shell_radii.size() - 1;
      if (!input.has_value())
        return default_shell_weights(num_shells);
      else
        return parse_shell_weights(input.value(), num_shells);
    }
    if constexpr (Mode == sublattice_mode::split) {
      if (input.size() != shell_radii.size()) {
        return std::unexpected(configuration_error{
            "shell_radii", error_code::invalid_size,
            helpers::format_string("the number of shell weight definitions (%zu) does not match "
                                   "the number of sublattices (%zu).",
                                   input.size(), shell_radii.size())});
      }
      auto num_sublattices = input.size();
      shell_weights<T, Mode> out;
      out.reserve(num_sublattices);
      for (std::size_t index = 0; index < num_sublattices; ++index) {
        auto num_shells = shell_radii[index].size() - 1;
        auto validated = parse_shell_weights(input[index], num_shells, std::make_optional(index));
        if (!validated) return std::unexpected(std::move(validated).error());
        out.push_back(std::move(validated).value());
      }
      return out;
    } else
      static_assert(Mode == sublattice_mode::interact || Mode == sublattice_mode::split,
                    "unhandled sublattice_mode");
  }
}  // namespace sqsgen::configuration
