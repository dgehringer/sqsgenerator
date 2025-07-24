//
// Created by Dominik Gehringer on 24.10.24.
//

#ifndef SQSGEN_TESTS_HELPERS_H
#define SQSGEN_TESTS_HELPERS_H

#include <gtest/gtest.h>

#include "sqsgen/core/helpers.h"
#include "sqsgen/log.h"

namespace sqsgen::testing::helpers {
  template <class Matrix> void assert_matrix_equal(const Matrix& lhs, const Matrix& rhs,
                                                   typename Matrix::Scalar epsilon = 1.0e-7) {
    ASSERT_EQ(lhs.rows(), rhs.rows()) << "Rows of lhs and rhs are not equal";
    ASSERT_EQ(lhs.cols(), rhs.cols()) << "Columns of lhs and rhs are not equal";
    for (auto row = 0; row < lhs.rows(); ++row) {
      for (auto col = 0; col < lhs.cols(); ++col) {
        ASSERT_NEAR(lhs(row, col), rhs(row, col), epsilon)
            << format_string("Error at position (%i, %i)", row, col);
      }
    }
  }

  template <class T>
  void assert_vector_equal(const std::vector<T>& lhs, const std::vector<T>& rhs) {
    ASSERT_EQ(lhs.size(), rhs.size());
    for (auto i = 0; i < lhs.size(); ++i)
      ASSERT_EQ(lhs[i], rhs[i]) << format_string("Error at position %i (%.7f != %.7f)", i, lhs[i],
                                                 rhs[i]);
  }

  template <class T>
  void assert_vector_equal(stl_matrix_t<T> const& lhs, const stl_matrix_t<T>& rhs) {
    ASSERT_EQ(lhs.size(), rhs.size());
    for (auto i = 0; i < lhs.size(); ++i) assert_vector_equal(lhs[i], rhs[i]);
  }

  template <class T> void assert_vector_is_close(const std::vector<T>& lhs,
                                                 const std::vector<T>& rhs, T atol = 1.0e-5,
                                                 T rtol = 1.0e-3) {
    ASSERT_EQ(lhs.size(), rhs.size());
    for (auto i = 0; i < lhs.size(); ++i)
      ASSERT_TRUE(core::helpers::is_close(lhs[i], rhs[i], atol, rtol))
          << format_string("Error at position %i (%.7f != %.7f)", i, lhs[i], rhs[i]);
  }

};  // namespace sqsgen::testing::helpers

#endif  // SQSGEN_TESTS_HELPERS_H
