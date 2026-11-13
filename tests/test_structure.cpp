
#include <Eigen/Dense>
#include <algorithm>
#include <array>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <glaze/glaze.hpp>
#include <map>
#include <ranges>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "helpers.h"
#include "sqsgen/configuration.h"
#include "sqsgen/core/atom.h"
#include "sqsgen/core/eigen.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/core/structure_utils.h"
#include "sqsgen/optimization/prefactors.h"
#include "sqsgen/types.h"

namespace sqsgen::testing {
  using namespace sqsgen;
  namespace ranges = std::ranges;
  namespace views = ranges::views;

  struct structure_case {
    core::nested_t<double, 2> lattice;
    core::nested_t<double, 2> frac_coords;
    core::nested_t<double, 2> distance_matrix;
    core::nested_t<std::size_t, 2> shell_matrix;
    std::vector<std::string> species;
  };

  struct structure_data {
    core::nested_t<double, 2> lattice;
    core::nested_t<double, 2> frac_coords;
    std::vector<int> species;
    std::array<bool, 3> pbc;
  };

  struct supercell_case {
    structure_data structure;
    structure_data supercell;
    std::array<std::size_t, 3> shape;
  };

  template <class T> T read_asset(const std::string& filename) {
    std::ifstream file{std::filesystem::current_path() / "assets" / filename};
    REQUIRE(file.is_open());
    std::stringstream ss;
    ss << file.rdbuf();
    T value{};
    REQUIRE(!glz::read_json(value, ss.str()));
    return value;
  }

  core::structure<double> make_structure(const core::nested_t<double, 2>& lattice,
                                         const core::nested_t<double, 2>& frac_coords,
                                         const std::vector<std::string>& symbols) {
    configuration_t species;
    species.reserve(symbols.size());
    for (const auto& symbol : symbols) species.push_back(core::atom::from_symbol(symbol).value().Z);
    return core::structure<double>(core::lattice_from<double>(lattice).value(),
                                   core::coords_from<double>(frac_coords).value(),
                                   std::move(species));
  }

  core::structure<double> make_structure(const core::nested_t<double, 2>& lattice,
                                         const core::nested_t<double, 2>& frac_coords,
                                         const std::vector<int>& ordinals) {
    configuration_t species(ordinals.begin(), ordinals.end());
    return core::structure<double>(core::lattice_from<double>(lattice).value(),
                                   core::coords_from<double>(frac_coords).value(),
                                   std::move(species));
  }

  template <class T> const auto TEST_FCC_STRUCTURE = core::structure<T>{
      lattice_t<T>{{1, 0, 0}, {0, 2, 0}, {0, 0, 3}},
      coords_t<T>{{0.0, 0.0, 0.0}, {0.0, 0.5, 0.5}, {0.5, 0.0, 0.5}, {0.5, 0.5, 0.0}},
      std::vector<specie_t>{11, 12, 13, 14}};

  template <class T> bool site_equals(core::site_t<T> const& lhs, core::site_t<T> const& rhs) {
    auto lcoords = lhs.frac_coords;
    auto rcoords = rhs.frac_coords;
    return lhs.atom().Z == rhs.atom().Z
           && lcoords(0) == Catch::Approx(rcoords(0)).margin(1.0e-5).epsilon(1.0e-3)
           && lcoords(1) == Catch::Approx(rcoords(1)).margin(1.0e-5).epsilon(1.0e-3)
           && lcoords(2) == Catch::Approx(rcoords(2)).margin(1.0e-5).epsilon(1.0e-3);
  }

  TEST_CASE("distance_matrix matches the reference", "[structure]") {
    const auto test_case = read_asset<structure_case>("case1.structure.json");
    auto structure = make_structure(test_case.lattice, test_case.frac_coords, test_case.species);
    helpers::assert_matrix_equal(core::matrix_from<double>(test_case.distance_matrix).value(),
                                 structure.distance_matrix());
  }

  TEST_CASE("shell_matrix matches the reference", "[structure]") {
    const auto test_case = read_asset<structure_case>("case1.structure.json");
    auto structure = make_structure(test_case.lattice, test_case.frac_coords, test_case.species);
    auto expected = core::matrix_from<std::size_t>(test_case.shell_matrix).value();
    auto computed = structure.shell_matrix(core::distances_naive(structure));

    REQUIRE(expected.rows() == computed.rows());
    REQUIRE(expected.cols() == computed.cols());
    for (Eigen::Index i = 0; i < expected.rows(); ++i)
      for (Eigen::Index j = 0; j < expected.cols(); ++j) {
        if (i == j) CHECK(computed(i, j) == 0);
        INFO(std::format("Shell mismatch at ({}, {})", i, j));
        CHECK(expected(i, j) == computed(i, j));
      }
  }

  TEST_CASE("supercell reproduces the reference sites", "[structure][supercell]") {
    const auto test_case = read_asset<supercell_case>("case1.supercell.json");
    auto source = make_structure(test_case.structure.lattice, test_case.structure.frac_coords,
                                 test_case.structure.species);
    auto reference = make_structure(test_case.supercell.lattice, test_case.supercell.frac_coords,
                                    test_case.supercell.species);
    auto [a, b, c] = test_case.shape;
    auto computed = source.supercell(a, b, c);

    auto expected_sites = reference.sites();
    for (const auto& site : computed.sites()) {
      auto equals_site = [&](const auto& other) { return site_equals(site, other); };
      CHECK(std::find_if(expected_sites.begin(), expected_sites.end(), equals_site)
            != expected_sites.end());
    }
  }

  TEST_CASE("sorted_with_indices reorders sites without losing them", "[structure]") {
    auto [structure, indices] = TEST_FCC_STRUCTURE<double>.sorted_with_indices(
        [](auto a, auto b) { return a.specie > b.specie; });
    helpers::assert_vector_equal(structure.species, {14, 13, 12, 11});
    helpers::assert_vector_equal(indices, {3, 2, 1, 0});

    using set_t = std::unordered_set<core::site_t<double>, core::site_t<double>::hasher>;
    auto computed_sites = structure.sites() | ranges::to<set_t>();
    auto expected_sites = TEST_FCC_STRUCTURE<double>.sites() | ranges::to<set_t>();
    CHECK(computed_sites == expected_sites);
  }

  TEST_CASE("filtered keeps matching sites and rejects an empty result", "[structure]") {
    auto structure
        = TEST_FCC_STRUCTURE<double>.filtered([](auto site) { return site.specie > 12; });
    REQUIRE(structure.size() == 2);
    helpers::assert_vector_equal(structure.species, {13, 14});
    CHECK_THROWS_AS(TEST_FCC_STRUCTURE<double>.filtered([](auto) { return false; }),
                    std::invalid_argument);
  }

  TEST_CASE("sliced selects/repeats sites and range-checks indices", "[structure]") {
    auto structure = TEST_FCC_STRUCTURE<double>.sliced(views::iota(0, 4));
    REQUIRE(structure.size() == 4);
    CHECK_THROWS_AS(TEST_FCC_STRUCTURE<double>.sliced(views::iota(-1, 4)), std::out_of_range);
    CHECK_THROWS_AS(TEST_FCC_STRUCTURE<double>.sliced(views::iota(3UL, structure.size() + 2UL)),
                    std::out_of_range);
    CHECK_THROWS_AS(TEST_FCC_STRUCTURE<double>.sliced(std::vector<std::size_t>{}),
                    std::invalid_argument);

    auto repeated = TEST_FCC_STRUCTURE<double>.sliced(std::vector<std::size_t>{1, 1, 1});
    helpers::assert_vector_equal(repeated.species, {12, 12, 12});

    auto sites = repeated.sites() | ranges::to<std::set<core::site_t<double>>>();
    CHECK(sites.size() == 1);
  }

  TEST_CASE("prefactors invert to fcc coordination numbers", "[structure][prefactors]") {
    configuration_t conf{1, 1, 1, 1};
    auto fcc_one_species = core::structure<double>{lattice_t<double>::Identity(),
                                                   coords_t<double>{{0.0, 0.0, 0.0},
                                                                    {0.0, 0.5, 0.5},
                                                                    {0.5, 0.0, 0.5},
                                                                    {0.5, 0.5, 0.0}},
                                                   conf}
                               .supercell(3, 3, 3);

    auto dists = core::distances_naive(fcc_one_species);
    helpers::assert_vector_is_close(
        dists, {0, 1.0 / std::sqrt(2), 1.0, std::sqrt(1.5), std::sqrt(2), std::sqrt(2.5),
                std::sqrt(3), std::sqrt(3.5), std::sqrt(4.5), std::sqrt(5.5)});

    std::map<shell_t, double> w{{1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}};

    const auto coordination_number = [&](auto&& prefactor) {
      return static_cast<std::size_t>((1.0 / prefactor)
                                      / static_cast<double>(fcc_one_species.size()))
             * 2;
    };
    const auto shell_radii = core::distances_histogram(
        fcc_one_species, configuration::defaults::bin_width_default<double>,
        configuration::defaults::peak_isolation_default<double>);
    auto f = optimization::compute_pair_prefactors<double>(
        fcc_one_species.shell_matrix(shell_radii), w, conf);
    std::vector prefactors(f.data(), f.data() + f.size());
    helpers::assert_vector_equal(
        prefactors | views::transform(coordination_number) | ranges::to<std::vector<std::size_t>>(),
        {12, 6, 24, 12, 12, 8});
  }

}  // namespace sqsgen::testing
