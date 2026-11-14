// DRAFT — not compiled.
//
// Everything the kernel reads (kernel_data, shared and const) and writes (kernel_scratch,
// one per thread), plus the conversions between the sparse configuration maps and the
// dense tensors the kernel uses.
//
// Neither the sublattice mode nor the objective type (sro / energy) appears here:
//   - sublattice mode: interact = one lattice with one range per sublattice,
//                      split    = one lattice per sublattice with one range
//   - objective type:  only changes how the coefficients A, B, s are computed
// Both are resolved by make_kernel_data.

#ifndef SQSGEN_OPTIMIZATION_KERNEL_DATA_H
#define SQSGEN_OPTIMIZATION_KERNEL_DATA_H

#include <cstdint>
#include <optional>
#include <vector>

#include "sqsgen/configuration.h"
#include "sqsgen/optimization/permutation.h"
#include "sqsgen/optimization/sro.h"
#include "sqsgen/types.h"

namespace sqsgen::optimization {

  namespace configuration = sqsgen::configuration;

  // objective_terms (the per-cell A, B, s of one objective term) lives in
  // sqsgen/optimization/sro.h, next to the evaluator (compute_pair_objective) that consumes it.

  // pairs of one lattice
  template <class T> struct pair_data {
    core::pair_list_t pairs;                  // (site, site, compact shell), each pair once
    std::vector<shell_t> shells;              // compact shell index -> real shell number
    pair_tensor_t<T> prefactors;              // f, to recompute alpha = 1 - N f on a hit
    objective_terms<pair_tensor_t<T>> terms;  // (shell, xi, eta)
  };

  // triplets of one lattice
  template <class T> struct triplet_data {
    core::triplet_list_t triplets;               // (sites in canonical order, compact class)
    core::triplet_class_mapping_t classes;       // compact class index -> (s1, s2, s3)
    triplet_tensor_t<T> prefactors;              // f, to recompute beta = 1 - N f on a hit
    objective_terms<triplet_tensor_t<T>> terms;  // (class, xi, eta, zeta)
  };

  // one lattice that is shuffled and evaluated on its own
  template <class T> struct lattice_data {
    configuration_t species_packed;           // initial configuration, sorted layout
    std::vector<specie_t> species;            // packed index -> atomic number
    std::vector<site_t> order;                // layout position -> input site
    std::vector<site_range> ranges;           // shuffled ranges of species_packed
    std::vector<std::uint64_t> seeds;         // one per range (random, monte carlo)
    std::optional<pair_data<T>> pairs;        // nullopt: no pairs
    std::optional<triplet_data<T>> triplets;  // nullopt: no triplets
  };

  template <class T> struct kernel_data {
    std::vector<lattice_data<T>> lattices;  // interact: 1, split: one per sublattice
    std::uint64_t block_size;               // random: reseed every block_size iterations
  };

  struct statistics {
    std::uint64_t iterations{0};
    std::uint64_t hits{0};
    double seconds{0.0};
  };

  // what one thread writes for one lattice; allocated once, reused every iteration
  struct lattice_scratch {
    configuration_t configuration;             // current configuration
    std::vector<rng> generators;               // one per range
    pair_tensor_t<count_t> pair_counts;        // N(shell, xi, eta), unsymmetrized
    triplet_tensor_t<count_t> triplet_counts;  // N(class, xi, eta, zeta), unsymmetrized
  };

  struct kernel_scratch {
    std::vector<lattice_scratch> lattices;  // same order as kernel_data::lattices
    statistics stats;
  };

  // allocates every buffer with the shapes of `data`
  template <class T> kernel_scratch make_scratch(const kernel_data<T>& data);

  // ======================================================================= coefficients

  // sro: omega = scale * weights,  A = omega (target - 1),  B = omega f,  s = 1 (0 if omega = 0)
  // scale = (1 - lambda) / W_pairs  or  lambda / W_triplets
  template <class Tensor, class T> objective_terms<Tensor> sro_terms(const Tensor& prefactors,
                                                                     const Tensor& weights,
                                                                     const Tensor& target, T scale);

  // energy: A = 0,  B = |J|,  s = sign(J)   =>   s |A + N B| = J N  (N >= 0)
  template <class Tensor> objective_terms<Tensor> energy_terms(const Tensor& interactions);

  // ======================================================================= map <-> tensor

  // Sparse map -> dense tensor (shell, xi, eta). Every cell (s, xi, eta) is looked up as
  // pair_key{shells[s], sorted(species[xi], species[eta])}, so both (xi, eta) and (eta, xi)
  // get the value. Cells without an entry get `missing` (0 for targets and interactions).
  template <class T> pair_tensor_t<T> to_pair_tensor(const pair_sro_t<T>& values,
                                                     const std::vector<shell_t>& shells,
                                                     const configuration_t& species, T missing);

  // Sparse map -> dense tensor (class, xi, eta, zeta). Every cell is looked up by its canonical
  // key canonical_triplet(classes[c], {species[xi], species[eta], species[zeta]}), so all
  // orderings of interchangeable corners get the same value.
  template <class T>
  triplet_tensor_t<T> to_triplet_tensor(const triplet_sro_t<T>& values,
                                        const core::triplet_class_mapping_t& classes,
                                        const configuration_t& species, T missing);

  // Dense -> sparse, for the results (alpha, beta of a hit); canonical cells only.
  template <class T> pair_sro_t<T> to_pair_sro(const pair_tensor_t<T>& tensor,
                                               const std::vector<shell_t>& shells,
                                               const std::vector<specie_t>& species);

  template <class T> triplet_sro_t<T> to_triplet_sro(const triplet_tensor_t<T>& tensor,
                                                     const core::triplet_class_mapping_t& classes,
                                                     const std::vector<specie_t>& species);

  // configuration -> kernel data; the only place that knows the sublattice mode and the
  // objective type. Defined in its own file (it needs the structure and shell code).
  template <class T, sublattice_mode Mode>
  kernel_data<T> make_kernel_data(const configuration::configuration<T, Mode>& config);

}  // namespace sqsgen::optimization

#include "sqsgen/impl/optimization/kernel_data_impl.hpp"
#include "sqsgen/impl/optimization/make_kernel_data_impl.hpp"

#endif  // SQSGEN_OPTIMIZATION_KERNEL_DATA_H
