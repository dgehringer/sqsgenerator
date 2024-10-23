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
  using specie_t = uint8_t;
  using rank_t = mppp::integer<2>;
  using configuration_t = std::vector<specie_t>;

  template <class T> using counter = std::unordered_map<T, size_t>;

  template <class T>
    requires std::is_arithmetic_v<T>
  using lattice_t = Eigen::Matrix<T, 3, 3>;

  template <class T>
    requires std::is_arithmetic_v<T>
  using coords_t = Eigen::Matrix<T, 3, Eigen::Dynamic>;

  template <class T>
  using matrix_t = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;

  using pair_shell_matrix_t = std::variant<matrix_t<std::uint8_t>, matrix_t<std::uint16_t>, matrix_t<std::uint32_t>, matrix_t<std::uint64_t>>;



}  // namespace sqsgen
#endif  // SQSGEN_TYPES_HPP