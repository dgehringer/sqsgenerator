

#ifndef SQSGEN_CONFIGURATION_MISC_H
#define SQSGEN_CONFIGURATION_MISC_H

#include <cstddef>
#include <cstdint>
#include <optional>

#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"
namespace sqsgen::configuration {

  template <class T, sublattice_mode Mode> using structure_t
      = sublattice_mode_nesting<core::structure<T>, Mode>;

  using iteration_mode_input = std::optional<iteration_mode>;

  using iterations_t = std::optional<std::uint64_t>;
  using iterations_input = std::optional<std::uint64_t>;

  using chunk_size_input = std::optional<std::uint64_t>;
  using chunk_size_t = std::uint64_t;

  using max_results_per_objective_input = std::optional<std::size_t>;
  using max_results_per_objective_t = std::optional<std::size_t>;

  template <class T> using atol_input = std::optional<T>;
  template <class T> using atol_t = T;

  template <class T> using rtol_input = std::optional<T>;
  template <class T> using rtol_t = T;

  template <class T> using bin_width_input = std::optional<T>;
  template <class T> using bin_width_t = T;

  template <class T> using peak_isolation_input = std::optional<T>;
  template <class T> using peak_isolation_t = T;

  using seed_spec = std::vector<std::optional<std::uint64_t>>;
  using seed_input = std::optional<seed_spec>;
  using seed_t = seed_spec;

  using keep_input = std::optional<std::size_t>;
  using keep_t = std::size_t;

  using thread_config_t = std::vector<std::optional<std::size_t>>;
  using thread_config_input = std::optional<
      std::variant<std::optional<std::size_t>, std::vector<std::optional<std::size_t>>>>;

  result_t<iteration_mode> validate_iteration_mode_input(iteration_mode,
                                                         const std::vector<sublattice>&);

  result_t<iterations_t> validate_iterations_input(const iterations_input&,
                                                   const std::vector<sublattice>&, iteration_mode);

  result_t<chunk_size_t> validate_chunk_size_input(const chunk_size_input&, const iterations_t&);
  template <class T> result_t<atol_t<T>> validate_atol_input(const atol_input<T>&);
  template <class T> result_t<rtol_t<T>> validate_rtol_input(const rtol_input<T>&);

  template <class T> result_t<bin_width_t<T>> validate_bin_width_input(const bin_width_input<T>&);
  template <class T>
  result_t<peak_isolation_t<T>> validate_peak_isolation_input(const peak_isolation_input<T>&);

  result_t<seed_t> validate_seed_input(const seed_input&, const std::vector<sublattice>&,
                                       iteration_mode);

  result_t<keep_t> validate_keep_input(const keep_input&);

  result_t<thread_config_t> validate_thread_config_input(const thread_config_input&);

  result_t<max_results_per_objective_t> validate_max_results_per_objective_input(
      const max_results_per_objective_input&);

  namespace defaults {
    static constexpr prec prec_default = prec::Single;
    static constexpr sublattice_mode sublattice_mode_default = sublattice_mode::interact;
    static constexpr std::size_t keep_default = 1UL;
    static constexpr std::size_t max_results_per_objective_default = 10UL;
    static constexpr std::uint64_t iterations_default = 500000;
    static constexpr chunk_size_t chunk_size_default = 100000;
    static constexpr iteration_mode iteration_mode_default = iteration_mode::random;
    static constexpr std::optional<std::uint64_t> seed_default = std::nullopt;
    template <class T> static constexpr T atol_default = 1.0e-3;
    template <class T> static constexpr T rtol_default = 1.0e-5;
    template <class T> static constexpr T bin_width_default = 0.05;
    template <class T> static constexpr T peak_isolation_default = 0.25;

  }  // namespace defaults

}  // namespace sqsgen::configuration
#endif
