

#include "sqsgen/io/glaze.h"

namespace sqsgen::io::detail {
  parser_error forward_glaze_error(const glz::error_ctx& ec, std::string_view buffer) {
    return parser_error{
        .code = error_code::parsing,
        .msg = glz::format_error(ec, buffer),
    };
  }
};  // namespace sqsgen::io::detail
