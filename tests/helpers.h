//
// Shared test utilities for the Catch2 suite.
//
// A handful of matrix/vector comparison helpers built on Catch2's floating-point
// matchers. Approximate comparisons use Catch::Approx (margin = absolute
// tolerance, epsilon = relative tolerance), which replaces the former
// core::helpers::is_close checks.
//

#ifndef SQSGEN_TESTS_HELPERS_H
#define SQSGEN_TESTS_HELPERS_H

#include <Eigen/Core>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstddef>
#include <vector>

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
      CHECK(static_cast<double>(lhs[i])
            == Catch::Approx(static_cast<double>(rhs[i])).margin(atol).epsilon(rtol));
  }

}  // namespace sqsgen::testing::helpers

#endif  // SQSGEN_TESTS_HELPERS_H
