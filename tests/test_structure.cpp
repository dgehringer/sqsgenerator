//
// Created by Dominik Gehringer on 23.10.24.
//

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "helpers.h"
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
  };

  template <class T> void to_json(json& j, SupercellTestData<T> const& st) {
    j = json{{"structure", st.structure}, {"supercell", st.supercell}, {"shape", st.shape}};
  }

  template <class T> void from_json(const json& j, SupercellTestData<T>& st) {
    j.at("structure").get_to(st.structure);
    j.at("supercell").get_to(st.supercell);
    j.at("shape").get_to(st.shape);
  }

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

  template <class T, template <class> class Matrix = matrix_t>
  Matrix<T> matrix_from_json(json const& j, std::string const& name) {
    return core::helpers::stl_to_eigen<Matrix<T>>(j.at(name).get<stl_matrix_t<T>>());
  }

  template <class T> void from_json(const json& j, StructureTestData<T>& st) {
    auto species = j.at("species").get<std::vector<std::string>>();

    auto ordinals = ranges::to<std::vector>(
        species | views::transform([](auto const& s) { return core::Atom::from_symbol(s).Z; }));

    st = StructureTestData<T>(std::move(matrix_from_json<T, lattice_t>(j, "lattice")),
                              std::move(matrix_from_json<T, coords_t>(j, "frac_coords")),
                              std::move(ordinals),
                              std::move(matrix_from_json<T>(j, "distance_matrix")),
                              std::move(matrix_from_json<std::size_t>(j, "shell_matrix")));
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
      auto m = structure.shell_matrix();

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
        auto equals_site = [&](auto other) { return site_equals(site, other); };
        auto expected_sites = test_case.supercell.sites();
        auto same_site = std::find_if(expected_sites.begin(), expected_sites.end(), equals_site);
        if (same_site == expected_sites.end()) ASSERT_TRUE(false) << std::format("Site same as supercell not found {}, {}", site.atom().name, site.frac_coords) << std::endl;
      }
    }
  }

}  // namespace sqsgen::testing