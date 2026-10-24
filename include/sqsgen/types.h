
#ifndef SQSGEN_TYPES_H
#define SQSGEN_TYPES_H

#include <Eigen/src/Core/util/EmulateArray.h>

#include <Eigen/Core>
#include <boost/multiprecision/cpp_int.hpp>
#include <cstdint>
#include <expected>
#include <map>
#include <type_traits>
#include <unsupported/Eigen/CXX11/Tensor>
#include <utility>
#include <vector>

#include "sqsgen/core/helpers/string.h"
#include "sqsgen/core/sorted_vector.h"

namespace sqsgen {

  using specie_t = std::uint_fast8_t;
  using rank_t = boost::multiprecision::cpp_int;
  using configuration_t = std::vector<specie_t>;

  template <class T> using sorted_vector = sqsgen::core::sorted_vector<T>;

  template <class T> using counter = std::map<T, size_t>;

  template <class T> using lattice_t = Eigen::Matrix<T, 3, 3>;

  template <class T> using coords_t = Eigen::Matrix<T, Eigen::Dynamic, 3>;

  template <class T> using matrix_t = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;

  template <class T> using cube_t = Eigen::Tensor<T, 3>;

  template <class T, class U>
    requires std::is_integral_v<T> && std::is_integral_v<U>
  using index_mapping_t = std::pair<std::map<T, U>, std::map<U, T>>;

  using shell_matrix_t = matrix_t<std::size_t>;

  template <class T> using bounds_t = std::pair<T, T>;

  enum class prec { Single, Double };

  enum class iteration_mode { random, systematic };

  enum class sublattice_mode { interact, split };

  using composition_t = std::map<specie_t, std::size_t>;

  using nanoseconds_t = std::uint64_t;

  template <size_t N> using string_literal = core::helpers::string_literal<N>;

  enum class error_code {
    unknown,
    out_of_range,
    bad_argument,
    invalid_size,
    not_found,
    parsing,
    type_error
  };

  template <class T, sublattice_mode Mode> using sublattice_mode_nesting
      = std::conditional_t<Mode == sublattice_mode::interact, T, std::vector<T>>;

  namespace configuration {

    struct configuration_error {
      std::string key;
      error_code code;
      std::string msg;
    };

    template <class T> using result_t = std::expected<T, configuration_error>;

    enum class shell_radii_detection { naive, peak };

  };  // namespace configuration

  namespace io {

    struct parser_error {
      error_code code;
      std::string msg;
    };

    template <class T> using result_t = std::expected<T, parser_error>;

    enum class structure_format { sqsgen, pymatgen, ase, cif, poscar, pdb };

    enum class wire_format { json, yaml, cbor, msgpack, beve };
  };  // namespace io

  namespace core {

    enum class array_mode { nested, packed };

    template <class T, int N> struct packed_tensor {
      std::array<std::int64_t, N> shape{};  // extents, one per dimension
      std::vector<T> data{};                // prod(shape) values, flat
    };

    template <class T, int N> struct nested_type {
      using type = std::vector<typename nested_type<T, N - 1>::type>;
    };

    template <class T> struct nested_type<T, 0> {
      using type = T;
    };
    template <class T, int N> using nested_t = typename nested_type<T, N>::type;
    template <class T, int N> struct nested_tensor {
      static constexpr int rank = N;
      nested_t<T, N> value;
    };

    template <class T, int N, array_mode Mode> using tensor_representation_t
        = std::conditional_t<Mode == array_mode::nested, nested_t<T, N>, packed_tensor<T, N>>;

  };  // namespace core

  struct sublattice {
    sorted_vector<std::size_t> sites;
    composition_t composition;
  };

  enum Timing {
    TIMING_UNDEFINED = -1,
    TIMING_TOTAL = 0,
    TIMING_CHUNK_SETUP = 1,
    TIMING_LOOP = 2,
    TIMING_COMM = 3
  };

  template <class T> struct sro_parameter {
    std::size_t shell;
    specie_t i;
    specie_t j;
    T value;
  };

  template <class T> struct sqs_statistics_data {
    std::uint64_t finished{};
    std::uint64_t working{};
    std::uint64_t best_rank{};
    T best_objective{std::numeric_limits<T>::infinity()};
    std::map<Timing, nanoseconds_t> timings{{TIMING_TOTAL, 0},
                                            {TIMING_CHUNK_SETUP, 0},
                                            {TIMING_LOOP, 0},
                                            {TIMING_COMM, 0}};
  };

  template <class T> struct objective {
    T best;    // the best objective value
    T search;  // the search limit in case we want to enure that more that one result is found.
  };

  enum StructureFormat {
    STRUCTURE_FORMAT_JSON_SQSGEN = 0,
    STRUCTURE_FORMAT_JSON_PYMATGEN = 1,
    STRUCTURE_FORMAT_JSON_ASE = 2,
    STRUCTURE_FORMAT_CIF = 3,
    STRUCTURE_FORMAT_POSCAR = 4,
    STRUCTURE_FORMAT_PDB = 5,
  };

  template <class T> class sqs_callback_context {
    std::shared_ptr<std::stop_source> _stop;

  public:
    explicit sqs_callback_context(std::shared_ptr<std::stop_source> stop_source,
                                  sqs_statistics_data<T> stats)
        : _stop(std::move(stop_source)), statistics(stats) {}
    sqs_statistics_data<T> statistics;

    void stop() { _stop->request_stop(); }
  };

  using sqs_callback_t = std::function<void(
      std::variant<sqs_callback_context<float>, sqs_callback_context<double>>)>;

}  // namespace sqsgen
#endif  // SQSGEN_TYPES_H
