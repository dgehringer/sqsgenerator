//
// Created by Dominik Gehringer on 22.10.24.
//

#ifndef SQSGEN_IMPL_CORE_STRUCTURE_STRUCTURE_IMPL_HPP
#define SQSGEN_IMPL_CORE_STRUCTURE_STRUCTURE_IMPL_HPP

#include <Eigen/Dense>
#include <array>
#include <cstddef>
#include <format>
#include <limits>
#include <map>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

#include "sqsgen/core/atom.h"
#include "sqsgen/core/eigen.h"
#include "sqsgen/core/helpers/expected.h"
#include "sqsgen/core/helpers/hash.h"
#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/core/permutation.h"
#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/core/structure_utils.h"
#include "sqsgen/types.h"

namespace sqsgen::core {

  namespace detail {

    template <class T> [[nodiscard]] sqsgen::core::atom site<T>::atom() const {
      return atom::from_z(specie);
    }

    template <class T> bool site<T>::operator<(const site& other) const {
      return specie < other.specie && frac_coords(0) < other.frac_coords(0)
             && frac_coords(1) < other.frac_coords(1) && frac_coords(2) < other.frac_coords(2);
    }
    template <class T> bool site<T>::operator==(const site& other) const {
      return specie == other.specie && frac_coords == other.frac_coords;
    }

    template <class T> std::size_t site<T>::hasher::operator()(const site& s) const {
      std::size_t res = 0;
      helpers::hash_combine(res, s.specie);
      helpers::hash_combine(res, s.frac_coords(0));
      helpers::hash_combine(res, s.frac_coords(1));
      helpers::hash_combine(res, s.frac_coords(2));
      return res;
    }

    inline std::size_t compute_num_species(const configuration_t& configuration) {
      return static_cast<std::size_t>(sorted_vector<specie_t>(configuration).size());
    }

  }  // namespace detail

  template <class T>
  structure<T>::structure(const lattice_t<T>& lattice, const coords_t<T>& frac_coords,
                          const configuration_t& species, const std::array<bool, 3>& pbc)
      : lattice(lattice),
        frac_coords(frac_coords),
        species(species),
        pbc(pbc),
        num_species(detail::compute_num_species(species)) {
    if (std::cmp_not_equal(frac_coords.rows(), species.size()) || species.empty())
      throw std::invalid_argument(
          "frac coords must have the same size as the species input and must not be empty");
  }

  template <class T> structure<T>::structure(lattice_t<T>&& lattice, coords_t<T>&& frac_coords,
                                             configuration_t&& species, std::array<bool, 3>&& pbc)
      : lattice(lattice),
        frac_coords(frac_coords),
        species(species),
        pbc(pbc),
        num_species(detail::compute_num_species(species)) {
    if (std::cmp_not_equal(frac_coords.rows(), species.size()) || species.empty())
      throw std::invalid_argument(
          "frac coords must have the same size as the species input and must not be empty");
  }

  template <class T> matrix_t<T> structure<T>::distance_matrix() const {
    return core::distance_matrix<T>(lattice, frac_coords);
  }

  template <class T>
  shell_matrix_t structure<T>::shell_matrix(const std::vector<T>& shell_radii, T atol, T rtol) {
    return core::shell_matrix(distance_matrix(), shell_radii, atol, rtol);
  }

  template <class T>
  structure<T> structure<T>::supercell(std::size_t a, std::size_t b, std::size_t c) const {
    // the implementation lives as a free function in sqsgen/core/structure_utils.h
    return core::supercell(*this, a, b, c);
  }

  template <class T>
  structure<T> structure<T>::apply_composition(const std::vector<sublattice>& composition) const {
    auto copy = structure(*this);
    for (const auto& [sites, species] : composition) {
      auto index = sites.begin();
      for (const auto& [specie, amount] : species)
        for (std::size_t _ = 0; _ < amount; _++, ++index) copy.species[*index] = specie;
    }
    copy.num_species = sqsgen::core::detail::compute_num_species(copy.species);
    return copy;
  }

  template <class T> structure<T> structure<T>::with_species(const configuration_t& conf) const {
    if (conf.size() != size()) throw std::invalid_argument("Species size mismatch");
    return structure{lattice, frac_coords, conf, pbc};
  }

  template <class T> std::vector<structure<T>> structure<T>::apply_composition_and_decompose(
      const std::vector<sublattice>& composition) const {
    auto with_species = apply_composition(composition);
    std::vector<structure<T>> out;
    out.reserve(composition.size());
    for (const auto& sublattice : composition) out.push_back(with_species.sliced(sublattice.sites));
    return out;
  }

  template <class T> structure<T> structure<T>::without_vacancies() const {
    return filtered([](auto site) { return site.specie != 0; });
  }

  template <class T> auto structure<T>::pairs(const std::vector<T>& radii,
                                              const std::map<std::size_t, T>& weights, bool pack,
                                              T atol, T rtol) {
    using namespace helpers;
    auto [shell_map, reverse_map] = make_index_mapping<std::size_t>(weights | views::elements<0>);
    std::vector<atom_pair> pairs;
    pairs.reserve(size() * size() / 2);
    auto sm = shell_matrix(radii, atol, rtol);
    for (std::size_t i = 0; i < size(); ++i) {
      for (std::size_t j = i + 1; j < size(); ++j) {
        if (!weights.contains(sm(i, j))) continue;
        pairs.push_back({i, j, static_cast<std::size_t>(pack ? shell_map[sm(i, j)] : sm(i, j))});
      }
    }
    pairs.shrink_to_fit();
    return std::make_tuple(pairs, shell_map, reverse_map);
  }

  template <class T> configuration_t structure<T>::packed_species() const {
    using namespace helpers;
    auto map = reversed_index_map<specie_t>(species);
    return species | views::transform([&](auto z) { return map[z]; }) | ranges::to<std::vector>();
  }

  template <class T> rank_t structure<T>::rank() const {
    return rank_permutation(packed_species());
  }

  template <class T> template <array_mode AMode>
  structure_dto<T, AMode> structure<T>::to_dto() const {
    return structure_dto<T, AMode>{
        .species = species,
        .lattice = tensor_as<AMode>(lattice),
        .coords = tensor_as<AMode>(frac_coords),
        .num_atoms = size(),
    };
  };

  inline io::parser_error forward_error(core::tensor_shape_error err) {
    return {map_error_kind(err.which), err.message};
  }

  template <class T> template <array_mode AMode>
  io::result_t<structure<T>> structure<T>::from_dto(const structure_dto<T, AMode>& data) {
    auto lattice_and_coords
        = helpers::combine(core::lattice_from<T>(data.lattice), core::coords_from<T>(data.coords));
    if (!lattice_and_coords)
      return std::unexpected(forward_error(std::move(lattice_and_coords).error()));
    const auto [lattice, coords] = std::move(lattice_and_coords).value();
    if (std::cmp_not_equal(coords.rows(), data.num_atoms))
      return std::unexpected(io::parser_error{
          error_code::invalid_size,
          std::format("Found {} coords but counted {} sites", coords.rows(), data.num_atoms)});
    if (data.species.size() != data.num_atoms)
      return std::unexpected(io::parser_error{error_code::invalid_size,
                                              std::format("Found {} coords but counted {} sites",
                                                          data.species.size(), data.num_atoms)});
    for (const auto ordinal : data.species) {
      if (ordinal > core::KNOWN_ELEMENTS.size() || ordinal < 0)
        return std::unexpected(io::parser_error{
            error_code::invalid_size,
            std::format("Unknown atomic element {}", static_cast<unsigned>(ordinal))});
    }

    return core::structure<T>(lattice, coords, data.species);
  };

}  // namespace sqsgen::core

#endif  // SQSGEN_IMPL_CORE_STRUCTURE_STRUCTURE_IMPL_HPP
