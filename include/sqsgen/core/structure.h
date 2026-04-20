//
// Created by Dominik Gehringer on 22.10.24.
//

#ifndef SQSGEN_CORE_STRUCTURE_H
#define SQSGEN_CORE_STRUCTURE_H

#include <Eigen/Dense>

#include "sqsgen/core/atom.h"
#include "sqsgen/core/helpers/hash.h"
#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/log.h"
#include "sqsgen/types.h"

namespace sqsgen::core {

  namespace ranges = std::ranges;
  namespace views = ranges::views;

  template <class Size>
    requires std::is_integral_v<Size>
  struct atom_pair {
    Size i;
    Size j;
    Size shell;
  };

  template <class T> class structure;

  template <class T> class site {
  public:
    friend class structure<T>;
    using row_t = Eigen::Vector3<T>;
    usize_t index;
    specie_t specie;
    row_t frac_coords;
    [[nodiscard]] sqsgen::core::atom atom() const { return atom::from_z(specie); }

    bool operator<(site const &other) const {
      return specie < other.specie && frac_coords(0) < other.frac_coords(0)
             && frac_coords(1) < other.frac_coords(1) && frac_coords(2) < other.frac_coords(2);
    }

    bool operator==(const site &other) const {
      return specie == other.specie && frac_coords == other.frac_coords;
    }

    struct hasher {
      std::size_t operator()(site const &s) const {
        std::size_t res = 0;
        helpers::hash_combine(res, s.specie);
        helpers::hash_combine(res, s.frac_coords(0));
        helpers::hash_combine(res, s.frac_coords(1));
        helpers::hash_combine(res, s.frac_coords(2));
        return res;
      }
    };
  };

  template <class T> class structure {
  private:
    std::optional<matrix_t<T>> _distance_matrix = std::nullopt;

  public:
    lattice_t<T> lattice;
    coords_t<T> frac_coords;
    configuration_t species;
    std::array<bool, 3> pbc = {true, true, true};
    usize_t num_species;

    structure() = default;

    template <ranges::input_range R>
      requires std::is_same_v<ranges::range_value_t<R>, site<T>>
    structure(const lattice_t<T> &lattice, R &&r) : lattice(lattice) {
      std::vector<site<T>> sites(r.begin(), r.end());
      if (sites.empty()) throw std::invalid_argument("Cannot create a structure without atoms");
      coords_t<T> fc(sites.size(), 3);
      species.resize(sites.size());
      for (auto index = 0; index < sites.size(); ++index) {
        species[index] = sites[index].specie;
        fc.row(index) = sites[index].frac_coords;
      };
      frac_coords = fc;
      _distance_matrix = std::nullopt;
      num_species = helpers::count(species).size();
    }

    structure(const lattice_t<T> &lattice, const coords_t<T> &frac_coords,
              configuration_t const &species, const std::array<bool, 3> &pbc = {true, true, true})
        : lattice(lattice),
          frac_coords(frac_coords),
          species(species),
          pbc(pbc),
          num_species(helpers::count(species).size()) {
      if (frac_coords.rows() != species.size() || species.empty())
        throw std::invalid_argument(
            "frac coords must have the same size as the species input and must not be empty");
    };

    structure(lattice_t<T> &&lattice, coords_t<T> &&frac_coords, configuration_t &&species,
              std::array<bool, 3> &&pbc = {true, true, true})
        : lattice(lattice),
          frac_coords(frac_coords),
          species(species),
          pbc(pbc),
          num_species(helpers::count(species).size()) {
      if (frac_coords.rows() != species.size() || species.empty())
        throw std::invalid_argument(
            "frac coords must have the same size as the species input and must not be empty");
    };

    [[nodiscard]] const matrix_t<T> &distance_matrix();

    [[nodiscard]] shell_matrix_t shell_matrix(std::vector<T> const &shell_radii,
                                              T atol = std::numeric_limits<T>::epsilon(),
                                              T rtol = 1.0e-9);

    std::vector<T> distances_naive(T atol = std::numeric_limits<T>::epsilon(), T rtol = 1e-9);

    std::vector<T> distances_histogram(T bin_width, T peak_isolation);

    [[nodiscard]] structure supercell(std::size_t a, std::size_t b, std::size_t c) const;

    [[nodiscard]] std::size_t size() const;

    std::vector<site<T>> sites() const;

    template <class Fn>
    std::tuple<std::vector<usize_t>, structure> sorted_with_indices(Fn &&fn) const {
      auto s = sites();
      std::sort(s.begin(), s.end(), std::forward<Fn>(fn));
      std::vector<usize_t> indices;
      std::transform(s.begin(), s.end(), std::back_inserter(indices),
                     [](auto site) { return site.index; });
      return std::make_tuple(structure(lattice, s), indices);
    }

    template <class Fn> structure sorted(Fn &&fn) const {
      return std::get<0>(sorted_with_indices(std::forward<Fn>(fn)));
    }

    structure apply_composition(std::vector<sublattice> const &composition);

    structure with_species(configuration_t const &conf) const;

    std::vector<structure> apply_composition_and_decompose(
        std::vector<sublattice> const &composition);

    template <class Fn> auto filtered(Fn &&fn) const {
      return structure(lattice, sites() | views::filter(std::forward<Fn>(fn)));
    }

    structure without_vacancies() const;

    template <ranges::input_range R, class V = ranges::range_value_t<R>>
      requires std::is_integral_v<V>
    structure sliced(R &&r) const {
      auto sites = std::vector<site<T>>{};
      for (auto index : r) {
        if (index >= size() || index < 0)
          throw std::out_of_range(format_string("index out of range 0 <= %i < %i", index, size()));
        sites.push_back(site<T>{static_cast<usize_t>(index), species[index],
                                Eigen::Vector3<T>(frac_coords.row(index))});
      }
      return structure(lattice, sites);
    }

    auto pairs(std::vector<T> const &radii, shell_weights_t<T> const &weights, bool pack = true,
               T atol = std::numeric_limits<T>::epsilon(), T rtol = 1.0e-9);

    [[nodiscard]] configuration_t packed_species() const;

    [[nodiscard]] rank_t rank() const;

    [[nodiscard]] std::string uuid() const;
  };

  template <typename T> using site_t = site<T>;

}  // namespace sqsgen::core

#endif  // SQSGEN_CORE_STRUCTURE_H
