//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_IMPL_OPTIMIZATION_MAKE_KERNEL_DATA_IMPL_HPP
#define SQSGEN_IMPL_OPTIMIZATION_MAKE_KERNEL_DATA_IMPL_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <ranges>
#include <variant>
#include <vector>

#include "sqsgen/configuration.h"
#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/core/helpers/variant.h"
#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/core/structure_utils.h"
#include "sqsgen/optimization/kernel_data.h"
#include "sqsgen/optimization/permutation.h"
#include "sqsgen/optimization/prefactors.h"
#include "sqsgen/types.h"

namespace sqsgen::optimization {

  namespace core = sqsgen::core;
  namespace configuration = sqsgen::configuration;
  namespace helpers = sqsgen::core::helpers;

  namespace detail {

    // packed index -> atomic number: the sorted, deduplicated species of the lattice. This is the
    // order the pair/triplet tensors are indexed in (compute_*_prefactors uses the same index_map).
    inline configuration_t unique_species(const configuration_t& species) {
      core::sorted_vector<specie_t> dedup(species);
      configuration_t out(dedup.begin(), dedup.end());
      return out;
    }

    template <class Map> typename Map::mapped_type sum_values(const Map& map) {
      typename Map::mapped_type total{};
      for (const auto& [key, value] : map) total += value;
      return total;
    }

    // pick one sublattice's slice of a per-mode nested object (interact: the whole object). The
    // nesting alias is a non-deduced context, so deduce the argument type directly instead.
    template <sublattice_mode Mode, class Nested>
    decltype(auto) select(const Nested& value, std::size_t index) {
      if constexpr (Mode == sublattice_mode::interact)
        return (value);
      else
        return (value[index]);
    }

    inline std::uint64_t resolve_seed(const configuration::seed_t& seed, std::size_t index) {
      return seed[index].value_or(make_random_seed());
    }

    template <class T> pair_data<T> make_sro_pair_data(const core::structure<T>& structure,
                                                       const std::vector<T>& radii,
                                                       const std::map<shell_t, T>& weights,
                                                       const pair_sro_t<T>& targets,
                                                       const configuration_t& species, T atol,
                                                       T rtol, T scale) {
      pair_data<T> data;
      data.pairs = core::make_pair_list(structure, radii, weights, atol, rtol);
      // compact shell index -> user shell number: the selected shells in ascending order
      data.shells = weights | std::views::keys | std::ranges::to<std::vector<shell_t>>();
      data.prefactors = compute_pair_prefactors(structure, radii, weights, atol, rtol);

      const auto num_shells = static_cast<Eigen::Index>(data.shells.size());
      const auto num_species = static_cast<Eigen::Index>(species.size());
      pair_tensor_t<T> weight_tensor(num_shells, num_species, num_species);
      for (Eigen::Index s = 0; s < num_shells; ++s)
        for (Eigen::Index xi = 0; xi < num_species; ++xi)
          for (Eigen::Index eta = 0; eta < num_species; ++eta)
            weight_tensor(s, xi, eta) = weights.at(data.shells[s]);

      const pair_tensor_t<T> target = to_pair_tensor<T>(targets, data.shells, species, T{0});
      data.terms = sro_terms(data.prefactors, weight_tensor, target, scale);
      return data;
    }

    template <class T> triplet_data<T> make_sro_triplet_data(
        const core::structure<T>& structure, const std::vector<T>& radii,
        const std::map<core::triplet_class_mapping_t::value_type, T>& weights,
        const triplet_sro_t<T>& targets, const configuration_t& species, T atol, T rtol, T scale) {
      // select the shells that carry a weighted triangle; a triangle is kept only if all three of
      // its edges lie in a selected shell (see make_triplet_list)
      std::map<shell_t, T> shell_selection;
      for (const auto& [triple, weight] : weights)
        for (const shell_t s : triple) shell_selection.emplace(s, T{1});

      auto [triplets, classes]
          = core::make_triplet_list(structure, radii, shell_selection, atol, rtol);
      triplet_data<T> data;
      data.triplets = std::move(triplets);
      data.classes = std::move(classes);
      data.prefactors
          = compute_triplet_prefactors<T>(data.triplets, data.classes, structure.species);

      const auto num_classes = static_cast<Eigen::Index>(data.classes.size());
      const auto num_species = static_cast<Eigen::Index>(species.size());
      triplet_tensor_t<T> weight_tensor(num_classes, num_species, num_species, num_species);
      for (Eigen::Index c = 0; c < num_classes; ++c) {
        const auto it = weights.find(data.classes[c]);
        const T weight = it == weights.end() ? T{0} : it->second;  // unweighted class -> 0
        for (Eigen::Index xi = 0; xi < num_species; ++xi)
          for (Eigen::Index eta = 0; eta < num_species; ++eta)
            for (Eigen::Index zeta = 0; zeta < num_species; ++zeta)
              weight_tensor(c, xi, eta, zeta) = weight;
      }
      const triplet_tensor_t<T> target = to_triplet_tensor<T>(targets, data.classes, species, T{0});
      data.terms = sro_terms(data.prefactors, weight_tensor, target, scale);
      return data;
    }

    template <class T> pair_data<T> make_energy_pair_data(const core::structure<T>& structure,
                                                          const std::vector<T>& radii,
                                                          const pair_sro_t<T>& interactions,
                                                          const configuration_t& species, T atol,
                                                          T rtol) {
      std::map<shell_t, T> shell_selection;  // every shell carrying an interaction
      for (const auto& [key, value] : interactions) shell_selection.emplace(key.shell, T{1});

      pair_data<T> data;
      data.pairs = core::make_pair_list(structure, radii, shell_selection, atol, rtol);
      data.shells = shell_selection | std::views::keys | std::ranges::to<std::vector<shell_t>>();
      // f is still needed to report the SRO parameter alpha = 1 - N f of a found configuration
      data.prefactors = compute_pair_prefactors(structure, radii, shell_selection, atol, rtol);
      const pair_tensor_t<T> J = to_pair_tensor<T>(interactions, data.shells, species, T{0});
      data.terms = energy_terms(J);
      return data;
    }

    template <class T>
    triplet_data<T> make_energy_triplet_data(const core::structure<T>& structure,
                                             const std::vector<T>& radii,
                                             const triplet_sro_t<T>& interactions,
                                             const configuration_t& species, T atol, T rtol) {
      std::map<shell_t, T> shell_selection;
      for (const auto& [key, value] : interactions)
        for (const shell_t s : key.shells) shell_selection.emplace(s, T{1});

      auto [triplets, classes]
          = core::make_triplet_list(structure, radii, shell_selection, atol, rtol);
      triplet_data<T> data;
      data.triplets = std::move(triplets);
      data.classes = std::move(classes);
      data.prefactors
          = compute_triplet_prefactors<T>(data.triplets, data.classes, structure.species);
      const triplet_tensor_t<T> J = to_triplet_tensor<T>(interactions, data.classes, species, T{0});
      data.terms = energy_terms(J);
      return data;
    }

    // Build everything of one lattice that depends on the objective (pairs, triplets). The site
    // layout, seeds and initial configuration are filled by the caller.
    template <class T, sublattice_mode Mode>
    lattice_data<T> build_lattice(const core::structure<T>& structure, const std::vector<T>& radii,
                                  const configuration::objective_t<T, Mode>& objective, bool pairs,
                                  bool triplets, T atol, T rtol, std::size_t index = 0) {
      lattice_data<T> lattice;
      lattice.species_packed = structure.packed_species();
      lattice.species = unique_species(structure.species);

      std::visit(
          helpers::overloaded{
              [&](const configuration::sro_objective<T, Mode>& sro) {
                if (pairs && sro.pairs.has_value()) {
                  const auto& weights = select<Mode>(sro.pairs->weights, index);
                  const auto& targets = select<Mode>(sro.pairs->targets, index);
                  const T scale = (T{1} - sro.lambda) / sum_values(weights);  // (1 - lambda) / W
                  lattice.pairs = make_sro_pair_data<T>(structure, radii, weights, targets,
                                                        lattice.species, atol, rtol, scale);
                }
                if (triplets && sro.triplets.has_value()) {
                  const auto& weights = select<Mode>(sro.triplets->weights, index);
                  const auto& targets = select<Mode>(sro.triplets->targets, index);
                  const T scale = sro.lambda / sum_values(weights);  // lambda / W
                  lattice.triplets = make_sro_triplet_data<T>(structure, radii, weights, targets,
                                                              lattice.species, atol, rtol, scale);
                }
              },
              [&](const configuration::energy_objective<T, Mode>& energy) {
                if (pairs && energy.pairs.has_value())
                  lattice.pairs = make_energy_pair_data<T>(
                      structure, radii, select<Mode>(energy.pairs.value(), index), lattice.species,
                      atol, rtol);
                if (triplets && energy.triplets.has_value())
                  lattice.triplets = make_energy_triplet_data<T>(
                      structure, radii, select<Mode>(energy.triplets.value(), index),
                      lattice.species, atol, rtol);
              }},
          objective);
      return lattice;
    }

  }  // namespace detail

  template <class T, sublattice_mode Mode>
  kernel_data<T> make_kernel_data(const configuration::configuration<T, Mode>& config) {
    const T atol = config.atol;
    const T rtol = config.rtol;

    kernel_data<T> data;
    data.block_size = config.chunk_size;
    data.objective = std::holds_alternative<configuration::sro_objective<T, Mode>>(config.objective)
                         ? objective_kind::sro
                         : objective_kind::energy;

    if constexpr (Mode == sublattice_mode::interact) {
      const core::structure<T> applied
          = config.structure.make_structure().apply_composition(config.composition);
      const site_layout layout = make_site_layout(
          applied.size(),
          config.composition | std::views::transform([](const sublattice& sl) -> const auto& {
            return sl.sites;
          }));
      // reorder the sites so each sublattice is contiguous; the pair/triplet site indices then
      // index straight into the (shuffled) configuration array
      const core::structure<T> ordered = applied.sliced(layout.order);

      lattice_data<T> lattice = detail::build_lattice<T, Mode>(
          ordered, config.shell_radii, config.objective, config.pairs, config.triplets, atol, rtol);
      lattice.order = layout.order;
      lattice.ranges = layout.ranges;
      lattice.seeds.reserve(config.seed.size());
      for (std::size_t r = 0; r < config.seed.size(); ++r)
        lattice.seeds.push_back(detail::resolve_seed(config.seed, r));
      data.lattices.push_back(std::move(lattice));
    } else {
      // one lattice per sublattice; each sublattice is shuffled as a single range
      const std::vector<core::structure<T>> structures
          = config.structure.template for_mode<Mode>(config.composition);
      data.lattices.reserve(structures.size());
      for (std::size_t i = 0; i < structures.size(); ++i) {
        const core::structure<T>& sub = structures[i];
        lattice_data<T> lattice
            = detail::build_lattice<T, Mode>(sub, config.shell_radii[i], config.objective,
                                             config.pairs, config.triplets, atol, rtol, i);

        lattice.order.assign(config.composition[i].sites.begin(),
                             config.composition[i].sites.end());
        lattice.ranges = {site_range{0, sub.size()}};
        lattice.seeds = {detail::resolve_seed(config.seed, i)};
        data.lattices.push_back(std::move(lattice));
      }
    }
    return data;
  }

}  // namespace sqsgen::optimization

#endif  // SQSGEN_IMPL_OPTIMIZATION_MAKE_KERNEL_DATA_IMPL_HPP
