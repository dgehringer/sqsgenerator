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

  enum Prec {
    PREC_INVALID = -1,
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

  using thread_config_t = std::vector<usize_t>;

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
      for (auto i = 0; i < objectives.size(); ++i)
        sublattices.push_back({objectives[i], species[i], sro[i]});
    }
  };

  enum Timing {
    TIMING_UNDEFINED = -1,
    TIMING_TOTAL = 0,
    TIMING_CHUNK_SETUP = 1,
    TIMING_LOOP = 2,
    TIMING_COMM = 3
  };

  template <class T> struct sro_parameter {
    usize_t shell;
    specie_t i;
    specie_t j;
    T value;
  };

  template <class T> struct sqs_statistics_data {
    iterations_t finished{};
    iterations_t working{};
    iterations_t best_rank{};
    T best_objective{std::numeric_limits<T>::infinity()};
    std::map<Timing, nanoseconds_t> timings{{TIMING_TOTAL, 0},
                                            {TIMING_CHUNK_SETUP, 0},
                                            {TIMING_LOOP, 0},
                                            {TIMING_COMM, 0}};
  };

  template <class T> struct objective {
    T value;
  };

  enum StructureFormat {
    STRUCTURE_FORMAT_JSON_SQSGEN = 0,
    STRUCTURE_FORMAT_JSON_PYMATGEN = 1,
    STRUCTURE_FORMAT_JSON_ASE = 2,
    STRUCTURE_FORMAT_CIF = 3,
    STRUCTURE_FORMAT_POSCAR = 4,
  };

  template <class T> class sqs_callback_context {
    std::shared_ptr<std::stop_source> _stop;

  public:
    explicit sqs_callback_context(std::shared_ptr<std::stop_source> stop_source,
                                  sqs_statistics_data<T> stats)
        : _stop(stop_source), statistics(stats) {}
    sqs_statistics_data<T> statistics;

    void stop() { _stop->request_stop(); }
  };

  using sqs_callback_t = std::function<void(
      std::variant<sqs_callback_context<float>, sqs_callback_context<double>>)>;

}  // namespace sqsgen
#endif  // SQSGEN_TYPES_HPP