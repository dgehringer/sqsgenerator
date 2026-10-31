//
// Created by Dominik Gehringer on 05.11.24.
//

#ifndef SQSGEN_IMPL_CONFIGURATION_CONFIGURATION_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_CONFIGURATION_IMPL_HPP

#include <expected>
#include <optional>
#include <variant>

#include "sqsgen/configuration.h"
#include "sqsgen/core/helpers/variant.h"
#include "sqsgen/io/glaze.h"
#include "sqsgen/types.h"

#define SQSGEN_TRY_ASSIGN(var, ...)                                                   \
  auto&& _res_##var = (__VA_ARGS__);                                                  \
  if (!_res_##var.has_value()) return std::unexpected(std::move(_res_##var.error())); \
  auto var = std::move(_res_##var.value());

namespace sqsgen::configuration {

  namespace helpers = sqsgen::core::helpers;
  namespace core = sqsgen::core;

  namespace detail {
    struct configuration_input_meta {
      std::optional<prec> prec;
      std::optional<sublattice_mode> sublattice_mode;
    };

    inline configuration_error to_configuration_error(io::parser_error error) {
      return configuration_error{.key = "", .code = error.code, .msg = error.msg};
    }

    inline io::parser_error to_parser_error(configuration_error error) {
      return io::parser_error{.code = error.code, .msg = error.msg};
    }

    template <class T, sublattice_mode Mode>
    std::expected<configuration_input_variant_t, io::parser_error> read_input_specific(
        std::string_view data, io::wire_format fmt) {
      auto parsed = io::read_any<configuration_input<T, Mode>, glz::opts{
                                                                   .error_on_unknown_keys = true,
                                                                   .error_on_missing_keys = false,
                                                               }>(data, fmt);
      if (!parsed) return std::unexpected(std::move(parsed).error());
      return std::move(parsed).value();
    }
  }  // namespace detail

  inline std::expected<configuration_input_variant_t, io::parser_error> read_input(
      std::string_view data, io::wire_format fmt) {
    auto metadata
        = io::read_any<detail::configuration_input_meta, glz::opts{
                                                             .error_on_unknown_keys = false,
                                                             .error_on_missing_keys = false,
                                                         }>(data, fmt);
    if (!metadata) return std::unexpected(std::move(metadata).error());
    auto prec = metadata.value().prec.value_or(defaults::prec_default);
    auto smode = metadata.value().sublattice_mode.value_or(defaults::sublattice_mode_default);

    if (prec == prec::Single && smode == sublattice_mode::interact)
      return detail::read_input_specific<float, sublattice_mode::interact>(data, fmt);
    else if (prec == prec::Double && smode == sublattice_mode::interact)
      return detail::read_input_specific<double, sublattice_mode::interact>(data, fmt);
    else if (prec == prec::Single && smode == sublattice_mode::split)
      return detail::read_input_specific<float, sublattice_mode::split>(data, fmt);
    else if (prec == prec::Double && smode == sublattice_mode::split)
      return detail::read_input_specific<double, sublattice_mode::split>(data, fmt);
    else
      return std::unexpected(io::parser_error{.code = error_code::unknown, .msg = "Unknown error"});
  }

  template <class T, sublattice_mode Mode>
  result_t<configuration<T, Mode>> parse_configuration_internal(
      const configuration_input<T, Mode>& input) {
    // the first thing we need is structure and composition
    SQSGEN_TRY_ASSIGN(_structure_config, validate_structure_input<T>(input.structure));

    SQSGEN_TRY_ASSIGN(
        composition,
        validate_composition_input(input.composition, _structure_config.make_structure().species));
    const structure_t<T, Mode> structure = _structure_config.template for_mode<Mode>(composition);
    SQSGEN_TRY_ASSIGN(imode, validate_iteration_mode_input(input.iteration_mode, composition));
    SQSGEN_TRY_ASSIGN(seed, validate_seed_input(input.seed, composition, imode));
    SQSGEN_TRY_ASSIGN(iterations, validate_iterations_input(input.iterations, imode));
    SQSGEN_TRY_ASSIGN(chunk_size, validate_chunk_size_input(input.chunk_size, iterations));
    SQSGEN_TRY_ASSIGN(keep, validate_keep_input(input.keep));
    SQSGEN_TRY_ASSIGN(max_results_per_objective,
                      validate_max_results_per_objective_input(input.max_results_per_objective));
    SQSGEN_TRY_ASSIGN(atol, validate_atol_input<T>(input.atol));
    SQSGEN_TRY_ASSIGN(rtol, validate_rtol_input<T>(input.rtol));
    SQSGEN_TRY_ASSIGN(bin_width, validate_bin_width_input<T>(input.bin_width));
    SQSGEN_TRY_ASSIGN(peak_isolation, validate_peak_isolation_input<T>(input.peak_isolation));
    SQSGEN_TRY_ASSIGN(shell_radii,
                      validate_shell_radii_input<T, Mode>(input.shell_radii, structure, atol, rtol,
                                                          bin_width, peak_isolation));
    SQSGEN_TRY_ASSIGN(shell_weights,
                      validate_shell_weight_input<T, Mode>(input.shell_weights, shell_radii));

    SQSGEN_TRY_ASSIGN(prefactors, validate_prefactors_input<T, Mode>(input.prefactors, structure,
                                                                     shell_weights, shell_radii));
    SQSGEN_TRY_ASSIGN(pair_weights, validate_pair_weights_input<T, Mode>(
                                        input.pair_weights, structure, shell_weights, shell_radii));

    SQSGEN_TRY_ASSIGN(target_objective,
                      validate_target_objective_input<T, Mode>(input.target_objective, structure,
                                                               shell_weights, shell_radii));
    SQSGEN_TRY_ASSIGN(thread_config, validate_thread_config_input(input.thread_config));
    return configuration<T, Mode>{
        .sublattice_mode = Mode,
        .prec = arithmethic_to_prec<T>(),
        .atol = atol,
        .rtol = rtol,
        .bin_width = bin_width,
        .peak_isolation = peak_isolation,
        .structure = std::move(_structure_config),
        .composition = std::move(composition),
        .iteration_mode = imode,
        .shell_radii = std::move(shell_radii),
        .shell_weights = std::move(shell_weights),
        .pair_weights = std::move(pair_weights),
        .target_objective = std::move(target_objective),
        .prefactors = std::move(prefactors),
        .iterations = iterations,
        .chunk_size = chunk_size,
        .keep = keep,
        .thread_config = std::move(thread_config),
        .max_results_per_objective = max_results_per_objective,
        .seed = std::move(seed),
    };
  }

  inline result_t<configuration_variant_t> parse_configuration(
      const configuration_input_variant_t& input) {
    return std::visit(
        helpers::overloaded{
            []<class T, sublattice_mode Mode>(
                const configuration_input<T, Mode>& input) -> result_t<configuration_variant_t> {
              auto validated = parse_configuration_internal<T, Mode>(input);
              if (!validated)
                return std::unexpected(std::move(validated).error());
              else
                return configuration_variant_t{std::move(validated.value())};
            }},
        input);
  }

}  // namespace sqsgen::configuration

#undef SQSGEN_TRY_ASSIGN

#endif  // SQSGEN_IMPL_CONFIGURATION_CONFIGURATION_IMPL_HPP
