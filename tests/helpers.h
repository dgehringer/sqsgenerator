//
// Shared test utilities for the Catch2 suite.
//
// Provides a handful of matrix/vector comparison helpers and the minimal set of
// nlohmann (de)serializers for the Eigen-backed domain types used by the
// structure tests. These are kept local to the tests (rather than pulling in
// <sqsgen/io/json.h> or <sqsgen/core/helpers/eigen.h>) because those headers
// still drag in the legacy, mid-migration result/config surface.
//

#ifndef SQSGEN_TESTS_HELPERS_H
#define SQSGEN_TESTS_HELPERS_H

#include <Eigen/Core>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstddef>
#include <nlohmann/json.hpp>
#include <utility>
#include <vector>

#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::testing::detail {

  template <class Matrix> nlohmann::json eigen_to_json(const Matrix& m) {
    auto rows = nlohmann::json::array();
    for (Eigen::Index r = 0; r < m.rows(); ++r) {
      auto row = nlohmann::json::array();
      for (Eigen::Index c = 0; c < m.cols(); ++c) row.push_back(m(r, c));
      rows.push_back(std::move(row));
    }
    return rows;
  }

  template <class Matrix> Matrix json_to_eigen(const nlohmann::json& j) {
    const Eigen::Index rows = static_cast<Eigen::Index>(j.size());
    const Eigen::Index cols = rows > 0 ? static_cast<Eigen::Index>(j.at(0).size()) : 0;
    Matrix m(rows, cols);
    for (Eigen::Index r = 0; r < rows; ++r)
      for (Eigen::Index c = 0; c < cols; ++c)
        m(r, c) = j.at(static_cast<std::size_t>(r))
                      .at(static_cast<std::size_t>(c))
                      .template get<typename Matrix::Scalar>();
    return m;
  }

}  // namespace sqsgen::testing::detail

NLOHMANN_JSON_NAMESPACE_BEGIN

using namespace sqsgen;

template <class T> struct adl_serializer<matrix_t<T>> {
  static void to_json(json& j, const matrix_t<T>& m) { j = testing::detail::eigen_to_json(m); }
  static void from_json(const json& j, matrix_t<T>& m) {
    m = testing::detail::json_to_eigen<matrix_t<T>>(j);
  }
};

template <class T> struct adl_serializer<lattice_t<T>> {
  static void to_json(json& j, const lattice_t<T>& m) { j = testing::detail::eigen_to_json(m); }
  static void from_json(const json& j, lattice_t<T>& m) {
    m = testing::detail::json_to_eigen<lattice_t<T>>(j);
  }
};

template <class T> struct adl_serializer<coords_t<T>> {
  static void to_json(json& j, const coords_t<T>& m) { j = testing::detail::eigen_to_json(m); }
  static void from_json(const json& j, coords_t<T>& m) {
    m = testing::detail::json_to_eigen<coords_t<T>>(j);
  }
};

template <class T> struct adl_serializer<core::structure<T>> {
  static void to_json(json& j, core::structure<T> const& s) {
    j = json{
        {"lattice", s.lattice},
        {"species", s.species},
        {"frac_coords", s.frac_coords},
        {"pbc", s.pbc},
    };
  }
  static void from_json(const json& j, core::structure<T>& s) {
    j.at("species").get_to(s.species);
    j.at("lattice").get_to(s.lattice);
    j.at("frac_coords").get_to(s.frac_coords);
    j.at("pbc").get_to(s.pbc);
    s.num_species = core::detail::compute_num_species(s.species);
  }
};

NLOHMANN_JSON_NAMESPACE_END

namespace sqsgen::testing::helpers {

  template <class Matrix> void assert_matrix_equal(const Matrix& lhs, const Matrix& rhs,
                                                   typename Matrix::Scalar epsilon = 1.0e-7) {
    REQUIRE(lhs.rows() == rhs.rows());
    REQUIRE(lhs.cols() == rhs.cols());
    for (Eigen::Index row = 0; row < lhs.rows(); ++row)
      for (Eigen::Index col = 0; col < lhs.cols(); ++col)
        CHECK_THAT(static_cast<double>(lhs(row, col)),
                   Catch::Matchers::WithinAbs(static_cast<double>(rhs(row, col)), epsilon));
  }

  template <class T>
  void assert_vector_equal(const std::vector<T>& lhs, const std::vector<T>& rhs) {
    REQUIRE(lhs.size() == rhs.size());
    for (std::size_t i = 0; i < lhs.size(); ++i) CHECK(lhs[i] == rhs[i]);
  }

  template <class T> void assert_vector_is_close(const std::vector<T>& lhs,
                                                 const std::vector<T>& rhs, T atol = 1.0e-5,
                                                 T rtol = 1.0e-3) {
    REQUIRE(lhs.size() == rhs.size());
    for (std::size_t i = 0; i < lhs.size(); ++i)
      CHECK(core::helpers::is_close(lhs[i], rhs[i], atol, rtol));
  }

}  // namespace sqsgen::testing::helpers

#endif  // SQSGEN_TESTS_HELPERS_H
