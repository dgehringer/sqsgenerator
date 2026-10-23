

#include "sqsgen/configuration.h"

#include <expected>

#include "sqsgen/configuration/misc.h"
#include "sqsgen/io/glaze.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  template <> struct configuration_input<float, sublattice_mode::interact>;
  template <> struct configuration_input<float, sublattice_mode::split>;
  template <> struct configuration_input<double, sublattice_mode::interact>;
  template <> struct configuration_input<double, sublattice_mode::split>;

  template <> struct configuration<float, sublattice_mode::interact>;
  template <> struct configuration<float, sublattice_mode::split>;
  template <> struct configuration<double, sublattice_mode::interact>;
  template <> struct configuration<double, sublattice_mode::split>;

  namespace detail {
    struct configuration_input_meta {
      std::optional<prec> prec;
      std::optional<sublattice_mode> sublattice_mode;
    };

    configuration_error to_configuration_error(io::parser_error error) {
      return configuration_error{.key = "", .code = error.code, .msg = error.msg};
    }

    io::parser_error to_parser_error(configuration_error error) {
      return io::parser_error{.code = error.code, .msg = error.msg};
    }

    template <class T, sublattice_mode Mode>
    std::expected<configuration_variant_t, io::parser_error> read_input_specific(
        std::string_view data, io::wire_format fmt) {
      auto parsed = io::read_any<configuration_input<T, Mode>, glz::opts{
                                                                   .error_on_unknown_keys = true,
                                                                   .error_on_missing_keys = false,
                                                               }>(data, fmt);
      if (!parsed) return std::unexpected(std::move(parsed).error());
      return std::move(parsed).value();
    }
  }  // namespace detail

  std::expected<configuration_variant_t, io::parser_error> read_input(std::string_view data,
                                                                      io::wire_format fmt) {
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
      return detail::read_input_specific<float, sublattice_mode::interact>(data, fmt);
    else if (prec == prec::Double && smode == sublattice_mode::split)
      return detail::read_input_specific<double, sublattice_mode::interact>(data, fmt);
    else
      return std::unexpected(io::parser_error{.code = error_code::unknown, .msg = "Unknown error"});
  }
};  // namespace sqsgen::configuration
