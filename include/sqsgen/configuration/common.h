

#ifndef SQSGEN_CONFIGURATION_COMMON_H
#define SQSGEN_CONFIGURATION_COMMON_H

#include <expected>
#include <string>

#include "sqsgen/types.h"

namespace sqsgen::configuration {

  template <class T> using result_t = std::expected<T, configuration_error>;

  template <string_literal Name, class Input, class Output> struct configuration_parse_base {
    using input_type = Input;
    using output_type = Output;

    std::expected<Output, configuration_error> fail(std::string const& msg,
                                                    error_code code = error_code::unknown) {
      return std::unexpected(
          configuration_error{.key = std::string(Name.c_str()), .code = code, .msg = msg});
    }
  };
}  // namespace sqsgen::configuration

#endif  // SQSGEN_CONFIGURATION_COMMON_H
