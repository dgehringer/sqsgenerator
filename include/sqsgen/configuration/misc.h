

#ifndef SQSGEN_CONFIGURATION_MISC_H
#define SQSGEN_CONFIGURATION_MISC_H

#include <cstdint>
#include <optional>

#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"
namespace sqsgen::configuration {

  template <class T, sublattice_mode Mode> using structure_input
      = sublattice_mode_nesting<core::structure<T>, Mode>;

  static constexpr iterations_t iterations_default = 500000;
  static constexpr iterations_t chunk_size_default = 100000;
  template <class T> static constexpr T atol_default = 1.0e-3;
  template <class T> static constexpr T rtol_default = 1.0e-5;
  template <class T> static constexpr T bin_width_default = 0.05;
  template <class T> static constexpr T peak_isolation_default = 0.25;

  using iterations_input = std::optional<unsigned long long>;
  using iterations = unsigned long long;

  using chunck_size_input = std::optional<unsigned long long>;
  using chunck_size_t = unsigned long long;

  using max_results_per_objective_input = std::optional<iterations_t>;
  using max_results_per_objective_t = iterations_t;

  template <class T> using atol_input = std::optional<T>;
  template <class T> using atol = T;

  template <class T> using rtol_input = std::optional<T>;
  template <class T> using rtol = T;

  template <class T> using bin_width_input = std::optional<T>;
  template <class T> using bin_width = T;

  template <class T> using peak_isolation_input = std::optional<T>;
  template <class T> using peak_isolation = T;

  using seed_spec = std::optional<std::uint64_t>;
  template <sublattice_mode Mode> using seed_input = sublattice_mode_nesting<seed_spec, Mode>;
  template <sublattice_mode Mode> using seed = seed_spec;

  template <sublattice_mode Mode>
  result_t<iterations> validate_iterations_input(const iterations_input&,
                                                 const structure_input<T, sublattice_mode Mode>);
  result_t<iterations> validate_chunk_size_input(const iterations_input&,
                                                 std::optional<iterations_t>);
  template <class T> result_t<atol<T>> validate_atol_input(const atol_input<T>&);
  template <class T> result_t<rtol<T>> validate_atol_input(const rtol_input<T>&);

  template <class T> result_t<bin_width<T>> validate_bin_width_input(const bin_width_input<T>&);
  template <class T>
  result_t<peak_isolation<T>> validate_peak_isolation_input(const peak_isolation_input<T>&);

  template <sublattice_mode Mode>
  result_t<seed<Mode>> validate_seed(const seed_input<Mode>& mode, iteration_mode imode);

}  // namespace sqsgen::configuration
#endif
