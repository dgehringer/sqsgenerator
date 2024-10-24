//
// Created by Dominik Gehringer on 24.10.24.
//

#ifndef SQSGEN_TESTS_HELPERS_H
#define SQSGEN_TESTS_HELPERS_H

#include <gtest/gtest.h>

namespace sqsgen::testing::helpers {
  template <class Matrix> void assert_matrix_equal(const Matrix& lhs, const Matrix& rhs,
                                                   typename Matrix::Scalar epsilon = 1.0e-7) {
    ASSERT_EQ(lhs.rows(), rhs.rows()) << "Rows of lhs and rhs are not equal";
    ASSERT_EQ(lhs.cols(), rhs.cols()) << "Columns of lhs and rhs are not equal";
    for (auto row = 0; row < lhs.rows(); ++row) {
      for (auto col = 0; col < lhs.cols(); ++col) {
        ASSERT_NEAR(lhs(row, col), rhs(row, col), epsilon)
            << std::format("Error at position ({},{})", row, col);
      }
    }
  }
};  // namespace sqsgen::testing::helpers

#endif  // SQSGEN_TESTS_HELPERS_H
