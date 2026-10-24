

#include <expected>

#include "sqsgen/core/structure.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::io {

  namespace core = sqsgen::core;

  template <class T> std::string structure_adapter<T, structure_format::sqsgen>::format(
      const core::structure<T>& structure, wire_format format) {
    auto data = write_any(structure.template to_dto<T, core::array_mode::packed>(), format);
    if (!data) throw std::runtime_error{"failed to serialize strucure"};
    return std::move(*data);
  }

  template <class T> std::expected<core::structure<T>, parser_error>
  structure_adapter<T, structure_format::sqsgen>::read(std::string_view data, wire_format format) {
    auto dto = read_any<core::structure_dto<T, core::array_mode::packed>>(data, format);
    if (!dto) return std::unexpected(std::move(dto).error());
    return dto.and_then(core::structure<T>::template from_dto<core::array_mode::packed>);
  }

  template <> struct structure_adapter<float, structure_format::sqsgen>;
  template <> struct structure_adapter<double, structure_format::sqsgen>;

};  // namespace sqsgen::io
