#include <array>
#include <expected>
#include <glaze/glaze.hpp>
#include <glaze/json/generic_fwd.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "sqsgen/core/atom.h"
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
  }  // namespace detail

  template <class T>
  detail::pymatgen_structure<T> to_pymatgen(core::structure<T> const& structure) {
    auto filtered = structure.without_vacancies();
    const auto [a, b, c] = detail::lengths(filtered.lattice);
    const auto [alpha, beta, gamma] = detail::angles<T>(filtered.lattice);

    detail::pymatgen_structure<T> out;
    out.lattice = {
        .matrix = filtered.lattice,
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
      detail::pymatgen_site{site.frac_coords, filtered.lattice.transpose() * site.frac_coords,
                            format_string("%s%i", site.atom().symbol, site.index), glz::generic{},
                            detail::pymatgen_species{site.atom().symbol, 1}};
    };
    for (auto&& site : filtered.sites()) out.sites.push_back(make_site(site));
    return out;
  }

  template <class T>
  std::expected<std::pair<std::string, std::array<T, 3>>, parser_error> convert_site(
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
      auto [specie, fc] = *site_result;
      species.push_back(specie);
      auto [a, b, c] = fc;
      frac_coords.row(index++) = Eigen::Vector3<T>{a, b, c};
    }

    return core::structure<T>(dto.lattice.matrix, frac_coords, species);
  }

  template <class T> std::string structure_adapter<T, structure_format::pymatgen>::format(
      core::structure<T> const& structure, wire_format format) {
    auto data = write_any(to_pymatgen(structure), format);
    if (!data) throw std::runtime_error{"failed to serialize strucure"};
    return std::move(*data);
  }

  template <class T> std::expected<core::structure<T>, parser_error>
  structure_adapter<T, structure_format::pymatgen>::read(std::string_view data,
                                                         wire_format format) {
    auto dto = read_any<detail::pymatgen_structure<T>>(data, format);
    if (!dto) return std::unexpected(std::move(dto).error());
    return dto.and_then(to_structure);
  }

  template <> struct structure_adapter<float, structure_format::pymatgen>;
  template <> struct structure_adapter<double, structure_format::pymatgen>;
}  // namespace sqsgen::io

namespace glz {
  template <class T> struct meta<sqsgen::io::detail::pymatgen_structure<T>> {
    using S = sqsgen::io::detail::pymatgen_structure<T>;
    static constexpr auto value
        = glz::object("properties", &S::properties, "lattice", &S::lattice, "@module", &S::module,
                      "@class", &S::class_, "charge", &S::charge, "sites", &S::sites);
  };

  template <> struct meta<sqsgen::io::detail::pymatgen_structure<float>>;
  template <> struct meta<sqsgen::io::detail::pymatgen_structure<double>>;
}  // namespace glz
