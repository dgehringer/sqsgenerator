//
// Created by Dominik Gehringer on 02.12.24.
//

#ifndef SQSGEN_IO_CONFIG_SHELL_RADII_H
#define SQSGEN_IO_CONFIG_SHELL_RADII_H

#include "sqsgen/config.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/io/config/shared.h"
#include "sqsgen/io/parsing.h"

namespace sqsgen::io::config {

  using namespace sqsgen::core::helpers;

  template <class T> using radii_t = parse_result<stl_matrix_t<T>>;

  template <class T> static constexpr T atol_default = 1.0e-3;
  template <class T> static constexpr T rtol_default = 1.0e-5;
  template <class T> static constexpr T bin_width_default = 0.05;       // Angstrom
  template <class T> static constexpr T peak_isolation_default = 0.25;  // Angstrom

  namespace detail {
    template <class T> using accepted_types_t = parse_result<ShellRadiiDetection, std::vector<T>>;

    template <string_literal key, class T, class Document>
    parse_result<std::vector<T>> parse_radii(Document const& doc, accepted_types_t<T>&& value,
                                             core::structure<T>&& structure) {
      using out_t = parse_result<std::vector<T>>;
      return value.template collapse<std::vector<T>>(
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
                    return core::distances_naive(std::forward<core::structure<T>>(structure), atol,
                                                 rtol);
                  });
            }
            if (mode == SHELL_RADII_DETECTION_PEAK) {
              return get_optional<"bin_width", T>(doc)
                  .value_or(T(bin_width_default<T>))
                  .combine(
                      get_optional<"peak_isolation", T>(doc).value_or(T(peak_isolation_default<T>)))
                  .and_then([&](auto&& params) -> out_t {
                    auto [bin_width, peak_isolation] = params;
                    return core::distances_histogram(std::forward<core::structure<T>>(structure),
                                                     bin_width, peak_isolation);
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
            if (radii.front() != 0.0 && !core::helpers::is_close<T>(radii.front(), 0.0))
              radii.insert(radii.begin(), 0.0);
            return radii;
          });
    }

    template <string_literal, SublatticeMode, class> struct shell_radii_parser {};

    template <string_literal key, class T>

    struct shell_radii_parser<key, SUBLATTICE_MODE_INTERACT, T> {
      template <class Document>
      static radii_t<T> parse(Document const& doc, core::structure<T>&& structure) {
        return parse_radii<key, T>(
                   doc,
                   get_either_optional<key, ShellRadiiDetection, std::vector<T>>(doc).value_or(
                       detail::accepted_types_t<T>{SHELL_RADII_DETECTION_PEAK}),
                   std::forward<core::structure<T>>(structure))
            .and_then([&](auto&& radii) -> radii_t<T> { return stl_matrix_t<T>{radii}; });
      }
    };

    template <string_literal key, class T>
    struct shell_radii_parser<key, SUBLATTICE_MODE_SPLIT, T> {
      template <class Document>
      static radii_t<T> parse(Document const& doc, core::structure<T>&& structure,
                              std::vector<sublattice> const& sublattices) {
        if (accessor<Document>::contains(doc, key.data)) {
          // otherwise we expect to object to be a list and hold the radii spec. for each sl
          auto list = accessor<Document>::get(doc, key.data);
          using accessor_t = accessor<std::decay_t<decltype(list)>>;
          if (!accessor_t::is_list(list))
            return parse_error::from_msg<key, CODE_TYPE_ERROR>(
                "You want to run a split sublattice mode, and did not specify valid modes. You "
                "have to specify the shell radii per sublattice");
          auto default_radii = [&](auto&& subdoc, auto&& sublattice) {
            return parse_radii<key, T>(
                doc, get_either<KEY_NONE, ShellRadiiDetection, std::vector<T>>(subdoc),
                std::move(structure.sliced(sublattice.sites)));
          };
          return lift<key>(default_radii, accessor_t::range(list), sublattices);
        }
        // nothing is specified return the default value
        auto default_radii = [&](auto&& sublattice) -> parse_result<std::vector<T>> {
          return detail::parse_radii<key, T>(
              doc, detail::accepted_types_t<T>{SHELL_RADII_DETECTION_PEAK},
              std::move(structure.sliced(sublattice.sites)));
        };
        return lift<key>(default_radii, sublattices);
      }
    };
  }  // namespace detail

  template <string_literal key, class T, class Document>
  radii_t<T> parse_shell_radii(Document const& doc, core::structure<T>&& structure,
                               std::vector<sublattice> const& composition) {
    return parse_for_mode<key, Document>(
        [&] {
          return detail::shell_radii_parser<key, SUBLATTICE_MODE_INTERACT, T>::template parse(
              doc, std::forward<core::structure<T>>(structure));
        },
        [&] {
          return detail::shell_radii_parser<key, SUBLATTICE_MODE_SPLIT, T>::template parse(
              doc, std::forward<core::structure<T>>(structure), composition);
        }, doc);
  }

}  // namespace sqsgen::io::config

#endif  // SQSGEN_IO_CONFIG_SHELL_RADII_H
