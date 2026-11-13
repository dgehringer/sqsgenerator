
#ifndef SQSGEN_IO_GLAZE_H
#define SQSGEN_IO_GLAZE_H

#include <expected>
#include <glaze/cbor.hpp>
#include <glaze/core/common.hpp>
#include <glaze/glaze.hpp>
#include <glaze/msgpack.hpp>
#include <glaze/yaml.hpp>
#include <type_traits>

#include "sqsgen/core/eigen.h"
#include "sqsgen/types.h"

namespace sqsgen::io {

  namespace detail {

    template <class DTO, glz::opts opts = {
        .error_on_unknown_keys = false,
        .error_on_missing_keys = true,
    }>
    static glz::error_ctx read_any(DTO& dto, std::string_view data, wire_format fmt);

    parser_error forward_glaze_error(const glz::error_ctx& ec, std::string_view buffer);

  }  // namespace detail

  template <class DTO, glz::opts opts = {
      .error_on_unknown_keys = false,
      .error_on_missing_keys = true,
  }>
    requires std::is_default_constructible_v<DTO>
  static std::expected<DTO, parser_error> read_any(std::string_view data, wire_format fmt);

  template <class DTO, wire_format Format,
            glz::opts opts = {.error_on_unknown_keys = false, .error_on_missing_keys = true}>
  static std::expected<DTO, parser_error> read_format(std::string_view data);

  template <class DTO>
  static std::expected<std::string, parser_error> write_any(DTO const& dto, wire_format fmt);

  template <class DTO, wire_format Format>
  static std::expected<std::string, parser_error> write_format(DTO const& dto);

}  // namespace sqsgen::io

namespace glz {

  template <> struct meta<sqsgen::prec> {
    using enum sqsgen::prec;
    static constexpr auto value = glz::enumerate("single", Single, "double", Double);
  };

  template <> struct meta<sqsgen::error_code> {
    using enum sqsgen::error_code;
    static constexpr auto value
        = glz::enumerate(type_error, out_of_range, invalid_size, bad_argument, not_found, parsing);
  };

  template <> struct meta<sqsgen::sublattice_mode> {
    using enum sqsgen::sublattice_mode;
    static constexpr auto value = glz::enumerate(interact, split);
  };

  template <> struct meta<sqsgen::iteration_mode> {
    using enum sqsgen::iteration_mode;
    static constexpr auto value = glz::enumerate(random, systematic);
  };

  template <> struct meta<sqsgen::configuration::shell_radii_detection> {
    using enum sqsgen::configuration::shell_radii_detection;
    static constexpr auto value = glz::enumerate(naive, peak);
  };

}  // namespace glz

#include "sqsgen/impl/io/glaze_impl.hpp"

#endif  // SQSGEN_IO_GLAZE_H
