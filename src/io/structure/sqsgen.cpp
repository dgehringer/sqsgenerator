

#include <expected>

#include "sqsgen/core/helpers/inst.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/glaze.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::io {

  namespace core = sqsgen::core;

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

#define INST_STRUCTURE_ADAPTER_SQSGEN(PREC) \
  template struct structure_adapter<PREC, structure_format::sqsgen>;
  SQSGEN_INSTANTIATE_PREC(INST_STRUCTURE_ADAPTER_SQSGEN)
#undef INST_STRUCTURE_ADAPTER_SQSGEN

};  // namespace sqsgen::io
