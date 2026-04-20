
// Created by Dominik Gehringer on 20.04.26.
//

#include "sqsgen/core/structure_utils.h"

#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/core/helpers/numeric.h"
#include "sqsgen/log.h"

namespace sqsgen::core::structure_utils {

  counter<specie_t> count_species(configuration_t const &configuration) {
    return helpers::count(configuration);
  }

  template <class T>
  matrix_t<T> distance_matrix(const lattice_t<T> &lattice, const coords_t<T> &frac_coords) {
    const coords_t<T> cart_coords = frac_coords * lattice;
    assert(frac_coords.cols() == 3);
    const auto num_atoms = frac_coords.rows();

    auto a{lattice.row(0)};
    auto b{lattice.row(1)};
    auto c{lattice.row(2)};

    std::array axis{-1, 0, 1};
    matrix_t<T> distances = matrix_t<T>::Ones(num_atoms, num_atoms) * std::numeric_limits<T>::max();
#pragma omp parallel for schedule(static) shared(distances) \
    firstprivate(a, b, c, axis, num_atoms, cart_coords) if (num_atoms > 100)
    for (auto i = 0; i < num_atoms; ++i) {
      auto p1{cart_coords.row(i)};
      for (auto j = i; j < num_atoms; ++j) {
        auto p2{cart_coords.row(j)};
        for (auto u = 0; u < axis.size(); ++u) {
          for (auto v = 0; v < axis.size(); ++v) {
            for (auto w = 0; w < axis.size(); ++w) {
              auto t = u * a + v * b + w * c;
              auto diff = p1 - (t + p2);
              T image_norm{std::abs(diff.norm())};
              if (image_norm < distances(i, j)) {
                distances(i, j) = image_norm;
                if (i != j) distances(j, i) = image_norm;
                assert(distances(i, j) == distances(j, i));
              }
            }
          }
        }
      }
    }
    assert(distances.rows() == num_atoms && distances.cols() == num_atoms);
    return distances;
  }

  template <class T> shell_matrix_t shell_matrix(matrix_t<T> const &distance_matrix,
                                                 std::vector<T> const &dists, T atol, T rtol) {
    assert(distance_matrix.rows() == distance_matrix.cols());
    const auto num_atoms{distance_matrix.rows()};

    auto is_close_tol = [=](T a, T b) { return helpers::is_close(a, b, atol, rtol); };

    auto find_shell = [&](T d) {
      if (d < 0) throw std::out_of_range("Invalid distance matrix input");
      if (is_close_tol(d, 0.0)) return 0;

      for (auto i = 0; i < dists.size() - 1; i++) {
        T lb{dists[i]}, up{dists[i + 1]};
        if ((is_close_tol(d, lb) or d > lb) and (is_close_tol(d, up) or up > d)) {
          return i + 1;
        }
      }
      return static_cast<int>(dists.size());
    };
    shell_matrix_t shells = matrix_t<usize_t>(num_atoms, num_atoms);
    for (auto i = 0; i < num_atoms; i++) {
      for (auto j = i + 1; j < num_atoms; j++) {
        auto shell{find_shell(distance_matrix(i, j))};
        shells(i, j) = static_cast<usize_t>(shell);
        shells(j, i) = static_cast<usize_t>(shell);
      }
      shells(i, i) = 0;
    }
    return shells;
  }

  std::size_t num_species(configuration_t const &configuration) {
    return helpers::sorted_vector<specie_t>(configuration).size();
  }

  template <class T> cube_t<T> compute_prefactors(shell_matrix_t const &shell_matrix,
                                                  shell_weights_t<T> const &weights,
                                                  configuration_t const &configuration) {
    using namespace helpers;
    if (weights.empty()) throw std::out_of_range("no coordination shells selected");
    auto neighbors = helpers::count(shell_matrix.reshaped());
    for (const auto &[shell, count] : neighbors) {
      auto atoms_per_shell{static_cast<T>(count) / static_cast<T>(configuration.size())};
      if (atoms_per_shell < 1)
        log::warn(format_string(
            R"(The coordination shell %i contains no or only one lattice position(s). Increase either "atol" or "rtol" or set the "shell_radii" parameter manually)",
            shell));
      if (!is_close(atoms_per_shell, static_cast<T>(static_cast<usize_t>(atoms_per_shell))))
        log::warn(format_string(
            "The coordination shell %i does not contain an integer number of sites. I hope you "
            "know what you are doing",
            count));
      neighbors[shell] = atoms_per_shell;
    }

    const auto shell_map = std::get<1>(make_index_mapping<usize_t>(weights | views::elements<0>));
    auto conf_map = std::get<1>(make_index_mapping<usize_t>(configuration));

    auto hist = helpers::count(configuration);
    const auto num_sites{configuration.size()};
    const auto num_species{static_cast<long>(hist.size())};
    const auto num_shells{weights.size()};

    Eigen::Tensor<T, 3> prefactors(num_shells, num_species, num_species);

    for (auto s = 0; s < num_shells; s++) {
      const auto neighbor_count = neighbors[shell_map[s]];
      if (neighbor_count > 0) {
        T M_i{static_cast<T>(neighbor_count)};
        for (auto a = 0; a < num_species; a++) {
          T x_a{static_cast<T>(hist[conf_map[a]]) / static_cast<T>(num_sites)};
          for (auto b = a; b < num_species; b++) {
            T x_b{static_cast<T>(hist[conf_map[b]]) / static_cast<T>(num_sites)};
            T prefactor{T(1.0) / (M_i * x_a * x_b * static_cast<T>(num_sites))};
            prefactors(s, a, b) = prefactor;
            prefactors(s, b, a) = prefactor;
          }
        }
      } else {
        log::warn(format_string(
            "The coordination shell %i contains no sites. This shell will not contribute to the "
            "objective function and can be removed from the \"shell_weights\" parameter",
            shell_map[s]));
        for (auto a = 0; a < num_species; a++)
          for (auto b = a; b < num_species; b++) {
            prefactors(s, a, b) = 0;
            prefactors(s, b, a) = 0;
          }
      }
    }
    return prefactors;
  }
}  // namespace sqsgen::core::structure_utils
