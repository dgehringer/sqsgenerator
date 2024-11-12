//
// Created by Dominik Gehringer on 21.09.24.
//

#ifndef SQSGEN_CORE_HELPERS_H
#define SQSGEN_CORE_HELPERS_H

#include <ranges>
#include <unordered_set>

#include "helpers/as.h"
#include "helpers/enumerate.h"
#include "helpers/fold.h"
#include "helpers/for_each.h"
#include "helpers/hash.h"
#include "helpers/numeric.h"
#include "helpers/static_string.h"
#include "helpers/templates.h"
#include "helpers/zip.h"
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

  template <class U, ranges::input_range R, class T = ranges::range_value_t<R>>
    requires std::is_integral_v<U>
  index_mapping_t<T, U> make_index_mapping(R&& r) {
    auto elements = as<std::vector>{}(as<std::set>{}(r));
    std::sort(elements.begin(), elements.end());
    U index{0};
    std::map<U, T> index_map = as<std::map>{}(
         enumerate<U>(elements) );
    std::map<T, U> reverse_map = as<std::map>{}(index_map | views::transform([&](auto const& item) {
                                                  auto [k, v] = item;
                                                  return std::make_pair(v, k);
                                                }));
    return index_mapping_t<T, U>(reverse_map, index_map);
  }

}  // namespace sqsgen::core::helpers

#endif  // SQSGEN_CORE_HELPERS_H
