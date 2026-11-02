

#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/optimization/sro.h"
#include "sqsgen/types.h"

namespace sqsgen::optimization {

  namespace helpers = sqsgen::core::helpers;

  template <class T>
  pair_list_t make_pair_list(const core::structure<T>& structure, const std::vector<T>& radii,
                             const std::map<shell_t, T>& weights, T atol, T rtol) {
    const auto shell_to_index = helpers::reversed_index_map<shell_t>(weights | std::views::keys);

    const auto shell_matrix = structure.shell_matrix(radii, atol, rtol);
    const std::size_t num_sites = structure.size();

    pair_list_t pairs;
    for (std::size_t i = 0; i < num_sites; ++i)
      for (std::size_t j = i + 1; j < num_sites; ++j) {
        const auto it = shell_to_index.find(static_cast<shell_t>(shell_matrix(i, j)));
        if (it == shell_to_index.end()) continue;  // shell not selected
        pairs.push_back(
            pair{.i = static_cast<site_t>(i), .j = static_cast<site_t>(j), .shell = it->second});
      }
    return pairs;
  }

  template <class T>
  pair_objective_coefficients<T> pair_coefficients(const pair_tensor_t<T>& scaled_pair_weights,
                                                   const pair_tensor_t<T>& alpha_target,
                                                   const pair_tensor_t<T>& prefactors) {
    pair_tensor_t<T> a = scaled_pair_weights * (alpha_target - alpha_target.constant(T{1}));
    pair_tensor_t<T> b = scaled_pair_weights * prefactors;
    return {std::move(a), std::move(b)};
  }

  template <class T> pair_tensor_t<T> scaled_pair_weights(const pair_tensor_t<T>& pair_weights,
                                                          const std::map<shell_t, T>& weights) {
    const auto shell_to_index = helpers::reversed_index_map<shell_t>(weights | std::views::keys);

    pair_tensor_t<T> scaled = pair_weights;
    for (const auto& [shell, w_s] : weights) {
      const auto s = static_cast<Eigen::Index>(shell_to_index.at(shell));
      scaled.chip(s, 0) = scaled.chip(s, 0) * w_s;
    }
    return scaled;
  }

  void count_pairs(pair_tensor_t<count_t>& counts, const pair_list_t& pairs,
                   configuration_t const& species) {
    counts.setConstant(0);
    for (const pair& p : pairs) ++counts(p.shell, species[p.i], species[p.j]);
  }

  // Pair objective in the hot path:
  //
  //   O = sum_s sum_{xi <= eta} | A^s_{xi eta} + N^s_{xi eta} * B^s_{xi eta} |
  //
  //   N^s_{xi eta} = bonds(s, xi, eta) + bonds(s, eta, xi)   for xi < eta  (pair list stores each
  //   bond once) N^s_{xi xi}  = bonds(s, xi, xi) A, B: precomputed coefficients (see
  //   pair_objective_coefficients), read at the canonical cell xi <= eta
  template <class T> T compute_pair_objective(const pair_tensor_t<count_t>& bonds,
                                              const pair_objective_coefficients<T>& coeffs) {
    const Eigen::Index num_shells = bonds.dimension(0);
    const Eigen::Index num_species = bonds.dimension(1);

    T objective{0};
    for (Eigen::Index s = 0; s < num_shells; ++s)
      for (Eigen::Index xi = 0; xi < num_species; ++xi) {
        // diagonal: same species
        const T n_diag = static_cast<T>(bonds(s, xi, xi));
        objective += std::abs(coeffs.a(s, xi, xi) + n_diag * coeffs.b(s, xi, xi));

        // off-diagonal: sum both cells, since each bond landed in one of them
        for (Eigen::Index eta = xi + 1; eta < num_species; ++eta) {
          const T n = static_cast<T>(bonds(s, xi, eta) + bonds(s, eta, xi));
          objective += std::abs(coeffs.a(s, xi, eta) + n * coeffs.b(s, xi, eta));
        }
      }
    return objective;
  }

}  // namespace sqsgen::optimization
