

#include "sqsgen/configuration/misc.h"

#include <expected>
#include <optional>

#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/core/helpers/string.h"
#include "sqsgen/core/permutation.h"
#include "sqsgen/core/structure.h"
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

  result_t<iteration_mode> validate_iterations_input(iteration_mode_input input,
                                                     const std::vector<sublattice>& composition) {
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

  template <class T, sublattice_mode Mode>
  result_t<iterations_t> validate_iterations_input(const iterations_input& input,
                                                   iteration_mode mode,
                                                   const structure_t<T, Mode>& structure);

  result_t<iterations_t> validate_chunk_size_input(const chunck_size_input& input,
                                                   const std::optional<iterations_t> iterations) {
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

  template <sublattice_mode Mode>
  result_t<seed_t<Mode>> validate_seed(const seed_input<Mode>&, iteration_mode);

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

  template <bool Mpi>
  result_t<thread_config_t<Mpi>> validate_thread_config_input(const thread_config_input<Mpi>&);

  result_t<prec> validate_prec_input(const prec_input& input) {
    if ("SINGLE" == helpers::to_uppercase(input))
      return prec::Single;
    else if ("DOUBLE" == helpers::to_uppercase(input))
      return prec::Double;
    return std::unexpected(configuration_error{"prec", error_code::bad_argument,
                                               "prec can only be \"single\" or \"double\""});
  }
}  // namespace sqsgen::configuration
