//
// Created by Dominik Gehringer on 02.12.24.
//

#ifndef SQSGEN_IO_CONFIG_SETTINGS_H
#define SQSGEN_IO_CONFIG_SETTINGS_H
#include <nlohmann/json.hpp>

#include "io/json.h"
#include "structure.h"
#include "types.h"

namespace sqsgen::io::config {

  using namespace sqsgen::core::helpers;

  template <class T> static constexpr T atol_default = 1.0e-3;
  template <class T> static constexpr T rtol_default = 1.0e-5;
  template <class T> static constexpr T bin_width_default = 0.05;       // Angstrom
  template <class T> static constexpr T peak_isolation_default = 0.25;  // Angstrom

  template <class T> struct settings {
    shell_weights_t<T> shell_weights;
    std::vector<T> shells;
  };

  template <string_literal key, class T, class Document>
  parse_result<std::vector<T>> parse_shell_radii(Document const& doc,
                                                 structure_config<T> const& structure) {
    using out_t = parse_result<std::vector<T>>;
    return get_either_optional<key, ShellDetectionMode, std::vector<T>>(doc)
        .value_or(
            parse_result<ShellDetectionMode, std::vector<T>>{SHELL_DETECTION_MODE_PEAK_ISOLATION})
        .template collapse<std::vector<T>>(
            [&](ShellDetectionMode&& mode) -> out_t {
              if (mode == SHELL_DETECTION_MODE_INVALID) {
                return parse_error::from_msg<key, CODE_BAD_VALUE>(
                    R"(Invalid shell radii detection mode must be either "naive" or "peak")");
              }
              if (mode == SHELL_DETECTION_MODE_NAIVE) {
                return get_optional<"atol", T>(doc)
                    .value_or(T(atol_default<T>))
                    .combine(get_optional<"rtol", T>(doc).value_or(T(rtol_default<T>)))
                    .and_then([&](auto&& params) -> out_t {
                      auto [atol, rtol] = params;
                      return core::distances_naive(structure.structure(), atol, rtol);
                    });
              }
              if (mode == SHELL_DETECTION_MODE_PEAK_ISOLATION) {
                get_optional<"bin_width", T>(doc)
                    .value_or(T(bin_width_default<T>))
                    .combine(get_optional<"peak_isolation", T>(doc).value_or(
                        T(peak_isolation_default<T>)))
                    .and_then([&](auto&& params) -> out_t {
                      auto [bin_width, peak_isolation] = params;
                      return core::distances_histogram(structure.structure(), bin_width,
                                                       peak_isolation);
                    });
              }
              return std::vector<T>{};
            },
            [&](std::vector<T>&& radii) -> out_t {
              if (radii.empty())
                return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
                    "You need to define at least one coordination shells");
              return radii;
            });

    return std::vector<T>{};
  }

}  // namespace sqsgen::io::config

#endif  // SQSGEN_IO_CONFIG_SETTINGS_H
