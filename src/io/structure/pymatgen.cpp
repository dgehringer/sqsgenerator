#include <glaze/glaze.hpp>
#include <glaze/json/generic_fwd.hpp>
#include <string>
#include <vector>

#include "sqsgen/core/eigen.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::io {

  namespace detail {
    template <class T> struct pymatgen_species {
      std::string element;
      T occu;
    };

    template <class T> struct pymatgen_lattice {
      nested_t<T, 2> matrix;  // whatever Glaze-serializable form your lattice maps to
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

  template <class T> struct structure_adapter_<T, structure_format::json_pymatgen> {
    static detail::pymatgen_structure<T> to_pymatgen(core::structure<T> const& structure) {
      auto filtered = structure.without_vacancies();
      const auto [a, b, c] = detail::lengths<T>(filtered.lattice);
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

    static std::string format(core::structure<T> const& structure) {
      return glz::write_json(to_pymatgen(structure)).value_or("");  // handle error properly
    }
  };

  template <> struct structure_adapter_<float, structure_format::json_pymatgen>;
  template <> struct structure_adapter_<double, structure_format::json_pymatgen>;
}  // namespace sqsgen::io

namespace glz {
  template <class T> struct glz::meta<sqsgen::io::detail::pymatgen_structure<T>> {
    using S = sqsgen::io::detail::pymatgen_structure<T>;
    static constexpr auto value
        = glz::object("properties", &S::properties, "lattice", &S::lattice, "@module", &S::module,
                      "@class", &S::class_, "charge", &S::charge, "sites", &S::sites);
  };
}  // namespace glz
