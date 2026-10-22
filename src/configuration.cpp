

#include "sqsgen/configuration.h"

#include "sqsgen/configuration/misc.h"
#include "sqsgen/io/glaze.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace detail {
    struct configuration_input_meta {
      prec_input prec;
      sublattice_mode sublattice_mode;
    };

    configuration_error forward_error(io::parser_error error) {
      return configuration_error{.key = "", .code = error.code, .msg = error.msg};
    }
  }  // namespace detail

  template <> struct configuration_input<float, sublattice_mode::interact>;
  template <> struct configuration_input<float, sublattice_mode::split>;
  template <> struct configuration_input<double, sublattice_mode::interact>;
  template <> struct configuration_input<double, sublattice_mode::split>;

  template <> struct configuration<float, sublattice_mode::interact>;
  template <> struct configuration<float, sublattice_mode::split>;
  template <> struct configuration<double, sublattice_mode::interact>;
  template <> struct configuration<double, sublattice_mode::split>;

  std::expected<std::variant<configuration_input<float, sublattice_mode::interact>,
                             configuration_input<float, sublattice_mode::split>,
                             configuration_input<double, sublattice_mode::interact>,
                             configuration_input<double, sublattice_mode::split>>,
                configuration_error>
  read_input(std::string_view, io::wire_format) {}

};  // namespace sqsgen::configuration
