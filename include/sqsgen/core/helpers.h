//
// Created by Dominik Gehringer on 21.09.24.
//

#ifndef SQSGEN_CORE_HELPERS_H
#define SQSGEN_CORE_HELPERS_H

#include <ranges>

#include "helpers/fold.h"
#include "helpers/templates.h"
#include "sqsgen/types.h"

namespace sqsgen::core::helpers {
  namespace ranges = std::ranges;
  namespace views = ranges::views;

  template <class Matrix>
  std::vector<std::vector<typename Matrix::Scalar>> eigen_to_stl(Matrix const& m) {
    std::vector<std::vector<typename Matrix::Scalar>> result;
    result.reserve(m.rows());
    for (auto row : m.rowwise())
      result.push_back(std::vector<typename Matrix::Scalar>(row.begin(), row.end()));
    return result;
  }

  template <class Matrix>
  Matrix stl_to_eigen(std::vector<std::vector<typename Matrix::Scalar>> const& v) {
    if (v.size() == 0) throw std::invalid_argument("empty vector");
    auto first_size = v.front().size();
    if (first_size == 0) throw std::invalid_argument("empty vector as first argument");

    if (!ranges::all_of(v, [&](auto const& row) { return row.size() == first_size; }))
      throw std::invalid_argument("not all vector have the same size");

    Matrix result(v.size(), first_size);
    for (auto i = 0; i < v.size(); ++i) {
      for (auto j = 0; j < first_size; ++j) result(i, j) = v[i][j];
    }

  return result;
}

}  // namespace sqsgen::core::helpers

#endif  // SQSGEN_CORE_HELPERS_H
