//
// Created by Dominik Gehringer on 22.10.24.
//

#ifndef SQSGEN_CORE_STRUCTURE_H
#define SQSGEN_CORE_STRUCTURE_H

#include <Eigen/Dense>
#include <cstddef>
#include <format>
#include <ranges>

#include "sqsgen/core/atom.h"
#include "sqsgen/core/helpers/as.h"
#include "sqsgen/core/structure_utils.h"
#include "sqsgen/types.h"

namespace sqsgen::core {

  namespace ranges = std::ranges;
  namespace views = ranges::views;

  struct atom_pair {
    std::size_t i;
    std::size_t j;
    std::size_t shell;
  };

  template <class T> class structure;

  namespace detail {

    template <class T> class site {
    public:
      friend class structure<T>;
      using row_t = Eigen::Vector3<T>;
      std::size_t index;
      specie_t specie;
      row_t frac_coords;
      [[nodiscard]] sqsgen::core::atom atom() const;

      bool operator<(const site& other) const;

      bool operator==(const site& other) const;
      struct hasher {
        std::size_t operator()(const site& s) const;
      };
    };

    std::size_t compute_num_species(const configuration_t& configuration);

  }  // namespace detail

  template <class T, array_mode AMode> struct structure_dto {
    configuration_t species;
    core::tensor_representation_t<T, 2, AMode> lattice;
    core::tensor_representation_t<T, 2, AMode> coords;
    std::size_t num_atoms;
  };

  template <class T> class structure {
  public:
    lattice_t<T> lattice;
    coords_t<T> frac_coords;
    configuration_t species;
    std::array<bool, 3> pbc = {true, true, true};
    std::size_t num_species;

    structure() = default;

    template <ranges::input_range R>
      requires std::is_same_v<ranges::range_value_t<R>, sqsgen::core::detail::site<T>>
    structure(const lattice_t<T>& lattice, R&& r) : lattice(lattice) {
      auto sites = r | ranges::to<std::vector>();
      if (sites.empty()) throw std::invalid_argument("Cannot create a structure without atoms");
      coords_t<T> fc(sites.size(), 3);
      species.resize(sites.size());
      for (std::size_t index = 0; index < sites.size(); ++index) {
        species[index] = sites[index].specie;
        fc.row(index) = sites[index].frac_coords;
      };
      frac_coords = fc;
      num_species = detail::compute_num_species(species);
    }

    structure(const lattice_t<T>& lattice, const coords_t<T>& frac_coords,
              const configuration_t& species, const std::array<bool, 3>& pbc = {true, true, true});

    structure(lattice_t<T>&& lattice, coords_t<T>&& frac_coords, configuration_t&& species,
              std::array<bool, 3>&& pbc = {true, true, true});

    [[nodiscard]] matrix_t<T> distance_matrix() const;

    [[nodiscard]] shell_matrix_t shell_matrix(const std::vector<T>& shell_radii,
                                              T atol = std::numeric_limits<T>::epsilon(),
                                              T rtol = 1.0e-9);

    [[nodiscard]] structure supercell(std::size_t a, std::size_t b, std::size_t c) const;

    [[nodiscard]] std::size_t size() const { return species.size(); }

    template <array_mode AMode> structure_dto<T, AMode> to_dto() const;

    template <array_mode AMode>
    static io::result_t<structure<T>> from_dto(const structure_dto<T, AMode>& input);

    auto sites() const {
      return ranges::iota_view(static_cast<std::size_t>(0), static_cast<std::size_t>(size()))
             | views::transform([&](auto i) {
                 return sqsgen::core::detail::site<T>{i, species[i],
                                                      Eigen::Vector3<T>(frac_coords.row(i))};
               });
    }

    template <class Fn>
    std::tuple<structure<T>, std::vector<std::size_t>> sorted_with_indices(Fn&& fn) const {
      auto s = sites() | ranges::to<std::vector>();
      std::sort(s.begin(), s.end(), std::forward<Fn>(fn));
      return std::make_tuple(structure(lattice, s), s | views::transform([](auto site) {
                                                      return site.index;
                                                    }) | ranges::to<std::vector>());
    }

    template <class Fn> auto sorted(Fn&& fn) const {
      return std::get<0>(sorted_with_indices(std::forward<Fn>(fn)));
    }

    structure apply_composition(const std::vector<sublattice>& composition) const;

    structure with_species(const configuration_t& conf) const;

    std::vector<structure> apply_composition_and_decompose(
        const std::vector<sublattice>& composition) const;

    template <class Fn> auto filtered(Fn&& fn) const {
      return structure(lattice, sites() | views::filter(std::forward<Fn>(fn)));
    }

    structure without_vacancies() const;

    template <ranges::input_range R, class V = ranges::range_value_t<R>>
      requires std::is_integral_v<V>
    structure sliced(R&& r) const {
      auto sites = std::vector<sqsgen::core::detail::site<T>>{};
      for (auto index : r) {
        if (index >= size() || index < 0)
          throw std::out_of_range(std::format("index out of range 0 <= {} < {}", index, size()));
        sites.push_back(sqsgen::core::detail::site<T>{static_cast<std::size_t>(index),
                                                      species[index],
                                                      Eigen::Vector3<T>(frac_coords.row(index))});
      }
      return structure(lattice, sites);
    }

    auto pairs(const std::vector<T>& radii, const std::map<std::size_t, T>& weights,
               bool pack = true, T atol = std::numeric_limits<T>::epsilon(), T rtol = 1.0e-9);

    [[nodiscard]] configuration_t packed_species() const;

    [[nodiscard]] rank_t rank() const;
  };

  template <typename T> using site_t = sqsgen::core::detail::site<T>;

}  // namespace sqsgen::core

#include "sqsgen/impl/core/structure/structure_impl.hpp"

#endif  // SQSGEN_CORE_STRUCTURE_H
