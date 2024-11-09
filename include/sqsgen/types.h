//
// Created by Dominik Gehringer on 19.09.24.
//

#ifndef SQSGEN_TYPES_HPP
#define SQSGEN_TYPES_HPP

#include <Eigen/Core>
#include <mp++/integer.hpp>
#include <unordered_map>
#include <vector>

namespace sqsgen {

  using specie_t = std::uint_fast8_t;
  using rank_t = mppp::integer<2>;
  using configuration_t = std::vector<specie_t>;

  template <class T> using counter = std::unordered_map<T, size_t>;

  template <class T>
    requires std::is_arithmetic_v<T>
  using lattice_t = Eigen::Matrix<T, 3, 3>;

  template <class T>
    requires std::is_arithmetic_v<T>
  using coords_t = Eigen::Matrix<T, Eigen::Dynamic, 3>;

  template <class T> using matrix_t = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;

  template <class T> using stl_matrix_t = std::vector<std::vector<T>>;

  template <class T>
    requires std::is_arithmetic_v<T>
  using shell_weights_t = std::unordered_map<std::size_t, T>;

  template <class T, class U>
    requires std::is_integral_v<T> && std::is_integral_v<U>
  using index_mapping_t = std::pair<std::unordered_map<T, U>, std::unordered_map<U, T>>;

  using usize_t = std::uint_fast32_t;

  using shell_matrix_t = matrix_t<usize_t>;

}  // namespace sqsgen
#endif  // SQSGEN_TYPES_HPP