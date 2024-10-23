//
// Created by Dominik Gehringer on 22.10.24.
//

#ifndef SQSGEN_CORE_STRUCTURE_H
#define SQSGEN_CORE_STRUCTURE_H
#include <unordered_set>

#include "sqsgen/core/helpers.h"
#include "sqsgen/types.h"

namespace sqsgen::core {

  template <class T>
  matrix_t<T> distance_matrix(const lattice_t<T> &lattice, const coords_t<T> &frac_coords) {
    const matrix_t<T> cart_coords{lattice.dot(frac_coords)};
    assert(frac_coords.rows() == 3);
    const auto num_atoms = frac_coords.cols();

    auto a{lattice.row(0)};
    auto b{lattice.row(1)};
    auto c{lattice.row(2)};

    std::array axis{-1, 0, 1};
    auto distances = matrix_t<T>::Zero(num_atoms, num_atoms);
#pragma omp parallel for schedule(static) shared(vecs) \
    firstprivate(a, b, c, axis, num_atoms, cart_coords) if (num_atoms > 100)
    for (auto pi1 = 0; pi1 < num_atoms; pi1++) {
      auto p1{cart_coords.col(pi1)};
      for (auto pi2 = pi1 + 1; pi2 < num_atoms; pi2++) {
        auto p2{cart_coords.col(pi2)};
        T norm{std::numeric_limits<T>::max()};
        helpers::for_each(
            [&](auto i, auto j, auto k) {
              auto t = i * a + j * b + k * c;
              auto diff = p1 - (t + p2);
              T image_norm{diff.norm()};
              if (image_norm < norm) {
                norm = image_norm;
                distances(pi1, pi2) = norm;
                distances(pi2, pi2) = norm;
              }
            },
            axis, axis, axis);
      }
    }
    return distances;
  }

  /*
  template <class T>
  pair_shell_matrix_t shell_matrix(const matrix_t<T> &distance_matrix, T atol, T rtol) {
    assert(distance_matrix.rows() == distance_matrix.cols());
    const std::unordered_set<T> unique_distances(distance_matrix.begin(), distance_matrix.end());
    const std::vector<T> distances(unique_distances.begin(), unique_distances.end());
    std::sort(distances.begin(), distances.end());
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
  public:
    lattice_t<T> lattice;
    coords_t<T> frac_coords;
    std::vector<specie_t> species;
    std::array<bool, 3> pbc = {true, true, true};

    Structure(const lattice_t<T> &lattice, const coords_t<T> &frac_coords,
              const std::vector<specie_t> &species,
              const std::array<bool, 3> &pbc = {true, true, true})
        : lattice(lattice), frac_coords(frac_coords), species(species), pbc(pbc) {
      if (frac_coords.rows() != species.size())
        throw std::invalid_argument("frac coords must have the same size as the species input");
    };

    Structure(lattice_t<T> &lattice, coords_t<T> &&frac_coords, std::vector<specie_t> &&species,
              std::array<bool, 3> &&pbc = {true, true, true})
        : lattice(lattice), frac_coords(frac_coords), species(species), pbc(pbc) {
      if (frac_coords.rows() != species.size())
        throw std::invalid_argument("frac coords must have the same size as the species input");
    };

    [[nodiscard]] const matrix_t<T> &distance_matrix() const {
      if (!_distance_matrix.has_value())
        _distance_matrix = distance_matrix<T>(lattice, frac_coords);
      return _distance_matrix.value();
    }

  private:
    std::optional<matrix_t<T>> _distance_matrix;
    // std::optional<pair_shell_matrix_t> _shell_matrix = std::nullopt;
  };
}  // namespace sqsgen::core

#endif  // SQSGEN_CORE_STRUCTURE_H
