
#ifndef SQSGEN_IO_GLAZE_H
#define SQSGEN_IO_GLAZE_H

#include <expected>
#include <glaze/cbor.hpp>
#include <glaze/glaze.hpp>
#include <glaze/msgpack.hpp>
#include <glaze/yaml.hpp>
#include <type_traits>

#include "sqsgen/types.h"

namespace sqsgen::io {

  namespace detail {

    template <class DTO>
    static glz::error_ctx read_any(DTO& dto, std::string_view data, wire_format fmt) {
      constexpr glz::opts opts{
          .error_on_unknown_keys = false,
          .error_on_missing_keys = true,
      };
      switch (fmt) {
        case wire_format::json:
          return glz::read<opts>(dto, data);
        case wire_format::yaml:
          return glz::read_yaml(dto, data);
        case wire_format::cbor:
          return glz::read_cbor(dto, data);
        case wire_format::msgpack:
          return glz::read_msgpack(dto, data);
        case wire_format::beve:
          return glz::read_beve(dto, data);
      }
      return {};  // unreachable; or an error_ctx for an invalid enum
    }

    parser_error forward_glaze_error(const glz::error_ctx& ec, std::string_view buffer);
  }  // namespace detail

  template <class DTO>
    requires std::is_default_constructible_v<DTO>
  static std::expected<DTO, parser_error> read_any(std::string_view data, wire_format fmt) {
    DTO dto{};
    if (auto ec = read_any(dto, data, fmt))
      return std::unexpected(detail::forward_glaze_error(ec, data));
    return dto;
  }

  template <class DTO>
  static std::expected<std::string, parser_error> write_any(DTO const& dto, wire_format fmt) {
    std::expected<std::string, glz::error_ctx> out;
    switch (fmt) {
      case wire_format::json:
        out = glz::write_json(dto);
        break;
      case wire_format::yaml:
        out = glz::write_yaml(dto);
        break;
      case wire_format::cbor:
        out = glz::write_cbor(dto);
        break;
      case wire_format::msgpack:
        out = glz::write_msgpack(dto);
        break;
      case wire_format::beve:
        out = glz::write_beve(dto);
        break;
    }
    if (!out) return std::unexpected(detail::forward_glaze_error(out.error(), {}));
    return std::move(*out);
  }
}  // namespace sqsgen::io

#endif  // SQSGEN_IO_GLAZE_H
