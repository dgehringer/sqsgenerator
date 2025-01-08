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
      if (sublattice_indices[i] != sublattice_indices[lower_index]
          || sublattice_indices[i] == composition.size()) {
        bounds.emplace_back(lower_index, i);
        lower_index = i;
      } else if (i == num_atoms - 1)
        bounds.emplace_back(lower_index, i + 1);
    }
    assert(bounds.size() == composition.size());
    return std::make_tuple(sorted, bounds, sort_order);
  }

}  // namespace sqsgen::optimization::helpers

#endif  // SQSGEN_OPTIMIZATION_HELPERS_H
