//
// Created by Dominik Gehringer on 19.09.24.
//

#ifndef SQSGEN_TYPES_HPP
#define SQSGEN_TYPES_HPP

#include <Eigen/Core>
#include <map>
#include <mp++/integer.hpp>
#include <set>
#include <unsupported/Eigen/CXX11/Tensor>
#include <vector>

#include "sqsgen/core/helpers/sorted_vector.h"

namespace sqsgen {

  using specie_t = std::uint_fast8_t;
  using rank_t = mppp::integer<2>;
  using configuration_t = std::vector<specie_t>;

  template <class T> using vset = core::helpers::sorted_vector<T>;

  template <class T> using counter = std::map<T, size_t>;

  template <class T>
    requires std::is_arithmetic_v<T>
  using lattice_t = Eigen::Matrix<T, 3, 3>;

  template <class T>
    requires std::is_arithmetic_v<T>
  using coords_t = Eigen::Matrix<T, Eigen::Dynamic, 3>;

  template <class T> using matrix_t = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;

  template <class T> using cube_t = Eigen::Tensor<T, 3>;

  template <class T> using stl_matrix_t = std::vector<std::vector<T>>;
  template <class T> using stl_cube_t = std::vector<std::vector<std::vector<T>>>;

  template <class T, class U>
    requires std::is_integral_v<T> && std::is_integral_v<U>
  using index_mapping_t = std::pair<std::map<T, U>, std::map<U, T>>;

  using usize_t = std::uint_fast32_t;

  using shell_matrix_t = matrix_t<usize_t>;

  template <class T> using bounds_t = std::pair<T, T>;

  template <class T>
    requires std::is_arithmetic_v<T>
  using shell_weights_t = std::map<usize_t, T>;

  struct _compare_usize {
    bool operator()(usize_t const& lhs, usize_t const& rhs) const { return lhs < rhs; }
  };

  enum Prec : std::uint8_t {
    PREC_SINGLE = 0,
    PREC_DOUBLE = 1,
  };

  enum IterationMode {
    ITERATION_MODE_INVALID = -1,
    ITERATION_MODE_RANDOM,
    ITERATION_MODE_SYSTEMATIC,
  };

  enum ShellRadiiDetection {
    SHELL_RADII_DETECTION_INVALID = -1,
    SHELL_RADII_DETECTION_NAIVE,
    SHELL_RADII_DETECTION_PEAK
  };

  enum SublatticeMode {
    SUBLATTICE_MODE_INVALID = -1,
    SUBLATTICE_MODE_INTERACT,
    SUBLATTICE_MODE_SPLIT,
  };

  using composition_t = std::map<specie_t, usize_t>;

  struct sublattice {
    vset<usize_t> sites;
    composition_t composition;
  };

}  // namespace sqsgen
#endif  // SQSGEN_TYPES_HPP