//
// Created by Dominik Gehringer on 22.10.24.
//

#ifndef SQSGEN_CORE_STRUCTURE_H
#define SQSGEN_CORE_STRUCTURE_H

#include "sqsgen/core/atom.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/types.h"

namespace sqsgen::core {

  namespace detail {
    template <class T>
    matrix_t<T> distance_matrix(const lattice_t<T> &lattice, const coords_t<T> &frac_coords) {
      const coords_t<T> cart_coords = frac_coords * lattice;
      assert(frac_coords.cols() == 3);
      const auto num_atoms = frac_coords.rows();

      auto a{lattice.row(0)};
      auto b{lattice.row(1)};
      auto c{lattice.row(2)};

      std::array axis{-1, 0, 1};
      matrix_t<T> distances
          = matrix_t<T>::Ones(num_atoms, num_atoms) * std::numeric_limits<T>::max();
#pragma omp parallel for schedule(static) shared(distances) \
    firstprivate(a, b, c, axis, num_atoms, cart_coords) if (num_atoms > 100)
      for (auto i = 0; i < num_atoms; i++) {
        auto p1{cart_coords.row(i)};
        for (auto j = i; j < num_atoms; j++) {
          auto p2{cart_coords.row(j)};
          helpers::for_each(
              [&](auto u, auto v, auto w) {
                auto t = u * a + v * b + w * c;
                auto diff = p1 - (t + p2);
                T image_norm{std::abs(diff.norm())};
                if (image_norm < distances(i, j)) {
                  distances(i, j) = image_norm;
                  if (i != j) distances(j, i) = image_norm;
                  assert(distances(i, j) == distances(j, i));
                }
              },
              axis, axis, axis);
        }
      }
      assert(distances.rows() == num_atoms && distances.cols() == num_atoms);
      return distances;
    }

    shell_matrix_t narrow_shell_type(auto num_distances, auto num_atoms) {
      if (num_distances < std::numeric_limits<std::uint_fast8_t>::max())
        return matrix_t<std::uint_fast8_t>(num_atoms, num_atoms);
      if (num_distances < std::numeric_limits<std::uint_fast16_t>::max())
        return matrix_t<std::uint_fast16_t>(num_atoms, num_atoms);
      if (num_distances < std::numeric_limits<std::uint_fast32_t>::max())
        return matrix_t<std::uint_fast32_t>(num_atoms, num_atoms);
      if (num_distances < std::numeric_limits<std::uint_fast64_t>::max())
        return matrix_t<std::uint_fast64_t>(num_atoms, num_atoms);
      throw std::runtime_error("Unsupported number of distances");
    }

    template <class T>
    shell_matrix_t shell_matrix(const matrix_t<T> &distance_matrix, T atol, T rtol) {
      assert(distance_matrix.rows() == distance_matrix.cols());
      const auto flattened = distance_matrix.reshaped();
      std::unordered_set<T> unique_distances(flattened.begin(), flattened.end());
      std::vector<T> dists(unique_distances.begin(), unique_distances.end());
      std::sort(dists.begin(), dists.end());
      const auto num_atoms{distance_matrix.rows()};

    auto is_close_tol = [=](T a, T b) { return is_close(a, b, atol, rtol); };

    auto find_shell = [&](T d) {
      if (d < 0) throw std::out_of_range("Invalid distance matrix input");
      if (is_close_tol(d, 0.0)) return 0;

      for (auto i = 0; i < distances.size() - 1; i++) {
        T lb{distances[i]}, up{distances[i + 1]};
        if ((is_close_tol(d, lb) or d > lb) and (is_close_tol(d, up) or up > d)) {
          return i + 1;
        }
      }

      return distances.size();
    };

    std::map<int, size_t> shell_hist;
    for (index_t i = 1; i < distances.size() - 1; i++) shell_hist.emplace(i, 0);

    for (index_t i = 0; i < num_atoms; i++) {
      for (index_t j = i + 1; j < num_atoms; j++) {
        int shell{find_shell(distance_matrix[i][j])};
        if (shell < 0)
          throw std::runtime_error("A shell was detected which I am not aware of");
        else if (shell == 0 and i != j) {
          BOOST_LOG_TRIVIAL(warning)
              << "Atoms " + std::to_string(i) + " and " + std::to_string(j)
                     + " are overlapping! (distance = " + std::to_string(distance_matrix[i][j])
              << ", shell = " << shell << ")!";
        }
        shell_hist[shell] += 2;
        shells[i][j] = shell;
        shells[j][i] = shell;
      }
    }

    BOOST_LOG_TRIVIAL(info) << "structure_utils::shell_matrix::pair_shell_hist="
                            << format_map(shell_hist);

    return shells;
  }*/

  template <class T>
    requires std::is_arithmetic_v<T>
  class Structure {
  private:
    std::optional<matrix_t<T>> _distance_matrix = std::nullopt;
    std::optional<shell_matrix_t> _shell_matrix = std::nullopt;

  public:
    lattice_t<T> lattice;
    coords_t<T> frac_coords;
    std::vector<specie_t> species;
    std::array<bool, 3> pbc = {true, true, true};

    Structure() = default;

    Structure(const lattice_t<T> &lattice, const coords_t<T> &frac_coords,
              const std::vector<specie_t> &species,
              const std::array<bool, 3> &pbc = {true, true, true})
        : lattice(lattice), frac_coords(frac_coords), species(species), pbc(pbc) {
      if (frac_coords.rows() != species.size())
        throw std::invalid_argument("frac coords must have the same size as the species input");
    };

    Structure(lattice_t<T> &&lattice, coords_t<T> &&frac_coords, std::vector<specie_t> &&species,
              std::array<bool, 3> &&pbc = {true, true, true})
        : lattice(lattice), frac_coords(frac_coords), species(species), pbc(pbc) {
      if (frac_coords.rows() != species.size())
        throw std::invalid_argument("frac coords must have the same size as the species input");
    };

    [[nodiscard]] const matrix_t<T> &distance_matrix() {
      if (!_distance_matrix.has_value())
        _distance_matrix = detail::distance_matrix(lattice, frac_coords);
      }

      return _distance_matrix.value();
    }

    [[nodiscard]] const shell_matrix_t &shell_matrix(T atol = std::numeric_limits<T>::epsilon(),
                                                     T rtol = 1.0e-9) {
      if (!_shell_matrix.has_value())
        _shell_matrix = detail::shell_matrix(distance_matrix(), atol, rtol);
      return _shell_matrix.value();
    }
  };
}  // namespace sqsgen::core

#endif  // SQSGEN_CORE_STRUCTURE_H
