

#include <expected>

#include "sqsgen/core/atom.h"
#include "sqsgen/core/eigen.h"
#include "sqsgen/core/helpers/expected.h"
#include "sqsgen/core/helpers/string.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::io {

  namespace helpers = sqsgen::core::helpers;

  namespace detail {

    template <class T> struct structure_dto {
      configuration_t species;
      core::packed_tensor<T, 2> lattice;
      core::packed_tensor<T, 2> coords;
      std::size_t num_atoms;
    };
  }  // namespace detail

  template <class T> std::expected<core::structure<T>, parser_error> to_structure(
      const detail::structure_dto<T>& data) {
    auto lattice_and_coords
        = helpers::combine(core::lattice_from(data.lattice), core::coords_from(data.coords));
    if (!lattice_and_coords) return std::unexpected(std::move(lattice_and_coords).error());
    const auto [lattice, coords] = lattice_and_coords;
    if (coords.rows() != data.num_atoms)
      return std::unexpected(parser_error{
          error_code::invalid_size, helpers::format_string("Found %i coords but counted %u sites",
                                                           coords.rows(), data.num_atoms)});
    if (data.species != data.num_atoms)
      return std::unexpected(parser_error{
          error_code::invalid_size, helpers::format_string("Found %u coords but counted %u sites",
                                                           data.species.size(), data.num_atoms)});
    for (const auto ordinal : data.species) {
      if (ordinal > core::KNOWN_ELEMENTS.size() || ordinal < 0)
        return std::unexpected(
            parser_error{error_code::invalid_size,
                         helpers::format_string("Unknown atomic element %u", ordinal)});
    }

    return core::structure<T>(lattice, coords, data.species);
  }

  template <class T> std::string structure_adapter<T, structure_format::sqsgen>::format(
      core::structure<T> const& structure, wire_format format) {
    auto data = write_any(
        detail::structure_dto<T>{
            structure.species, core::tensor_as<core::packed_tensor<T, 2>>(structure.lattice),
            core::tensor_as<core::packed_tensor<T, 2>>(structure.frac_coords), structure.size()},
        format);
    if (!data) throw std::runtime_error{"failed to serialize strucure"};
    return std::move(*data);
  }

  template <class T> std::expected<core::structure<T>, parser_error>
  structure_adapter<T, structure_format::sqsgen>::read(std::string_view data, wire_format format) {
    auto dto = read_any<detail::structure_dto<T>>(data, format);
    if (!dto) return std::unexpected(std::move(dto).error());
    return dto.and_then(to_structure);
  }

  template <> struct structure_adapter<float, structure_format::sqsgen>;
  template <> struct structure_adapter<double, structure_format::sqsgen>;

};  // namespace sqsgen::io
