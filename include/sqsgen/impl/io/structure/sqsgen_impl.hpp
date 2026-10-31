//
// Created by Dominik Gehringer on 04.03.25.
//

#ifndef SQSGEN_IMPL_IO_STRUCTURE_SQSGEN_IMPL_HPP
#define SQSGEN_IMPL_IO_STRUCTURE_SQSGEN_IMPL_HPP

#include <expected>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "sqsgen/core/structure.h"
#include "sqsgen/io/glaze.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::io {

  template <class T> std::string structure_adapter<T, structure_format::sqsgen>::format(
      const core::structure<T>& structure, wire_format format) {
    auto data = write_any(structure.template to_dto<core::array_mode::packed>(), format);
    if (!data) throw std::runtime_error{"failed to serialize strucure"};
    return std::move(*data);
  }

  template <class T> std::expected<core::structure<T>, parser_error>
  structure_adapter<T, structure_format::sqsgen>::read(std::string_view data, wire_format format) {
    auto dto = read_any<core::structure_dto<T, core::array_mode::packed>>(data, format);
    if (!dto) return std::unexpected(std::move(dto).error());
    return dto.and_then(core::structure<T>::template from_dto<core::array_mode::packed>);
  }

}  // namespace sqsgen::io

#endif  // SQSGEN_IMPL_IO_STRUCTURE_SQSGEN_IMPL_HPP
