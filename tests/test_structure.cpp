//
// Created by Dominik Gehringer on 23.10.24.
//

#include <gtest/gtest.h>
#include <Eigen/Dense>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "helpers.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/json.h"

namespace sqsgen::testing {
  using namespace sqsgen;
  using json = nlohmann::json;
  namespace ranges = std::ranges;
  namespace views = ranges::views;

  template <class T> using stl_matrix = std::vector<std::vector<T>>;

  template <class T> struct SupercellTestData {
    core::structure<T> structure;
    core::structure<T> supercell;
    std::array<std::size_t, 3> shape;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SupercellTestData, structure, supercell, shape);
  };

  template <class T> class StructureTestData : public core::structure<T> {
  public:
    matrix_t<T> distance_matrix;
    matrix_t<std::size_t> shell_matrix;

    StructureTestData() = default;

    StructureTestData(lattice_t<T>&& lattice, coords_t<T>&& frac_coords,
                      std::vector<specie_t>&& species, matrix_t<T>&& distance_matrix,
                      matrix_t<std::size_t>&& shell_matrix)
        : distance_matrix(distance_matrix),
          shell_matrix(shell_matrix),
          core::structure<T>(lattice, frac_coords, species, {true, true, true}) {}

    [[nodiscard]] core::structure<T> structure() const {
      return core::structure<T>(this->lattice, this->frac_coords, this->species);
    }
  };

  template <class T> void to_json(json& j, StructureTestData<T> const& st) {
    j = json{
        {"lattice", st.lattice},
        {"species", st.species},
        {"frac_coords", st.frac_coords},
        {"pbc", st.pbc},
        {"distance_matrix", st.distance_matrix},
        {"shell_matrix", st.shell_matrix},
    };
  }

  template <class T> void from_json(const json& j, StructureTestData<T>& st) {
    auto species = j.at("species").get<std::vector<std::string>>();

    auto ordinals = core::helpers::as<std::vector>{}(
        species | views::transform([](auto const& s) { return core::atom::from_symbol(s).Z; }));

    st = StructureTestData<T>(j.at("lattice").get<lattice_t<T>>(),
                              j.at("frac_coords").get<coords_t<T>>(), std::move(ordinals),
                              j.at("distance_matrix").get<matrix_t<T>>(),
                              j.at("shell_matrix").get<matrix_t<std::size_t>>());
  }

  class StructureTestFixture : public ::testing::Test {
  public:
    std::vector<StructureTestData<double>> test_cases;

    StructureTestFixture() {
      const auto cwd = std::filesystem::current_path() / "assets";
      std::vector<StructureTestData<double>> test_cases;
      for (const auto& entry : std::filesystem::directory_iterator{cwd}) {
        if (entry.path().string().ends_with(".structure.json")) {
          StructureTestData<double> test_case{};
          std::ifstream file{entry.path().string()};
          json::parse(file).get_to(test_case);
          test_cases.push_back(test_case);
        }
      }
      this->test_cases = std::move(test_cases);
    }
  };

  class SupercellTestFixture : public ::testing::Test {
  public:
    std::vector<SupercellTestData<double>> test_cases;

    SupercellTestFixture() {
      const auto cwd = std::filesystem::current_path() / "assets";
      std::vector<SupercellTestData<double>> test_cases;
      for (const auto& entry : std::filesystem::directory_iterator{cwd}) {
        if (entry.path().string().ends_with(".supercell.json")) {
          SupercellTestData<double> test_case{};
          std::ifstream file{entry.path().string()};
          json::parse(file).get_to(test_case);
          test_cases.push_back(test_case);
        }
      }
      this->test_cases = std::move(test_cases);
    }
  };

  TEST_F(StructureTestFixture, distance_matrix) {
    for (const auto& test_case : this->test_cases) {
      auto structure = test_case.structure();
      helpers::assert_matrix_equal(test_case.distance_matrix, structure.distance_matrix());
    }
  }

  TEST_F(StructureTestFixture, shell_matrix) {
    for (const auto& test_case : this->test_cases) {
      auto structure = test_case.structure();
      auto m = structure.shell_matrix(
          distances_naive(std::forward<core::structure<double>>(structure)));

      ASSERT_EQ(test_case.shell_matrix.rows(), m.rows());
      ASSERT_EQ(test_case.shell_matrix.cols(), m.cols());
      core::helpers::for_each(
          [&](auto i, auto j) {
            if (i == j) ASSERT_EQ(m(i, j), 0);
            ASSERT_EQ(test_case.shell_matrix(i, j), m(i, j))
                << std::format("Shell mismatch at ({}, {})", i, j);
          },
          test_case.shell_matrix.rows(), test_case.shell_matrix.cols());
    }
  }

  template <class T> bool site_equals(core::site_t<T> const& lhs, core::site_t<T> const& rhs) {
    auto lcoords = lhs.frac_coords;
    auto rcoords = rhs.frac_coords;
    return lhs.atom().Z == rhs.atom().Z && core::helpers::is_close(lcoords(0), rcoords(0))
           && core::helpers::is_close(lcoords(1), rcoords(1))
           && core::helpers::is_close(lcoords(2), rcoords(2));
  }

  TEST_F(SupercellTestFixture, supercell) {
    for (const auto& test_case : this->test_cases) {
      auto shape = test_case.shape;
      auto supercell_computed = test_case.structure.supercell(
          std::get<0>(shape), std::get<1>(shape), std::get<2>(shape));

      for (const auto& site : supercell_computed.sites()) {
        auto equals_site = [&](const auto& other) { return site_equals(site, other); };
        auto expected_sites = test_case.supercell.sites();
        auto same_site = std::find_if(expected_sites.begin(), expected_sites.end(), equals_site);
        if (same_site == expected_sites.end()) ASSERT_TRUE(false);
      }
    }
  }

  template <class T> const static auto TEST_FCC_STRUCTURE = core::structure<T>{
      lattice_t<T>{{1, 0, 0}, {0, 2, 0}, {0, 0, 3}},
      coords_t<T>{{0.0, 0.0, 0.0}, {0.0, 0.5, 0.5}, {0.5, 0.0, 0.5}, {0.5, 0.5, 0.0}},
      std::vector<specie_t>{11, 12, 13, 14}};

  TEST(Structure, sorted) {
    auto [structure, indices] = TEST_FCC_STRUCTURE<double>.sorted_with_indices(
        [](auto a, auto b) { return a.specie > b.specie; });
    // structure is sorted in reversed order
    helpers::assert_vector_equal(structure.species, {14, 13, 12, 11});
    // indices are a contiguous reversed sequence
    helpers::assert_vector_equal(indices, {3, 2, 1, 0});
    // sorting the structure must not change the underlying sites
    using hasher_t = core::site_t<double>::hasher;
    auto computed_sites
        = core::helpers::as<std::unordered_set>{}.operator()<hasher_t>(structure.sites());
    auto expected_sites = core::helpers::as<std::unordered_set>{}.operator()<hasher_t>(
        TEST_FCC_STRUCTURE<double>.sites());
    ASSERT_EQ(computed_sites, expected_sites);
  }

  TEST(Structure, filtered) {
    auto structure
        = TEST_FCC_STRUCTURE<double>.filtered([](auto site) { return site.specie > 12; });
    ASSERT_EQ(structure.size(), 2);
    helpers::assert_vector_equal(structure.species, {13, 14});
    // filtering out all atoms must result in an exception
    ASSERT_THROW(TEST_FCC_STRUCTURE<double>.filtered([](auto) { return false; }),
                 std::invalid_argument);
  }

  TEST(Structure, sliced) {
    auto structure = TEST_FCC_STRUCTURE<double>.sliced(views::iota(0, 4));
    ASSERT_EQ(structure.size(), 4);
    // filtering out all atoms must result in an exception
    ASSERT_THROW(TEST_FCC_STRUCTURE<double>.sliced(views::iota(-1, 4)), std::out_of_range);
    ASSERT_THROW(TEST_FCC_STRUCTURE<double>.sliced(views::iota(3UL, structure.size() + 2UL)),
                 std::out_of_range);
    ASSERT_THROW(TEST_FCC_STRUCTURE<double>.sliced(std::vector<std::size_t>{}),
                 std::invalid_argument);

    auto repeated = TEST_FCC_STRUCTURE<double>.sliced(std::vector<std::size_t>{1, 1, 1});
    helpers::assert_vector_equal(repeated.species, {12, 12, 12});

    auto sites = core::helpers::as<std::set>{}(repeated.sites());
    ASSERT_EQ(sites.size(), 1);
  }

  TEST(Structure, prefactors_single) {
    // recompute coordination numbers using prefactor mode
    auto fcc_one_species = core::structure<double>{lattice_t<double>::Identity(),
                                                   coords_t<double>{{0.0, 0.0, 0.0},
                                                                    {0.0, 0.5, 0.5},
                                                                    {0.5, 0.0, 0.5},
                                                                    {0.5, 0.5, 0.0}},
                                                   configuration_t{1, 1, 1, 1}}
                               .supercell(3, 3, 3);

    auto dists = distances_naive(std::forward<decltype(fcc_one_species)>(fcc_one_species));
    helpers::assert_vector_is_close(
        dists, {0, 1.0 / std::sqrt(2), 1.0, std::sqrt(1.5), std::sqrt(2),std::sqrt(2.5), std::sqrt(3), std::sqrt(3.5), std::sqrt(4.5), std::sqrt(5.5)});

    shell_weights_t<double> w {{1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}};

    const auto coordination_number = [&](auto && prefactor) {
      // x_a
      return static_cast<usize_t>((1.0 / prefactor) / static_cast<double>(fcc_one_species.size()));
    };

    auto f = core::detail::compute_prefactors(fcc_one_species.shell_matrix(dists), w, fcc_one_species.species);
    std::vector prefactors(f.data(), f.data() + f.size());
    // compute the coordination number of a fcc lattice by inverting the prefactors
    helpers::assert_vector_equal(core::helpers::as<std::vector>{}(prefactors | views::transform(coordination_number)), {12, 6, 24, 12, 12, 8});
  }

}  // namespace sqsgen::testing