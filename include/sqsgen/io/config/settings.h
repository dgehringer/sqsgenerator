//
// Created by Dominik Gehringer on 02.12.24.
//

#ifndef SQSGEN_IO_CONFIG_SETTINGS_H
#define SQSGEN_IO_CONFIG_SETTINGS_H

#include "sqsgen/config.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/io/parsing.h"

namespace sqsgen::io::config {

  using namespace sqsgen::core::helpers;

  template <class T> static constexpr T atol_default = 1.0e-3;
  template <class T> static constexpr T rtol_default = 1.0e-5;
  template <class T> static constexpr T bin_width_default = 0.05;       // Angstrom
  template <class T> static constexpr T peak_isolation_default = 0.25;  // Angstrom

  template <string_literal key, class T, class Document>
  parse_result<shell_weights_t<T>> parse_shell_weights(Document const& doc, auto num_shells) {
    if (!accessor<decltype(doc)>::is_document(doc))
      parse_error::from_msg<key, CODE_BAD_VALUE>("\"shell_weights\" must be a JSON object");
    shell_weights_t<T> weights;
    for (auto&& [si, w] : accessor<Document>::items(doc)) {
      std::optional<usize_t> shell_index;
      try {
        shell_index = std::stoul(si);
      } catch (std::invalid_argument const& e) {
        return parse_error::from_msg<key, CODE_BAD_VALUE>(
            std::format("Could not parse shell index: {}", e.what()));
      } catch (std::out_of_range const& e) {
        return parse_error::from_msg<key, CODE_BAD_VALUE>(
            std::format("Could not parse shell index: {}", e.what()));
      }
      if (shell_index == 0)
        return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
            "There is not point int including self-interactions");
      if (shell_index >= num_shells)
        return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(std::format(
            "There are {} shells in total. You passed {} as index.", num_shells, shell_index));
      assert(shell_index.has_value());
      auto weight = accessor<decltype(doc)>::template get_as<KEY_NONE, int>(w);
      if (weight.failed()) return weights.error().with_key(key.data);
    }
  }

  template <string_literal key, class T, class Document>
  parse_result<std::vector<T>> parse_shell_radii(Document const& doc,
                                                 structure_config<T> const& structure) {
    using out_t = parse_result<std::vector<T>>;
    return get_either_optional<key, ShellRadiiDetection, std::vector<T>>(doc)
        .value_or(parse_result<ShellRadiiDetection, std::vector<T>>{SHELL_RADII_DETECTION_PEAK})
        .template collapse<std::vector<T>>(
            [&](ShellRadiiDetection&& mode) -> out_t {
              if (mode == SHELL_RADII_DETECTION_INVALID) {
                return parse_error::from_msg<key, CODE_BAD_VALUE>(
                    R"(Invalid shell radii detection mode must be either "naive" or "peak")");
              }
              if (mode == SHELL_RADII_DETECTION_NAIVE) {
                return get_optional<"atol", T>(doc)
                    .value_or(T(atol_default<T>))
                    .combine(get_optional<"rtol", T>(doc).value_or(T(rtol_default<T>)))
                    .and_then([&](auto&& params) -> out_t {
                      auto [atol, rtol] = params;
                      return core::distances_naive(std::move(structure.structure()), atol, rtol);
                    });
              }
              if (mode == SHELL_RADII_DETECTION_PEAK) {
                return get_optional<"bin_width", T>(doc)
                    .value_or(T(bin_width_default<T>))
                    .combine(get_optional<"peak_isolation", T>(doc).value_or(
                        T(peak_isolation_default<T>)))
                    .and_then([&](auto&& params) -> out_t {
                      auto [bin_width, peak_isolation] = params;
                      return core::distances_histogram(std::move(structure.structure()), bin_width,
                                                       peak_isolation);
                    });
              }
              return std::vector<T>{};
            },
            [&](std::vector<T>&& radii) -> out_t {
              if (radii.empty())
                return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
                    "You need to define at least one coordination shells");
              for (auto&& r : radii)
                if (r < 0)
                  return parse_error::from_msg<key, CODE_BAD_VALUE>(
                      std::format("You cannot specify a shell radius that is less than 0 ({})", r));
              if (radii.front() != 0.0 && !core::helpers::is_close(radii.front(), 0.0))
                radii.insert(radii.begin(), 0.0);
              return radii;
            });
  }

}  // namespace sqsgen::io::config

#endif  // SQSGEN_IO_CONFIG_SETTINGS_H
