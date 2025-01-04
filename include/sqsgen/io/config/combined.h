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

  template <string_literal key, class Document>
  parse_result<IterationMode> parse_mode(Document const& document) {
    using result_t = parse_result<IterationMode>;
    return get_optional<key, IterationMode>(document)
        .value_or(parse_result<IterationMode>{ITERATION_MODE_RANDOM})
        .and_then([&](auto&& mode) -> result_t {
          return parse_for_mode<key>(
              [&]() -> result_t { return result_t{mode}; },
              [&]() -> result_t {
                if (mode == ITERATION_MODE_SYSTEMATIC)
                  return parse_error::from_msg<key, CODE_BAD_VALUE>("IterationMode");
                return result_t{mode};
              },
              document);
        });
  }

  template <class T, class Document>
  parse_result<configuration<T>> parse_config(Document const& doc) {
    return config::parse_structure_config<"structure", T>(doc).and_then([&](auto&& sc) {
      auto structure = sc.structure();
      return config::parse_composition<"composition", "sites">(doc, structure.species)
          .and_then([&](auto&& composition) {
            return config::parse_shell_radii<"shell_radii">(
                       doc, std::forward<decltype(structure)>(structure), composition)
                .and_then([&](auto&& radii) {
                  return config::parse_shell_weights<"shell_weights", double>(doc, radii)
                      .and_then([&](auto&& weights) {
                        return config::parse_prefactors<"prefactors", T>(
                                   doc, std::forward<decltype(structure)>(structure), composition,
                                   radii, weights)
                            .combine(config::parse_pair_weights<"pair_weights", T>(
                                doc, std::forward<decltype(structure)>(structure), composition,
                                weights))
                            .combine(config::parse_target_objective<"target_objective", T>(
                                doc, std::forward<decltype(structure)>(structure), composition,
                                weights))
                            .combine(parse_mode<"mode">(doc))
                            .and_then([&](auto&& arrays) -> parse_result<configuration<T>> {
                              auto [prefactors, pair_weights, target_objective, mode] = arrays;
                              return configuration<T>{
                                  std::forward<structure_config<T>>(sc),
                                  std::forward<std::vector<sublattice>>(composition),
                                  std::forward<stl_matrix_t<T>>(radii),
                                  std::forward<std::vector<shell_weights_t<T>>>(weights),
                                  std::forward<std::vector<cube_t<T>>>(prefactors),
                                  std::forward<std::vector<cube_t<T>>>(pair_weights),
                                  std::forward<std::vector<cube_t<T>>>(target_objective),
                                  mode,
                              };
                            });
                      });
                });
          });
    });
  }
}  // namespace sqsgen::io::config
#endif  // SQSGEN_IO_CONFIG_COMBINED_H
