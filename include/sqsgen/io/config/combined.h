//
// Created by Dominik Gehringer on 04.01.25.
//

#ifndef SQSGEN_IO_CONFIG_COMBINED_H
#define SQSGEN_IO_CONFIG_COMBINED_H
#include "sqsgen/core/config.h"
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

  template <string_literal key, class Document, class T>
  parse_result<std::optional<iterations_t>> parse_iterations(
      Document const& doc, core::structure<T>&& structure,
      std::vector<sublattice> const& composition, IterationMode mode) {
    using result_t = parse_result<std::optional<iterations_t>>;
    if (mode == ITERATION_MODE_SYSTEMATIC) {
      if (composition.size() != 1)
        return parse_error::from_msg<key, CODE_BAD_VALUE>(
            "In \"systematic\" iteration mode only one sublattice is allowed");
      auto iterations = core::num_permutations(
          structure.apply_composition_and_decompose(composition).front().species);
      if (iterations > std::numeric_limits<iterations_t>::max())
        return parse_error::from_msg<key, CODE_BAD_VALUE>(
            std::format("The number of permutations to test is {}. I'm a pretty fast programm, but "
                        "this is too much even for me ;=)",
                        iterations.to_string()));
      return std::make_optional(iterations_t{iterations});
    } else
      return get_optional<key, iterations_t>(doc)
          .value_or(parse_result<iterations_t>{iterations_default})
          .and_then([](auto&& it) -> result_t { return result_t{it}; });
  }

  template <string_literal key, class Document>
  parse_result<thread_config_t> parse_threads_per_rank(Document const& doc) {
    using result_t = parse_result<thread_config_t>;
    return get_either_optional<key, usize_t, std::vector<usize_t>>(doc)
        .value_or(parse_result<usize_t, std::vector<usize_t>>{usize_t(0)})
        .template collapse<thread_config_t>(
            [&](usize_t&& num_threads) -> result_t { return {std::vector{num_threads}}; },
            [&](std::vector<usize_t>&& num_threads) -> result_t { return {num_threads}; });
  }

  template <string_literal key, class Document>
  parse_result<iterations_t> parse_chunk_size(Document const& doc,
                                              std::optional<iterations_t> iterations) {
    using result_t = parse_result<iterations_t>;
    return get_optional<key, iterations_t>(doc)
        .value_or(parse_result<iterations_t>{iterations.has_value()
                                                 ? std::min(iterations.value(), chunk_size_default)
                                                 : chunk_size_default})
        .and_then([&](auto&& cs) -> result_t {
          if (iterations.has_value() && cs > iterations.value())
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

  template <string_literal key, class Document>
  parse_result<Prec> parse_precision(Document const& doc) {
    return get_optional<"prec", Prec>(doc)
        .value_or(parse_result<Prec>{Prec::PREC_DOUBLE})
        .and_then([&](auto&& prec) -> parse_result<Prec> {
          if (prec == Prec::PREC_INVALID)
            return parse_error::from_msg<key, CODE_BAD_VALUE>(
                "Invalid precision mode. Must be either \"double\" (double) or \"single\" "
                "(float)");
          return prec;
        });
    ;
  }

  template <string_literal key, class Document>
  parse_result<std::size_t> parse_keep(Document const& doc) {
    return get_optional<key, std::size_t>(doc)
        .value_or(parse_result<std::size_t>{1UL})
        .and_then([](auto&& keep) -> parse_result<std::size_t> {
          if (keep == 0)
            return parse_error::from_msg<key, CODE_BAD_VALUE>(
                "The number of best kept structures must be greater than 0");
          return keep;
        });
  }

  template <class T, class Document>
  parse_result<configuration<T>> parse_config_for_prec(Document const& doc) {
    return parse_iteration_mode<"iteration_mode">(doc)
        .combine(parse_sublattice_mode<"sublattice_mode">(doc))
        .and_then([](auto&& modes) -> parse_result<std::tuple<IterationMode, SublatticeMode>> {
          auto [iteration_mode, sublattice_mode] = modes;
          if (iteration_mode == ITERATION_MODE_SYSTEMATIC
              && sublattice_mode == SUBLATTICE_MODE_SPLIT)
            return {parse_error::from_msg<"iteration_mode", CODE_BAD_VALUE>(
                "It is not possible to do an exhaustive search on multiple sublattices in mode "
                "\"split\"")};
          return modes;
        })
        .combine(parse_structure_config<"structure", T>(doc))
        .and_then([&](auto&& sc_and_modes) {
          auto [iteration_mode, sublattice_mode, sc] = sc_and_modes;
          auto structure = sc.structure();
          return config::parse_composition<"composition", "sites">(doc, structure.species,
                                                                   sublattice_mode)
              .and_then([&](auto&& composition) {
                return config::parse_shell_radii<"shell_radii">(
                           doc, sublattice_mode, std::forward<core::structure<T>>(structure),
                           composition)
                    .combine(parse_iterations<"iterations">(
                        doc, std::forward<core::structure<T>>(structure), composition,
                        iteration_mode))
                    .and_then([&](auto&& radii_and_iterations) {
                      auto [radii, iterations] = radii_and_iterations;
                      return config::parse_shell_weights<"shell_weights", T>(doc, sublattice_mode,
                                                                             radii)
                          .and_then([&](auto&& weights) {
                            return config::parse_prefactors<"prefactors", T>(
                                       doc, sublattice_mode,
                                       std::forward<core::structure<T>>(structure), composition,
                                       radii, weights)
                                .combine(config::parse_pair_weights<"pair_weights", T>(
                                    doc, sublattice_mode,
                                    std::forward<core::structure<T>>(structure), composition,
                                    weights))
                                .combine(config::parse_target_objective<"target_objective", T>(
                                    doc, sublattice_mode,
                                    std::forward<core::structure<T>>(structure), composition,
                                    weights))
                                .combine(parse_chunk_size<"chunk_size">(doc, iterations))
                                .combine(parse_threads_per_rank<"threads_per_rank">(doc))
                                .combine(parse_keep<"keep">(doc))
                                .and_then([&](auto&& arrays) -> parse_result<configuration<T>> {
                                  auto [prefactors, pair_weights, target_objective, chunk_size,
                                        thread_config, to_keep]
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
                                      thread_config,
                                      to_keep};
                                });
                          });
                    });
              });
        });
  }

  template <class Document>
  parse_result<configuration<float>, configuration<double>> parse_config(Document const& doc) {
    using result_t = parse_result<configuration<float>, configuration<double>>;
    return parse_precision<"prec">(doc).and_then([&](auto&& prec) -> result_t {
      if (prec == PREC_DOUBLE)
        return parse_config_for_prec<double, Document>(doc).and_then(
            [](auto&& config) -> result_t { return {config}; });
      if (prec == PREC_SINGLE)
        return parse_config_for_prec<float>(doc).and_then(
            [](auto&& config) -> result_t { return {config}; });
      throw std::runtime_error("Unsupported precision");
    });
  }
}  // namespace sqsgen::io::config
#endif  // SQSGEN_IO_CONFIG_COMBINED_H
