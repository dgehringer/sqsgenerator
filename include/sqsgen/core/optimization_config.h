//
// Created by Dominik Gehringer on 03.03.25.
//

#ifndef OPTIMIZATION_CONFIG_H
#define OPTIMIZATION_CONFIG_H

#include "sqsgen/core/optimization.h"
#include "sqsgen/core/shuffle.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::core {

  namespace detail {
    using namespace core::helpers;
    namespace ranges = std::ranges;
    namespace views = ranges::views;

    template <ranges::sized_range... R> bool same_length(R&&... r) {
      if constexpr (sizeof...(R) == 0)
        return false;
      else if constexpr (sizeof...(R) == 1)
        return true;
      else {
        std::array<size_t, sizeof...(R)> lengths{ranges::size(r)...};
        auto first_length = std::get<0>(lengths);
        return ranges::all_of(lengths, [first_length](auto l) { return l == first_length; });
      }
    }
  }  // namespace detail

  template <class T> struct optimization_config_data {
    std::vector<sublattice> sublattice;
    core::structure<T> structure;
    core::structure<T> sorted;
    std::vector<bounds_t<usize_t>> bounds;
    std::vector<usize_t> sort_order;
    configuration_t species_packed;
    index_mapping_t<specie_t, specie_t> species_map;
    index_mapping_t<usize_t, usize_t> shell_map;
    std::vector<atom_pair<usize_t>> pairs;
    cube_t<T> pair_weights;
    cube_t<T> prefactors;
    cube_t<T> target_objective;
    std::vector<T> shell_radii;
    shell_weights_t<T> shell_weights;
  };

  template <class T, SublatticeMode Mode> struct optimization_config : optimization_config_data<T> {
    shuffler shuffler;

    static std::vector<optimization_config> from_config(configuration<T> config) {
      auto [structures, sorted, bounds, sort_order] = decompose_sort_and_bounds(config);
      if (!core::detail::same_length(structures, sorted, bounds, sort_order, config.shell_radii,
                                     config.shell_weights, config.prefactors,
                                     config.target_objective, config.pair_weights))
        throw std::invalid_argument("Incompatible configuration");
      auto num_sublattices = sorted.size();

      std::vector<optimization_config> configs;
      configs.reserve(num_sublattices);
      for (auto i = 0u; i < num_sublattices; i++) {
        auto [species_packed, species_map, species_rmap, pairs, shells_map, shells_rmap,
              pair_weights]
            = shared(sorted[i], config.shell_radii[i], config.shell_weights[i],
                     config.pair_weights[i]);
        std::vector<sublattice> sublattices;
        if constexpr (Mode == SUBLATTICE_MODE_INTERACT)
          sublattices = config.composition;
        else if constexpr (Mode == SUBLATTICE_MODE_SPLIT)
          sublattices = {config.composition[i]};

        configs.emplace_back(
            optimization_config{sublattices,
                                std::move(structures[i]),
                                std::move(sorted[i]),
                                {bounds[i]},
                                std::move(sort_order[i]),
                                std::move(species_packed),
                                std::make_pair(std::move(species_map), std::move(species_rmap)),
                                std::make_pair(std::move(shells_map), std::move(shells_rmap)),
                                std::move(pairs),
                                std::move(config.pair_weights[i]),
                                std::move(config.prefactors[i]),
                                std::move(config.target_objective[i]),
                                std::move(config.shell_radii[i]),
                                std::move(config.shell_weights[i]),
                                core::shuffler({bounds[i]})});
      }
      return configs;
    }

    optimization_config_data<T> data() const { return *this; }

  private:
    static auto decompose_sort_and_bounds(configuration<T> const& config) {
      if constexpr (Mode == SUBLATTICE_MODE_INTERACT) {
        auto s = config.structure.structure().apply_composition(config.composition);

        auto [sorted, bounds, sort_order]
            = optimization::compute_shuffling_bounds(s, config.composition);

        return std::make_tuple(std::vector{s}, std::vector{sorted},
                               stl_matrix_t<bounds_t<usize_t>>{bounds},
                               stl_matrix_t<usize_t>{sort_order});
      } else if constexpr (Mode == SUBLATTICE_MODE_SPLIT) {
        auto structures
            = config.structure.structure().apply_composition_and_decompose(config.composition);
        return std::make_tuple(
            structures, structures,
            helpers::as<std::vector>{}(
                structures
                | views::transform([](auto&& s) -> bounds_t<usize_t> { return {0, s.size()}; })),
            helpers::as<std::vector>{}(
                structures | views::transform([](auto&& s) -> std::vector<usize_t> {
                  return helpers::as<std::vector>{}(helpers::range(static_cast<usize_t>(s.size())));
                })));
      }
    }
    static auto shared(structure<T> sorted, std::vector<T> const& radii,
                       shell_weights_t<T> const& weights, cube_t<T> const& pair_weights) {
      auto [species_map, species_rmap] = helpers::make_index_mapping<specie_t>(sorted.species);
      auto species_packed
          = helpers::as<std::vector>{}(sorted.species | views::transform([&species_map](auto&& s) {
                                         return species_map.at(s);
                                       }));
      auto [pairs, shells_map, shells_rmap] = sorted.pairs(radii, weights);
      std::sort(pairs.begin(), pairs.end(), [](auto p, auto q) {
        return helpers::absolute(p.i - p.j) < helpers::absolute(q.i - q.j) && p.i < q.i;
      });
      return std::make_tuple(
          species_packed, species_map, species_rmap, pairs, shells_map, shells_rmap,
          optimization::scaled_pair_weights(pair_weights, weights, sorted.num_species));
    }
  };

}  // namespace sqsgen::core

#endif  // OPTIMIZATION_CONFIG_H
