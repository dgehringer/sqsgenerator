//
// Created by Dominik Gehringer on 22.10.24.
//

#ifndef SQSGEN_CORE_STRUCTURE_H
#define SQSGEN_CORE_STRUCTURE_H

#include <Eigen/Dense>
#include <cstddef>

#include "sqsgen/core/atom.h"
#include "sqsgen/core/helpers/as.h"
#include "sqsgen/types.h"

namespace sqsgen::core {

  namespace ranges = std::ranges;
  namespace views = ranges::views;

  template <class T>
  matrix_t<T> distance_matrix(const lattice_t<T>& lattice, const coords_t<T>& frac_coords);

  template <class T> shell_matrix_t shell_matrix(const matrix_t<T>& distance_matrix,
                                                 const std::vector<T>& dists, T atol, T rtol);

  template <class T> cube_t<T> compute_prefactors(const shell_matrix_t& shell_matrix,
                                                  const shell_weights_t<T>& weights,
                                                  const configuration_t& configuration);

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

      bool operator<(site const& other) const;

      bool operator==(const site& other) const;
      struct hasher {
        std::size_t operator()(site const& s) const;
      };
    };

    inline std::size_t compute_num_species(const configuration_t& configuration);

  }  // namespace detail

  template <class T> class structure {
  private:
    std::optional<matrix_t<T>> _distance_matrix = std::nullopt;

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
      auto sites = helpers::as<std::vector>{}(r);
      if (sites.empty()) throw std::invalid_argument("Cannot create a structure without atoms");
      coords_t<T> fc(sites.size(), 3);
      species.resize(sites.size());
      for (auto index = 0; index < sites.size(); ++index) {
        species[index] = sites[index].specie;
        fc.row(index) = sites[index].frac_coords;
      };
      frac_coords = fc;
      _distance_matrix = std::nullopt;
      num_species = detail::compute_num_species(species);
    }

    structure(const lattice_t<T>& lattice, const coords_t<T>& frac_coords,
              configuration_t const& species, const std::array<bool, 3>& pbc = {true, true, true});

    structure(lattice_t<T>&& lattice, coords_t<T>&& frac_coords, configuration_t&& species,
              std::array<bool, 3>&& pbc = {true, true, true});

    [[nodiscard]] const matrix_t<T>& distance_matrix();

    [[nodiscard]] shell_matrix_t shell_matrix(std::vector<T> const& shell_radii,
                                              T atol = std::numeric_limits<T>::epsilon(),
                                              T rtol = 1.0e-9);

    [[nodiscard]] structure supercell(std::size_t a, std::size_t b, std::size_t c) const;

    [[nodiscard]] std::size_t size() const { return species.size(); }

    auto sites() const {
      return ranges::iota_view(static_cast<std::size_t>(0), static_cast<std::size_t>(size()))
             | views::transform([&](auto i) {
                 return sqsgen::core::detail::site<T>{i, species[i],
                                                      Eigen::Vector3<T>(frac_coords.row(i))};
               });
    }

    template <class Fn>
    std::tuple<structure<T>, std::vector<std::size_t>> sorted_with_indices(Fn&& fn) const {
      auto s = helpers::as<std::vector>{}(sites());
      std::sort(s.begin(), s.end(), std::forward<Fn>(fn));
      return std::make_tuple(
          structure(lattice, s),
          helpers::as<std::vector>{}(s | views::transform([](auto site) { return site.index; })));
    }

    template <class Fn> auto sorted(Fn&& fn) const {
      return std::get<0>(sorted_with_indices(std::forward<Fn>(fn)));
    }

    structure apply_composition(std::vector<sublattice> const& composition) const;

    structure with_species(configuration_t const& conf) const;

    std::vector<structure> apply_composition_and_decompose(
        std::vector<sublattice> const& composition) const;

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
          throw std::out_of_range(format_string("index out of range 0 <= %i < %i", index, size()));
        sites.push_back(sqsgen::core::detail::site<T>{static_cast<std::size_t>(index),
                                                      species[index],
                                                      Eigen::Vector3<T>(frac_coords.row(index))});
      }
      return structure(lattice, sites);
    }

    auto pairs(std::vector<T> const& radii, shell_weights_t<T> const& weights, bool pack = true,
               T atol = std::numeric_limits<T>::epsilon(), T rtol = 1.0e-9);

    [[nodiscard]] configuration_t packed_species() const;

    [[nodiscard]] rank_t rank() const;
  };

  template <class T> std::vector<T> distances_naive(const structure<T>& structure,
                                                    T atol = std::numeric_limits<T>::epsilon(),
                                                    T rtol = 1e-9);

  template <class T>
  std::vector<T> distances_histogram(const structure<T>& structure, T bin_width, T peak_isolation);
  template <class T> cube_t<T> compute_prefactors(const shell_matrix_t& shell_matrix,
                                                  const shell_weights_t<T>& weights,
                                                  const configuration_t& configuration);
  template <class T> cube_t<T> compute_prefactors(const structure<T>& structure,
                                                  const std::vector<T>& shell_radii,
                                                  const shell_weights_t<T>& weights);

  template <typename T> using site_t = sqsgen::core::detail::site<T>;

}  // namespace sqsgen::core

#endif  // SQSGEN_CORE_STRUCTURE_H
