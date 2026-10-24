

#include "sqsgen/core/eigen.h"

namespace sqsgen::core {

  error_code map_error_kind(tensor_shape_error::kind kind) {
    switch (kind) {
      case (core::tensor_shape_error::kind::empty_dimension):
        return error_code::not_found;
      case (core::tensor_shape_error::kind::negative_extent):
        return error_code::bad_argument;
      case (core::tensor_shape_error::kind::size_mismatch):
        return error_code::invalid_size;
      case (core::tensor_shape_error::kind::ragged):
        return error_code::bad_argument;
      default:
        return error_code::unknown;
    }
  }
};  // namespace sqsgen::core
