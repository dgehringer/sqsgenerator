//
// Created by Dominik Gehringer on 21.09.24.
//

#ifndef SQSGEN_CORE_HELPERS_MISC_H
#define SQSGEN_CORE_HELPERS_MISC_H

#include <ranges>

#include "sqsgen/log.h"
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

  template <class T> stl_cube_t<typename cube_t<T>::Scalar> eigen_to_stl(cube_t<T> const& c) {
    const typename cube_t<T>::Dimensions& d = c.dimensions();
    stl_cube_t<typename cube_t<T>::Scalar> result(d[0]);
    for_each(
        [&](auto&& i) {
          result[i].resize(d[1]);
          for_each(
              [&](auto&& j) {
                result[i][j].resize(d[2]);
                for_each([&](auto&& k) { result[i][j][k] = c(i, j, k); }, d[2]);
              },
              d[1]);
        },
        d[0]);
    return result;
  }

  template <class Matrix, int Rows = Matrix::Base::RowsAtCompileTime,
            int Cols = Matrix::Base::ColsAtCompileTime>
  Matrix stl_to_eigen(std::vector<std::vector<typename Matrix::Scalar>> const& v) {
    if (v.size() == 0) throw std::invalid_argument("empty vector");
    using index_t = typename Matrix::Index;
    index_t first_size = v.front().size();
    if (first_size == 0) throw std::invalid_argument("empty vector as first argument");
    if (!ranges::all_of(v, [&](auto const& row) { return row.size() == first_size; }))
      throw std::invalid_argument("not all vector have the same size");

    if ((Rows != Eigen::Dynamic && Rows != v.size())
        || (Cols != Eigen::Dynamic && Cols != first_size))
      throw std::out_of_range(format_string("invalid matrix size: %ix%i compared to %ux%u", Rows,
                                            Cols, v.size(), first_size));

    Matrix result(v.size(), first_size);
    for (auto i = 0; i < v.size(); ++i)
      for (auto j = 0; j < first_size; ++j) result(i, j) = v[i][j];
    return result;
  }

  template <class Tensor> Tensor stl_to_eigen(stl_cube_t<typename Tensor::Scalar> const& v) {
    using index_t = typename Tensor::Index;
    index_t zero_size = v.size();
    if (zero_size == 0) throw std::invalid_argument("empty vector");

    index_t first_size = v.front().size();
    if (first_size == 0) throw std::invalid_argument("empty vector in first dimension");
    index_t second_size = v.front().front().size();
    if (second_size == 0) throw std::invalid_argument("empty vector in second dimension");
    if (!ranges::all_of(v, [&](auto const& face) {
          return face.size() == first_size && ranges::all_of(face, [&](auto const& row) {
                   return static_cast<index_t>(row.size()) == second_size;
                 });
        }))
      throw std::invalid_argument("not all vector have the same size");
    Tensor result(v.size(), first_size, second_size);
    for (auto i = 0; i < zero_size; ++i)
      for (auto j = 0; j < first_size; ++j)
        for (auto k = 0; k < second_size; ++k) result(i, j, k) = v[i][j][k];
    return result;
  }

  template <class U, ranges::input_range R, class T = ranges::range_value_t<R>>
    requires std::is_integral_v<U>
  index_mapping_t<T, U> make_index_mapping(R&& r) {
    auto elements = sorted_vector<T>(r);
    std::sort(elements.begin(), elements.end());
    U index = 0;
    std::map<U, T> index_map;
    std::transform(elements.begin(), elements.end(), std::inserter(index_map, index_map.begin()),
                   [&](auto const& val) { return std::make_pair(index++, val); });
    std::map<T, U> reverse_map;
    std::transform(index_map.begin(), index_map.end(),
                   std::inserter(reverse_map, reverse_map.begin()),
                   [&](auto const& item) { return std::make_pair(item.second, item.first); });

    return index_mapping_t<T, U>(reverse_map, index_map);
  }

  template <ranges::range R, class T = ranges::range_value_t<R>> counter<T> count(R&& r) {
    counter<T> result{};
    for (auto e : r) {
      if (result.contains(e))
        ++result[e];
      else
        result.emplace(e, 1);
    }
    return result;
  }

  inline std::string to_uppercase(std::string_view input) {
    std::string result{input};
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
  }
}  // namespace sqsgen::core::helpers

#endif  // SQSGEN_CORE_HELPERS_H
