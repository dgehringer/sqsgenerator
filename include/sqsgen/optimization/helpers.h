//
// Created by Dominik Gehringer on 06.01.25.
//

#ifndef SQSGEN_OPTIMIZATION_HELPERS_H
#define SQSGEN_OPTIMIZATION_HELPERS_H

#include "sqsgen/types.h"

namespace sqsgen::optimization::helpers {

  namespace ranges = std::ranges;
  namespace views = ranges::views;

  namespace detail {}

  enum class ScopedExecutionMode : uint8_t {
    SCOPED_EXECUTION_MODE_INIT,
    SCOPED_EXECUTION_MODE_DESTRUCT,
    SCOPED_EXECUTION_MODE_BOTH,
  };

  template <class Fn> struct scoped_execution {
    ScopedExecutionMode mode;
    Fn fn;

    explicit scoped_execution(Fn&& fn, ScopedExecutionMode mode
                                       = ScopedExecutionMode::SCOPED_EXECUTION_MODE_BOTH)
        : mode(mode), fn(fn) {
      if (mode == ScopedExecutionMode::SCOPED_EXECUTION_MODE_BOTH
          || mode == ScopedExecutionMode::SCOPED_EXECUTION_MODE_INIT)
        fn();
    }
    ~scoped_execution() {
      if (mode == ScopedExecutionMode::SCOPED_EXECUTION_MODE_BOTH
          || mode == ScopedExecutionMode::SCOPED_EXECUTION_MODE_DESTRUCT)
        fn();
    }
  };

  template <class T> cube_t<T> scaled_pair_weights(cube_t<T> const& pair_weights,
                                                   shell_weights_t<T> const& weights,
                                                   usize_t num_species) {
    auto w(pair_weights);
    auto shells_rmap
        = std::get<1>(core::helpers::make_index_mapping<usize_t>(weights | views::elements<0>));
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

  template <class T> auto compute_shuffling_bounds(core::structure<T> const& structure,
                                                   std::vector<sublattice> const& composition) {
    using namespace core::helpers;
    auto num_atoms = structure.size();
    auto sublattice_index = [&](auto&& site) -> usize_t {
      usize_t search;
      if constexpr (std::is_same_v<std::decay_t<decltype(site)>, usize_t>)
        search = site;
      else
        search = site.index;
      auto num_sl = composition.size();
      for (usize_t sl_index = 0; sl_index < num_sl; ++sl_index)
        if (composition[sl_index].sites.contains(search)) return sl_index;
      return num_sl;
    };
    auto [sorted, sort_order] = structure.sorted_with_indices(
        [&](auto&& a, auto&& b) { return sublattice_index(a) < sublattice_index(b); });

    auto sublattice_indices = as<std::vector>{}(sort_order | views::transform(sublattice_index));

    assert(ranges::all_of(range(num_atoms - 1), [&](auto&& i) {
      return sublattice_indices[i] <= sublattice_indices[i + 1];
    }));
    std::vector<bounds_t<usize_t>> bounds;
    bounds.reserve(composition.size());

    usize_t lower_index{0};
    for (usize_t i = 0; i < num_atoms; ++i) {
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

  void count_bonds(cube_t<usize_t>& bonds, auto const& pairs, configuration_t const& species) {
    bonds.setConstant(0);
    for (auto& [i, j, s] : pairs) {
      auto si{species[i]};
      auto sj{species[j]};
      ++bonds(s, si, sj);
    }
  }

  template <class T> T compute_objective(cube_t<T>& sro, cube_t<usize_t> const& bonds,
                                         cube_t<T> const& prefactors, cube_t<T> const& pair_weights,
                                         cube_t<T> const& target, auto num_shells,
                                         auto num_species) {
    sro.setConstant(T(0.0));
    T objective{0.0};
    for (auto s = 0; s < num_shells; ++s)
      for (auto xi = 0; xi < num_species; ++xi)
        for (auto eta = xi; eta < num_species; ++eta) {
          auto pair_bonds = bonds(s, xi, eta) + bonds(s, eta, xi);
          T sigma_s_xi_eta = T(1.0) - static_cast<T>(pair_bonds) * prefactors(s, xi, eta);
          sro(s, xi, eta) = sigma_s_xi_eta;
          sro(s, eta, xi) = sigma_s_xi_eta;
          objective += pair_weights(s, xi, eta) * core::helpers::absolute(sigma_s_xi_eta - target(s, xi, eta));
        }
    return objective;
  }

}  // namespace sqsgen::optimization::helpers

#endif  // SQSGEN_OPTIMIZATION_HELPERS_H
