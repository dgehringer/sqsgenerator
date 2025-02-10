//
// Created by Dominik Gehringer on 19.09.24.
//

#ifndef SQSGEN_TYPES_HPP
#define SQSGEN_TYPES_HPP

#include <Eigen/Core>
#include <map>
#include <mp++/integer.hpp>
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
    bool operator()(usize_t const &lhs, usize_t const &rhs) const { return lhs < rhs; }
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

  using iterations_t = unsigned long long;

  using nanoseconds_t = iterations_t;

  using thread_config_t = std::variant<usize_t, std::vector<usize_t>>;

  struct sublattice {
    vset<usize_t> sites;
    composition_t composition;
  };

  template <class, SublatticeMode> struct sqs_result {};

  template <class T> struct sqs_result<T, SUBLATTICE_MODE_INTERACT> {
    T objective;
    configuration_t species;
    cube_t<T> sro;

    sqs_result(T objective, configuration_t species, cube_t<T> sro)
        : objective(objective), species(std::move(species)), sro(std::move(sro)) {}
    // compatibility constructor to
    sqs_result(T, T objective, configuration_t species, cube_t<T> sro)
        : sqs_result(objective, species, std::move(sro)) {}

    static sqs_result empty(auto num_atoms, auto num_shells, auto num_species) {
      return {std::numeric_limits<T>::infinity(), configuration_t(num_atoms),
              cube_t<T>(num_shells, num_species, num_species)};
    }
  };

  template <class T> struct sqs_result<T, SUBLATTICE_MODE_SPLIT> {
    T objective;
    std::vector<sqs_result<T, SUBLATTICE_MODE_INTERACT>> sublattices;

    sqs_result(T objective, std::vector<sqs_result<T, SUBLATTICE_MODE_INTERACT>> const &sublattices)
        : objective(objective), sublattices(sublattices) {}

    sqs_result(T objective, std::vector<T> const &objectives,
               const std::vector<configuration_t> &species, std::vector<cube_t<T>> const &sro)
        : objective(objective) {
      if (objectives.size() != species.size() || objectives.size() != sro.size())
        throw std::invalid_argument("invalid number entries");
      sublattices.reserve(objectives.size());
      core::helpers::for_each(
          [&](auto &&index) {
            sublattices.push_back({objectives[index], species[index], sro[index]});
          },
          species.size());
    }

    template <std::ranges::range RNumAtoms, std::ranges::range RNumShells,
              std::ranges::range RNumSpecies>
    static sqs_result empty(RNumAtoms &&range_num_atoms, RNumShells &&range_num_shells,
                            RNumSpecies &&range_num_species) {
      using namespace sqsgen::core::helpers;
      auto num_atoms = as<std::vector>{}(range_num_atoms);
      auto num_shells = as<std::vector>{}(range_num_shells);
      auto num_species = as<std::vector>{}(range_num_species);
      if (num_species.size() != num_shells.size() || num_species.size() != num_atoms.size())
        throw std::invalid_argument("invalid sizes");
      return {std::numeric_limits<T>::infinity(),
              as<std::vector>{}(range(num_atoms.size()) | views::transform([&](auto &&index) {
                                  return sqs_result<T, SUBLATTICE_MODE_INTERACT>::empty(
                                      num_atoms[index], num_shells[index], num_species[index]);
                                }))};
    }
  };

  template <class T> struct sqs_statistics_data {
    iterations_t iterations{};
    std::map<std::string, nanoseconds_t> timings{{"total", 0},
                                                 {"chunk_setup", 0},
                                                 {"loop", 0},
                                                 {"sync", 0}};
    std::map<iterations_t, T> history;
  };

}  // namespace sqsgen
#endif  // SQSGEN_TYPES_HPP