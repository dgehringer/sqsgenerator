//
// Created by Dominik Gehringer on 24.10.24.
//

#ifndef SQSGEN_TESTS_HELPERS_H
#define SQSGEN_TESTS_HELPERS_H

#include <gtest/gtest.h>

#include "core/structure.h"

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

  template <class T>
  void assert_structure_equal(core::structure<T> const& lhs, core::structure<T> const& rhs, T epsilon = 1.0e-7) {
    ASSERT_EQ(lhs.size(), rhs.size()) << "Size of lhs and rhs are not equal";
    auto lhs_sites = core::helpers::as<std::vector>{}(lhs.sites());
    auto rhs_sites = core::helpers::as<std::vector>{}(rhs.sites());
    for (int i = 0; i < lhs.size(); ++i) {
      auto lsite = lhs_sites[i];
      auto rsite = rhs_sites[i];
      ASSERT_EQ(lsite.specie, rsite.specie);
      ASSERT_NEAR(lsite.frac_coords(0), rsite.frac_coords(0), epsilon);
      ASSERT_NEAR(lsite.frac_coords(1), rsite.frac_coords(1), epsilon);
      ASSERT_NEAR(lsite.frac_coords(2), rsite.frac_coords(2), epsilon);
    }
  }

  template <class T>
  void assert_vector_equal(const std::vector<T>& lhs, const std::vector<T>& rhs) {
    ASSERT_EQ(lhs.size(), rhs.size());
    for (auto i = 0; i < lhs.size(); ++i)
      ASSERT_EQ(lhs[i], rhs[i]) << std::format("Error at position {} ({} != {})", i, lhs[i],
                                               rhs[i]);
  }

};  // namespace sqsgen::testing::helpers

#endif  // SQSGEN_TESTS_HELPERS_H
