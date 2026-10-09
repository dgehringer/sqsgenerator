

#ifndef SQSGEN_CORE_HELPERS_EIGEN_H
#define SQSGEN_CORE_HELPERS_EIGEN_H

#include <vector>

#include "sqsgen/types.h"

namespace sqsgen::core::helpers {
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
    for (auto i = 0; i < d[0]; ++i) {
      result[i].resize(d[1]);
      for (auto j = 0; j < d[1]; ++j) {
        result[i][j].resize(d[2]);
        for (auto k = 0; k < d[2]; ++k) result[i][j][k] = c(i, j, k);
      }
    }
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
    for (auto i = 0; i < static_cast<index_t>(v.size()); ++i)
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
        for (auto k = 0; k < second_size; ++j) result(i, j, k) = v[i][j][k];
    return result;
  }
}  // namespace sqsgen::core::helpers

#endif SQSGEN_CORE_HELPERS_EIGEN_H
