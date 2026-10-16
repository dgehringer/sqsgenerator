

#include <string>

#include "sqsgen/core/permutation.h"
#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::io {

  namespace core = sqsgen::core;
  namespace helpers = sqsgen::core::helpers;

  template <class T> std::string structure_adapter<T, io::structure_format::cif>::format(
      const core::structure<T>& structure) {
    auto filtered = structure.without_vacancies();
    auto sorted = filtered.sorted([](auto&& a, auto&& b) { return a.specie < b.specie; });
    auto unique_species = core::sorted_vector<specie_t>{sorted.species};
    auto num_species = core::count_species(sorted.species);
    const auto z_to_symbol = [](auto&& z) { return core::atom::from_z(z).symbol; };
    std::string result;
    result.reserve(filtered.size() * 144);

    const auto println = [&result](std::string const& line) {
      result.append(helpers::format_string("%s\n", line));
    };

    const auto make_formula = [&](std::string const& delimiter) {
      return detail::join(unique_species | std::ranges::views::transform([&](auto&& s) {
                            return helpers::format_string("%s%i", z_to_symbol(s), num_species[s]);
                          }),
                          delimiter);
    };

    println("# generated using sqsgen");
    println(format_string("data_%s", make_formula("")));
    println("_symmetry_space_group_name_H-M   'P 1'");

    const auto [a, b, c] = detail::lengths<T>(filtered.lattice);
    const auto [alpha, beta, gamma] = detail::angles<T>(filtered.lattice);

    println(format_string("_cell_length_a       %.8f", a));
    println(format_string("_cell_length_b       %.8f", b));
    println(format_string("_cell_length_c       %.8f", c));
    println(format_string("_cell_angle_alpha     %.8f", alpha));
    println(format_string("_cell_angle_beta     %.8f", beta));
    println(format_string("_cell_angle_gamma     %.8f", gamma));

    println("_symmetry_Int_Tables_number   1");
    println(format_string("_chemical_formula_structural   %s", make_formula("")));
    println(format_string("_chemical_formula_sum   '%s'", make_formula(" ")));
    println(format_string("_cell_volume   %.5f", std::abs(filtered.lattice.determinant())));
    println(format_string("_cell_formula_units_Z   %u", filtered.size()));

    println("loop_");
    println(" _symmetry_equiv_pos_site_id");
    println(" _symmetry_equiv_pos_as_xyz");
    println("  1  'x, y, z'");

    println("loop_");
    println(" _atom_type_symbol");
    println(" _atom_type_oxidation_number");
    for (auto z : unique_species) println(format_string("  %s0+  0.0", z_to_symbol(z)));

    println("loop_");
    println(" _atom_site_type_symbol");
    println(" _atom_site_label");
    println(" _atom_site_symmetry_multiplicity");
    println(" _atom_site_fract_x");
    println(" _atom_site_fract_y");
    println(" _atom_site_fract_z");
    println(" _atom_site_occupancy");

    for (auto const& site : sorted.sites())
      println(format_string("  %s0+  %s%i  1  %.8f  %.8f  %.8f  1", z_to_symbol(site.specie),
                            z_to_symbol(site.specie), num_species[site.specie]--,
                            site.frac_coords(0), site.frac_coords(1), site.frac_coords(2)));
    println("");
    result.shrink_to_fit();

    return result;
  }

  template <> struct structure_adapter<float, structure_format::cif>;
  template <> struct structure_adapter<double, structure_format::cif>;
}  // namespace sqsgen::io
