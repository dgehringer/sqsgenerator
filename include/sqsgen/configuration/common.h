

#ifndef SQSGEN_CONFIGURATION_COMMON_H
#define SQSGEN_CONFIGURATION_COMMON_H

#include <expected>
#include <string>

#include "absl/strings/str_format.h"
#include "sqsgen/core/helpers/static_string.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  template <class... Args>
  std::string format_string(const absl::FormatSpec<Args...>& fmt, Args&&... args) {
    return absl::StrFormat(fmt, std::forward<Args>(args)...);
  }

  template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
  };

  enum class configuration_parameter { structure };

  template <size_t N> using string_literal = core::helpers::string_literal<N>;

  enum configuration_error_code_ {
    CODE_UNKNOWN = -1,
    CODE_NOT_FOUND = 0,
    CODE_TYPE_ERROR = 1,
    CODE_OUT_OF_RANGE = 2,
    CODE_BAD_VALUE = 3,
    CODE_BAD_ARGUMENT = 4,
  };

  struct configuration_error {
    std::string key;
    error_code code;
    std::string msg;
  };

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
