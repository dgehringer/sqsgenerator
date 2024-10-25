//
// Created by Dominik Gehringer on 23.10.24.
//

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "helpers.h"
#include "sqsgen/core/structure.h"

namespace sqsgen::testing {
  using json = nlohmann::json;
  namespace ranges = std::ranges;
  namespace views = ranges::views;

  template <class T> using stl_matrix = std::vector<std::vector<T>>;

  template <class T> class StructureTestData : public core::Structure<T> {
  public:
    matrix_t<T> distance_matrix;
    matrix_t<std::size_t> shell_matrix;

    StructureTestData() = default;

    StructureTestData(lattice_t<T>&& lattice, coords_t<T>&& frac_coords,
                      std::vector<specie_t>&& species, matrix_t<T>&& distance_matrix,
                      matrix_t<std::size_t>&& shell_matrix)
        : distance_matrix(distance_matrix),
          shell_matrix(shell_matrix),
          core::Structure<T>(lattice, frac_coords, species, {true, true, true}) {}

    core::Structure<T> structure() const {
      return core::Structure<T>(this->lattice, this->frac_coords, this->species);
    }
  };

  template <class T> void to_json(json& j, StructureTestData<T> const& st) {
    j = json{
        {"lattice", core::helpers::eigen_to_stl(st.lattice)},
        {"species", st.species},
        {"frac_coords", core::helpers::eigen_to_stl(st.frac_coords)},
        {"pbc", st.pbc},
        {"distance_matrix", core::helpers::eigen_to_stl(st.distance_matrix)},
        {"shell_matrix", core::helpers::eigen_to_stl(st.shell_matrix)},
    };
  }

  template <class T> void from_json(const json& j, StructureTestData<T>& st) {
    auto lattice = j.at("lattice").get<stl_matrix<T>>();
    auto frac_coords = j.at("frac_coords").get<stl_matrix<T>>();
    auto distance_matrix = j.at("distance_matrix").get<stl_matrix<T>>();
    auto shell_matrix = j.at("shell_matrix").get<stl_matrix<std::size_t>>();
    auto species = j.at("species").get<std::vector<std::string>>();

    auto ordinals = ranges::to<std::vector>(
        species | views::transform([](auto const& s) { return core::Atom::from_symbol(s).Z; }));

    st = StructureTestData<T>(
        std::move(core::helpers::stl_to_eigen<lattice_t<T>>(lattice)),
        std::move(core::helpers::stl_to_eigen<coords_t<T>>(frac_coords)), std::move(ordinals),
        std::move(core::helpers::stl_to_eigen<matrix_t<T>>(distance_matrix)),
        std::move(core::helpers::stl_to_eigen<matrix_t<std::size_t>>(shell_matrix)));
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

  TEST_F(StructureTestFixture, distance_matrix) {
    for (const auto& test_case : this->test_cases) {
      auto structure = test_case.structure();
      helpers::assert_matrix_equal(test_case.distance_matrix, structure.distance_matrix());
    }
  }

  TEST_F(StructureTestFixture, shell_matrix) {


    for (const auto& test_case : this->test_cases) {
      auto structure = test_case.structure();
      auto shell_matrix = structure.shell_matrix();
      auto compare_matrices = [&]<class T>(matrix_t<T> const&m) {
          ASSERT_EQ(test_case.shell_matrix.rows(), m.rows());
          ASSERT_EQ(test_case.shell_matrix.cols(), m.cols());
          std::cout << m << std::endl;
          core::helpers::for_each([&](auto i, auto j) {
              if(i == j) ASSERT_EQ(m(i, j), 0);
              ASSERT_EQ(test_case.shell_matrix(i, j), m(i, j)) << std::format("Shell mismatch at ({}, {})", i, j);
          }, test_case.shell_matrix.rows(), test_case.shell_matrix.cols());
      };
      std::visit(compare_matrices, shell_matrix);
    }
  }
}  // namespace sqsgen::testing