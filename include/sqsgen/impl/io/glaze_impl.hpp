
#ifndef SQSGEN_IMPL_IO_GLAZE_IMPL_HPP
#define SQSGEN_IMPL_IO_GLAZE_IMPL_HPP

#include "sqsgen/io/glaze.h"

namespace sqsgen::io {

  namespace detail {

    template <class DTO, glz::opts opts>
    static glz::error_ctx read_any(DTO& dto, std::string_view data, wire_format fmt) {
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
      return {};
    }

    inline parser_error forward_glaze_error(const glz::error_ctx& ec, std::string_view buffer) {
      return parser_error{
          .code = error_code::parsing,
          .msg = glz::format_error(ec, buffer),
      };
    }

  }  // namespace detail

  template <class DTO, glz::opts opts>
    requires std::is_default_constructible_v<DTO>
  static std::expected<DTO, parser_error> read_any(std::string_view data, wire_format fmt) {
    DTO dto{};
    if (auto ec = detail::read_any<DTO, opts>(dto, data, fmt))
      return std::unexpected(detail::forward_glaze_error(ec, data));
    return dto;
  }

  template <class DTO, wire_format Format, glz::opts opts>
  static std::expected<DTO, parser_error> read_format(std::string_view data) {
    DTO dto{};
    glz::error_ctx ec;

    if constexpr (Format == wire_format::json) {
      ec = glz::read<opts>(dto, data);
    } else if constexpr (Format == wire_format::yaml) {
      ec = glz::read_yaml(dto, data);
    } else if constexpr (Format == wire_format::cbor) {
      ec = glz::read_cbor(dto, data);
    } else if constexpr (Format == wire_format::msgpack) {
      ec = glz::read_msgpack(dto, data);
    } else if constexpr (Format == wire_format::beve) {
      ec = glz::read_beve(dto, data);
    } else {
      static_assert(Format == wire_format::json, "unhandled wire_format");
    }

    if (ec) return std::unexpected(detail::forward_glaze_error(ec, data));
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

  template <class DTO, wire_format Format>
  static std::expected<std::string, parser_error> write_format(DTO const& dto) {
    std::expected<std::string, glz::error_ctx> out;

    if constexpr (Format == wire_format::json) {
      out = glz::write_json(dto);
    } else if constexpr (Format == wire_format::yaml) {
      out = glz::write_yaml(dto);
    } else if constexpr (Format == wire_format::cbor) {
      out = glz::write_cbor(dto);
    } else if constexpr (Format == wire_format::msgpack) {
      out = glz::write_msgpack(dto);
    } else if constexpr (Format == wire_format::beve) {
      out = glz::write_beve(dto);
    } else {
      static_assert(Format == wire_format::json, "unhandled wire_format");
    }

    if (!out) return std::unexpected(detail::forward_glaze_error(out.error(), {}));
    return std::move(*out);
  }

}  // namespace sqsgen::io

#endif  // SQSGEN_IMPL_IO_GLAZE_IMPL_HPP
