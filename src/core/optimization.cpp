//
// Created by Dominik Gehringer on 18.03.25.
//

#include "sqsgen/core/optimization.h"

#include "sqsgen/core/helpers.h"

namespace sqsgen::core::optimization {

  namespace ranges = std::ranges;
  namespace views = ranges::views;

  template <class T> cube_t<T> scaled_pair_weights(cube_t<T> const& pair_weights,
                                                   shell_weights_t<T> const& weights,
                                                   std::size_t num_species) {
    auto w(pair_weights);
    auto shells_rmap
        = std::get<1>(helpers::make_index_mapping<std::size_t>(weights | views::elements<0>));
    for (auto s = 0; s < weights.size(); ++s) {
      auto w_s = weights.at(shells_rmap.at(s));
      for (auto xi = 0; xi < num_species; ++xi)
        for (auto eta = xi; eta < num_species; ++eta) {
          w(s, xi, eta) *= w_s;
          if (xi == eta) continue;
          w(s, eta, xi) *= w_s;
        }
    }
    return w;
  }

  template <class T>
  std::tuple<std::vector<std::size_t>, std::vector<bounds_t<std::size_t>>, std::vector<std::size_t>>
  compute_shuffling_bounds(structure<T> const& structure,
                           std::vector<sublattice> const& composition) {
    using namespace core::helpers;
    auto num_atoms = structure.size();
    auto sublattice_index = [&]<typename T0>(T0&& site) -> std::size_t {
      std::size_t search;
      if constexpr (std::is_same_v<std::decay_t<T0>, std::size_t>)
        search = site;
      else
        search = site.index;
      auto num_sl = composition.size();
      for (std::size_t sl_index = 0; sl_index < num_sl; ++sl_index)
        if (composition[sl_index].sites.contains(search)) return sl_index;
      return num_sl;
    };
    auto [sorted, sort_order] = structure.sorted_with_indices(
        [&](auto&& a, auto&& b) { return sublattice_index(a) < sublattice_index(b); });

    auto sublattice_indices = as<std::vector>{}(sort_order | views::transform(sublattice_index));

    assert(ranges::all_of(range(num_atoms - 1), [&](auto&& i) {
      return sublattice_indices[i] <= sublattice_indices[i + 1];
    }));
    std::vector<bounds_t<std::size_t>> bounds;
    bounds.reserve(composition.size());

    std::size_t lower_index{0};
    for (std::size_t i = 0; i < num_atoms; ++i) {
      if (sublattice_indices[i] != sublattice_indices[lower_index]) {
        bounds.emplace_back(lower_index, i);
        lower_index = i;
        if (sublattice_indices[i] == composition.size()) break;
      } else if (i == num_atoms - 1)
        bounds.emplace_back(lower_index, i + 1);
    }
    assert(bounds.size() == composition.size());
    return std::make_tuple(sorted, bounds, sort_order);
  }

  void count_bonds(cube_t<std::size_t>& bonds, auto const& pairs, configuration_t const& species) {
    bonds.setConstant(0);
    for (auto& [i, j, s] : pairs) bonds(s, species[i], species[j])++;
  }

  template <class T> T compute_objective(cube_t<T>& sro, cube_t<std::size_t> const& bonds,
                                         cube_t<T> const& prefactors, cube_t<T> const& pair_weights,
                                         cube_t<T> const& target, std::size_t num_shells,
                                         std::size_t num_species) {
    sro.setConstant(T(0.0));
    T objective{0.0};
    for (auto s = 0; s < num_shells; ++s)
      // symmetrize upper half of the matrix \alpha_{\xi\eta}^i
      for (auto xi = 0; xi < num_species; ++xi) {
        T sigma_s_xi_xi = T(1.0) - static_cast<T>(bonds(s, xi, xi)) * prefactors(s, xi, xi);
        sro(s, xi, xi) = sigma_s_xi_xi;
        objective
            += pair_weights(s, xi, xi) * core::helpers::absolute(sigma_s_xi_xi - target(s, xi, xi));
        for (auto eta = xi + 1; eta < num_species; ++eta) {
          auto pair_bonds = bonds(s, xi, eta) + bonds(s, eta, xi);
          T sigma_s_xi_eta = T(1.0) - static_cast<T>(pair_bonds) * prefactors(s, xi, eta);
          sro(s, xi, eta) = sigma_s_xi_eta;
          sro(s, eta, xi) = sigma_s_xi_eta;
          objective += pair_weights(s, xi, eta)
                       * core::helpers::absolute(sigma_s_xi_eta - target(s, xi, eta));
        }
      }

    return objective;
  }

  template <> cube_t<double> scaled_pair_weights(cube_t<double> const& pair_weights,
                                                 shell_weights_t<double> const& weights,
                                                 std::size_t num_species);
  template <> cube_t<float> scaled_pair_weights(cube_t<float> const& pair_weights,
                                                shell_weights_t<float> const& weights,
                                                std::size_t num_species);

  template <>
  std::tuple<std::vector<std::size_t>, std::vector<bounds_t<std::size_t>>, std::vector<std::size_t>>
  compute_shuffling_bounds(structure<double> const& structure,
                           std::vector<sublattice> const& composition);
  template <>
  std::tuple<std::vector<std::size_t>, std::vector<bounds_t<std::size_t>>, std::vector<std::size_t>>
  compute_shuffling_bounds(structure<float> const& structure,
                           std::vector<sublattice> const& composition);

  template <> double compute_objective(cube_t<double>& sro, cube_t<std::size_t> const& bonds,
                                       cube_t<double> const& prefactors,
                                       cube_t<double> const& pair_weights,
                                       cube_t<double> const& target, std::size_t num_shells,
                                       std::size_t num_species);

  template <> float compute_objective(cube_t<float>& sro, cube_t<std::size_t> const& bonds,
                                      cube_t<float> const& prefactors,
                                      cube_t<float> const& pair_weights,
                                      cube_t<float> const& target, std::size_t num_shells,
                                      std::size_t num_species);

}  // namespace sqsgen::core::optimization
