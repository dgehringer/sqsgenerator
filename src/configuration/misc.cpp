

#include "sqsgen/configuration/misc.h"

#include <cstddef>
#include <expected>
#include <optional>
#include <vector>

#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/core/helpers/string.h"
#include "sqsgen/core/permutation.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace core = sqsgen::core;
  namespace helpers = sqsgen::core::helpers;

  template <class T>
  result_t<T> validate_positive_arithmetic(const std::string& key, const std::optional<T>& input,
                                           T default_value, const std::string& name) {
    if (input.has_value()) {
      auto value = input.value();
      auto eps = std::numeric_limits<T>::epsilon();
      if (value <= T{0.0} || helpers::is_close(value, +eps) || helpers::is_close(value, -eps))
        return std::unexpected(
            configuration_error{key, error_code::bad_argument,
                                helpers::format_string("The %s must be greater 0", name)});
      return value;
    }
    return default_value;
  }

  result_t<iteration_mode> validate_iteration_mode_input(
      iteration_mode_input input, const std::vector<sublattice>& composition) {
    if (input.has_value()) {
      auto iteration_mode = input.value();
      if (iteration_mode == iteration_mode::systematic) {
        if (composition.size() != 1)
          return std::unexpected(configuration_error{
              "iteration_mode", error_code::bad_argument,
              "In \"systematic\" iteration mode only one sublattice is allowed"});
        auto iterations = core::num_permutations(composition.front().composition);
        if (iterations > std::numeric_limits<iterations_t>::max()) {
          return std::unexpected(configuration_error{
              "iteration_mode", error_code::bad_argument,
              helpers::format_string("The number of permutations to test is %s. I'm a pretty fast "
                                     "program, but this is too much even for me ;=)",
                                     iterations.str())});
        }
      }

      return iteration_mode;
    }
    return defaults::iteration_mode_default;
  }

  result_t<iterations_t> validate_iterations_input(const iterations_input& input,
                                                   iteration_mode mode) {
    if (input.value()) {
      if (mode == iteration_mode::systematic)
        return std::unexpected(
            configuration_error{"iterations", error_code::bad_argument,
                                "In systematic mode no iterations can be specified"});
      auto iterations = input.value();
      if (iterations == 0) {
        return std::unexpected(configuration_error{
            "iterations", error_code::bad_argument,
            helpers::format_string("The number of iterations must be greater 0")});
      }
      return input;
    } else {
      return mode == iteration_mode::random ? std::make_optional(defaults::iterations_default)
                                            : std::nullopt;
    }
  }

  result_t<chunk_size_t> validate_chunk_size_input(const chunk_size_input& input,
                                                   const iterations_t iterations) {
    if (input) {
      auto chunk_size = input.value();
      if (iterations.has_value() && chunk_size > iterations.value())
        return std::unexpected(configuration_error{
            "chunk_size", error_code::out_of_range,
            helpers::format_string("\"chunk_size\" was set to %zu and iterations set to %zu",
                                   chunk_size, iterations.value())});
      return chunk_size;
    }
    return iterations.has_value() ? std::min(iterations.value(), defaults::chunk_size_default)
                                  : defaults::chunk_size_default;
  };
  template <class T> result_t<atol_t<T>> validate_atol_input(const atol_input<T>& input) {
    return validate_positive_arithmetic("atol", input, defaults::atol_default<T>,
                                        "absolute tolerance");
  }
  template <class T> result_t<rtol_t<T>> validate_rtol_input(const rtol_input<T>& input) {
    return validate_positive_arithmetic("rtol", input, defaults::rtol_default<T>,
                                        "relative tolerance");
  }

  template <class T>
  result_t<bin_width_t<T>> validate_bin_width_input(const bin_width_input<T>& input) {
    return validate_positive_arithmetic("bin_width", input, defaults::bin_width_default<T>,
                                        "shell radii histogramm bin width");
  };
  template <class T> result_t<peak_isolation_t<T>> validate_peak_isolation_input(
      const peak_isolation_input<T>& input) {
    return validate_positive_arithmetic("peak_isolation", input, defaults::bin_width_default<T>,
                                        "relative peak isolation in the shell radii historgram");
  };

  result_t<seed_t> validate_seed(const seed_input& input,
                                 const std::vector<sublattice>& composition, iteration_mode mode) {
    if (mode == iteration_mode::systematic)
      return std::unexpected(configuration_error{
          "seed", error_code::bad_argument,
          "You cannot specify a seed if you have set the iteration mode to \"systematic\". The "
          "seed is only used for random iterations, but in systematic mode there is no "
          "randomness involved."});
    if (input.has_value()) {
      auto seed = input.value();
      if (seed.size() != composition.size())
        std::unexpected(
            configuration_error{"seed", error_code::invalid_size,
                                helpers::format_string("The number of seeds specified (%zu) does "
                                                       "not match the number of sublattices (%zu)",
                                                       seed.size(), composition.size())});
      return seed;
    }
    return seed_t(composition.size(), defaults::seed_default);
  };

  result_t<keep_t> validate_keep_input(const keep_input& input) {
    if (input.has_value()) {
      auto keep = input.value();
      if (keep == 0)
        return std::unexpected(
            configuration_error{"keep", error_code::bad_argument,
                                "The number of best kept objectives must be greater than 0"});
      return keep;
    }
    return defaults::keep_default;
  }

  result_t<thread_config_t> validate_thread_config_input(const thread_config_input&) {
    return thread_config_t(1, std::nullopt);
  }

  template result_t<atol_t<float>> validate_atol_input(const atol_input<float>&);
  template result_t<atol_t<double>> validate_atol_input(const atol_input<double>&);

  template result_t<rtol_t<float>> validate_rtol_input(const rtol_input<float>&);
  template result_t<rtol_t<double>> validate_rtol_input(const rtol_input<double>&);

  template result_t<bin_width_t<float>> validate_bin_width_input(const bin_width_input<float>&);
  template result_t<bin_width_t<double>> validate_bin_width_input(const bin_width_input<double>&);

  template result_t<peak_isolation_t<float>> validate_peak_isolation_input(
      const peak_isolation_input<float>&);
  template result_t<peak_isolation_t<double>> validate_peak_isolation_input(
      const peak_isolation_input<double>&);

}  // namespace sqsgen::configuration
