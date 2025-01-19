//
// Created by Dominik Gehringer on 04.01.25.
//

#ifndef SQSGEN_IO_CONFIG_COMBINED_H
#define SQSGEN_IO_CONFIG_COMBINED_H
#include "sqsgen/config.h"
#include "sqsgen/io/config/arrays.h"
#include "sqsgen/io/config/composition.h"
#include "sqsgen/io/config/shell_radii.h"
#include "sqsgen/io/config/shell_weights.h"
#include "sqsgen/io/config/structure.h"
#include "sqsgen/io/parsing.h"

namespace sqsgen::io::config {

  static constexpr iterations_t iterations_default = 500000;
  static constexpr iterations_t chunk_size_default = 100000;

  template <string_literal key, class Document>
  parse_result<IterationMode> parse_iteration_mode(Document const& document) {
    return get_optional<key, IterationMode>(document).value_or(
        parse_result<IterationMode>{ITERATION_MODE_RANDOM});
  }

  template <string_literal key, class Document>
  parse_result<std::optional<iterations_t>> parse_iterations(Document const& doc,
                                                             IterationMode mode) {
    using result_t = parse_result<std::optional<iterations_t>>;
    if (mode == ITERATION_MODE_SYSTEMATIC) return result_t{std::nullopt};
    return get_optional<key, iterations_t>(doc)
        .value_or(parse_result<iterations_t>{iterations_default})
        .and_then([](auto&& iterations) -> result_t { return result_t{iterations}; });
  }

  template <string_literal key, class Document>
  parse_result<thread_config_t> parse_threads_per_rank(Document const& doc) {
    using result_t = parse_result<thread_config_t>;
    return get_either_optional<key, usize_t, std::vector<usize_t>>(doc)
        .value_or(parse_result<usize_t, std::vector<usize_t>>{usize_t(0)})
        .template collapse<thread_config_t>(
            [&](usize_t&& num_threads) -> result_t { return {num_threads}; },
            [&](std::vector<usize_t>&& num_threads) -> result_t { return {num_threads}; });
  }

  template <string_literal key, class Document>
  parse_result<iterations_t> parse_chunk_size(Document const& doc,
                                              std::optional<iterations_t> iterations) {
    using result_t = parse_result<iterations_t>;
    return get_optional<key, iterations_t>(doc)
        .value_or(parse_result<iterations_t>{chunk_size_default})
        .and_then([&](auto&& cs) -> result_t {
          if (iterations.has_value() && cs >= iterations.value())
            return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(std::format(
                "\"chunk_size\" was set to {} and iterations set to {}", cs, iterations.value()));
          return result_t{cs};
        });
  }

  template <string_literal key, class Document>
  parse_result<SublatticeMode> parse_sublattice_mode(Document const& doc) {
    return get_optional<"sublattice_mode", SublatticeMode>(doc)
        .value_or(parse_result<SublatticeMode>{SUBLATTICE_MODE_INTERACT})
        .and_then([&](auto&& mode) -> parse_result<SublatticeMode> {
          if (mode == SUBLATTICE_MODE_INVALID)
            return parse_error::from_msg<key, CODE_BAD_VALUE>(
                R"(Invalid sublattice mode. Must be either "interact" or "split")");
          return mode;
        });
  }

  template <class T, class Document>
  parse_result<configuration<T>> parse_config(Document const& doc) {
    return config::parse_structure_config<"structure", T>(doc)
        .combine(parse_iteration_mode<"mode">(doc))
        .combine(parse_sublattice_mode<"sublattice_mode">(doc))
        .and_then([&](auto&& sc_and_m) {
          auto [sc, iteration_mode, sublattice_mode] = sc_and_m;
          auto structure = sc.structure();
          /*if (iteration_mode == ITERATION_MODE_SYSTEMATIC
              && sublattice_mode == SUBLATTICE_MODE_SPLIT)
            return parse_error::from_msg<"mode", CODE_BAD_VALUE>(
                "It is not possible to do an exhaustive search on multiple sublattices");*/
          return config::parse_composition<"composition", "sites">(doc, structure.species)
              .combine(parse_iterations<"iterations">(doc, iteration_mode))
              .and_then([&](auto&& comp_and_iter) {
                auto [composition, iterations] = comp_and_iter;
                return config::parse_shell_radii<"shell_radii">(
                           doc, sublattice_mode, std::forward<decltype(structure)>(structure),
                           composition)
                    .and_then([&](auto&& radii) {
                      return config::parse_shell_weights<"shell_weights", T>(doc, sublattice_mode,
                                                                             radii)
                          .and_then([&](auto&& weights) {
                            return config::parse_prefactors<"prefactors", T>(
                                       doc, sublattice_mode,
                                       std::forward<decltype(structure)>(structure), composition,
                                       radii, weights)
                                .combine(config::parse_pair_weights<"pair_weights", T>(
                                    doc, sublattice_mode,
                                    std::forward<decltype(structure)>(structure), composition,
                                    weights))
                                .combine(config::parse_target_objective<"target_objective", T>(
                                    doc, sublattice_mode,
                                    std::forward<decltype(structure)>(structure), composition,
                                    weights))
                                .combine(parse_chunk_size<"chunk_size">(doc, iterations))
                                .combine(parse_threads_per_rank<"threads_per_rank">(doc))
                                .and_then([&](auto&& arrays) -> parse_result<configuration<T>> {
                                  auto [prefactors, pair_weights, target_objective, chunk_size,
                                        thread_config]
                                      = arrays;
                                  return configuration<T>{
                                      sublattice_mode,
                                      iteration_mode,
                                      std::forward<structure_config<T>>(sc),
                                      std::forward<std::vector<sublattice>>(composition),
                                      std::forward<stl_matrix_t<T>>(radii),
                                      std::forward<std::vector<shell_weights_t<T>>>(weights),
                                      std::forward<std::vector<cube_t<T>>>(prefactors),
                                      std::forward<std::vector<cube_t<T>>>(pair_weights),
                                      std::forward<std::vector<cube_t<T>>>(target_objective),
                                      iterations,
                                      chunk_size,
                                      thread_config};
                                });
                          });
                    });
              });
        });
  }
}  // namespace sqsgen::io::config
#endif  // SQSGEN_IO_CONFIG_COMBINED_H
