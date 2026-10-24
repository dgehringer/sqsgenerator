#include <array>
#include <expected>
#include <glaze/glaze.hpp>
#include <glaze/json/generic_fwd.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "sqsgen/core/atom.h"
#include "sqsgen/core/eigen.h"
#include "sqsgen/core/helpers/inst.h"
#include "sqsgen/core/helpers/string.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/glaze.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::io {

  namespace core = sqsgen::core;
  namespace helpers = sqsgen::core::helpers;
  namespace detail {
    template <class T> struct pymatgen_species {
      std::string element;
      T occu;
    };

    template <class T> struct pymatgen_lattice {
      core::nested_t<T, 2> matrix;
      std::array<bool, 3> pbc;
      T a, b, c;
      T volume;
      T alpha, beta, gamma;
    };

    template <class T> struct pymatgen_site {
      std::array<T, 3> abc;
      std::array<T, 3> xyz;
      std::string label;
      std::vector<pymatgen_species<T>> species;
    };

    template <class T> struct pymatgen_structure {
      glz::generic properties;
      pymatgen_lattice<T> lattice;
      std::string module{"pymatgen.core.structure"};
      std::string class_{"Structure"};
      T charge{0};
      std::vector<pymatgen_site<T>> sites;
    };

#define INST_DETAIL_PYMATGEN_SPECIES(PREC) template struct pymatgen_species<PREC>;
    SQSGEN_INSTANTIATE_PREC(INST_DETAIL_PYMATGEN_SPECIES)
#undef INST_DETAIL_PYMATGEN_SPECIES

#define INST_DETAIL_PYMATGEN_LATTICE(PREC) template struct pymatgen_lattice<PREC>;
    SQSGEN_INSTANTIATE_PREC(INST_DETAIL_PYMATGEN_LATTICE)
#undef INST_DETAIL_PYMATGEN_LATTICE

#define INST_DETAIL_PYMATGEN_SITE(PREC) template struct pymatgen_site<PREC>;
    SQSGEN_INSTANTIATE_PREC(INST_DETAIL_PYMATGEN_SITE)
#undef INST_DETAIL_PYMATGEN_SITE

#define INST_DETAIL_PYMATGEN_STRUCTURE(PREC) template struct pymatgen_structure<PREC>;
    SQSGEN_INSTANTIATE_PREC(INST_DETAIL_PYMATGEN_STRUCTURE)
#undef INST_DETAIL_PYMATGEN_STRUCTURE
  }  // namespace detail

  template <class T> std::array<T, 3> to_array(const Eigen::Matrix<T, 3, 1>& v) {
    return {v(0), v(1), v(2)};
  }

  parser_error forward_error(core::tensor_shape_error err) {
    return {map_error_kind(err.which), err.message};
  }

  template <class T>
  detail::pymatgen_structure<T> to_pymatgen(core::structure<T> const& structure) {
    auto filtered = structure.without_vacancies();
    const auto [a, b, c] = detail::lengths(filtered.lattice);
    const auto [alpha, beta, gamma] = detail::angles<T>(filtered.lattice);

    detail::pymatgen_structure<T> out;
    out.lattice = detail::pymatgen_lattice<T>{
        .matrix = core::tensor_as<core::array_mode::nested>(filtered.lattice),
        .pbc = filtered.pbc,
        .a = a,
        .b = b,
        .c = c,
        .volume = std::abs(filtered.lattice.determinant()),
        .alpha = alpha,
        .beta = beta,
        .gamma = gamma,
    };

    const auto make_site = [&](auto&& site) -> detail::pymatgen_site<T> {
      return detail::pymatgen_site<T>{
          .abc = to_array<T>(site.frac_coords),
          .xyz = to_array<T>(filtered.lattice.transpose() * site.frac_coords),
          .label = helpers::format_string("%s%i", site.atom().symbol, site.index),
          .species = {detail::pymatgen_species<T>{site.atom().symbol, 1}}};
    };
    for (auto&& site : filtered.sites()) out.sites.push_back(make_site(site));
    return out;
  }

  template <class T>
  std::expected<std::pair<specie_t, std::array<T, 3>>, parser_error> convert_site(
      const detail::pymatgen_site<T>& site) {
    if (site.species.size() != 1)
      return std::unexpected(
          parser_error{error_code::out_of_range,
                       helpers::format_string("Cannot handle site with more than one species %u",
                                              site.species.size())});
    auto specie = site.species.front();
    if (!core::SYMBOL_MAP.contains(specie.element))
      return std::unexpected(parser_error{
          error_code::out_of_range, helpers::format_string("Unkown species %s", specie.element)});
    return std::make_pair(core::atom::from_symbol(specie.element).Z, site.abc);
  }

  template <class T> std::expected<core::structure<T>, parser_error> to_structure(
      const detail::pymatgen_structure<T>& dto) {
    const auto n = dto.sites.size();
    if (n == 0)
      return std::unexpected(
          parser_error{error_code::invalid_size, "Cannot load an empty structure"});
    configuration_t species;
    species.reserve(n);
    coords_t<T> frac_coords(n, 3);

    int index = 0;
    for (const auto& site : dto.sites) {
      auto site_result = convert_site(site);
      if (!site_result) return std::unexpected(std::move(site_result).error());
      auto [specie, fc] = std::move(site_result).value();
      species.push_back(specie);
      auto [a, b, c] = fc;
      frac_coords.row(index++) = Eigen::Vector3<T>{a, b, c};
    }

    auto lattice = core::lattice_from<T>(dto.lattice.matrix);
    if (!lattice) return std::unexpected(forward_error(std::move(lattice).error()));

    return core::structure<T>(std::move(lattice).value(), frac_coords, species);
  }

  template <class T> std::string structure_adapter<T, structure_format::pymatgen>::format(
      const core::structure<T>& structure, wire_format format) {
    auto data = write_any(to_pymatgen<T>(structure), format);
    if (!data) throw std::runtime_error{"failed to serialize strucure"};
    return std::move(*data);
  }

  template <class T> std::expected<core::structure<T>, parser_error>
  structure_adapter<T, structure_format::pymatgen>::read(std::string_view data,
                                                         wire_format format) {
    auto dto = read_any<detail::pymatgen_structure<T>>(data, format);
    if (!dto) return std::unexpected(std::move(dto).error());
    return dto.and_then(to_structure<T>);
  }

#define INST_STRUCTURE_ADAPTER_PYMATGEN(PREC) \
  template struct structure_adapter<PREC, structure_format::pymatgen>;
  SQSGEN_INSTANTIATE_PREC(INST_STRUCTURE_ADAPTER_PYMATGEN)
#undef INST_STRUCTURE_ADAPTER_PYMATGEN
}  // namespace sqsgen::io

namespace glz {
  template <class T> struct meta<sqsgen::io::detail::pymatgen_structure<T>> {
    using S = sqsgen::io::detail::pymatgen_structure<T>;
    static constexpr auto value
        = glz::object("properties", &S::properties, "lattice", &S::lattice, "@module", &S::module,
                      "@class", &S::class_, "charge", &S::charge, "sites", &S::sites);
  };

#define INST_DETAIL_PYMATGEN_STRUCTURE_META(PREC) \
  template struct meta<sqsgen::io::detail::pymatgen_structure<PREC>>;
  SQSGEN_INSTANTIATE_PREC(INST_DETAIL_PYMATGEN_STRUCTURE_META)
#undef INST_DETAIL_PYMATGEN_STRUCTURE_META

}  // namespace glz
