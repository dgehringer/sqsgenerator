
#ifndef SQSGEN_CORE_EIGEN_H
#define SQSGEN_CORE_EIGEN_H

#include <expected>
#include <format>

#include "sqsgen/types.h"

namespace sqsgen::core {

  struct tensor_shape_error {
    enum class kind { empty_dimension, ragged, size_mismatch, negative_extent };
    kind which;
    std::string message;
  };

  error_code map_error_kind(tensor_shape_error::kind kind);

  template <class T, int R, int C> Eigen::Tensor<T, 2> to_tensor(const Eigen::Matrix<T, R, C>& m);

  template <class T>
  std::expected<lattice_t<T>, tensor_shape_error> tensor_to_lattice(const Eigen::Tensor<T, 2>& t);

  template <class T>
  std::expected<coords_t<T>, tensor_shape_error> tensor_to_coords(const Eigen::Tensor<T, 2>& t);

  template <class T> matrix_t<T> tensor_to_matrix(const Eigen::Tensor<T, 2>& t);

  template <class Target, class T, int N> Target tensor_as(const Eigen::Tensor<T, N>& t);

  template <array_mode Mode, class T, int N>
  tensor_representation_t<T, N, Mode> tensor_as(const Eigen::Tensor<T, N>& t);

  template <array_mode Mode, class T, int R, int C>
  tensor_representation_t<T, 2, Mode> tensor_as(const Eigen::Matrix<T, R, C>& t);

  template <class T>
  std::expected<lattice_t<T>, tensor_shape_error> lattice_from(const packed_tensor<T, 2>& p);

  template <class T>
  std::expected<coords_t<T>, tensor_shape_error> coords_from(const packed_tensor<T, 2>& p);

  template <class T>
  std::expected<matrix_t<T>, tensor_shape_error> matrix_from(const packed_tensor<T, 2>& p);

  template <class T, int N>
  std::expected<Eigen::Tensor<T, N>, tensor_shape_error> tensor_from(const packed_tensor<T, N>& p);

  template <class T, int N>
  std::expected<Eigen::Tensor<T, N>, tensor_shape_error> tensor_from(const nested_t<T, N>& v);

  template <array_mode Mode, class T, int N>
  std::expected<Eigen::Tensor<T, N>, tensor_shape_error> tensor_from(
      const tensor_representation_t<T, N, Mode>& v);

  template <class T>
  std::expected<lattice_t<T>, tensor_shape_error> lattice_from(const nested_t<T, 2>& p);

  template <class T>
  std::expected<coords_t<T>, tensor_shape_error> coords_from(const nested_t<T, 2>& p);

  template <class T>
  std::expected<matrix_t<T>, tensor_shape_error> matrix_from(const nested_t<T, 2>& p);

}  // namespace sqsgen::core

#include "sqsgen/impl/core/eigen_impl.hpp"

#endif  // SQSGEN_CORE_EIGEN_H
