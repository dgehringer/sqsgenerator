//
// Created by Dominik Gehringer on 22.10.24.
//

#ifndef SQSGEN_CORE_STRUCTURE_H
#define SQSGEN_CORE_STRUCTURE_H

#include <Eigen/Dense>

#include "spdlog/spdlog.h"
#include "sqsgen/core/atom.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/core/permutation.h"
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

  template <class T>
    requires std::is_arithmetic_v<T>
  class structure;

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
      }
      helpers::for_each([&](auto i) { shells(i, i) = 0; }, num_atoms);
      return shells;
    }

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

    inline usize_t compute_num_species(configuration_t const &configuration) {
      return static_cast<usize_t>(helpers::sorted_vector<specie_t>(configuration).size());
    }

    template <class T> cube_t<T> compute_prefactors(shell_matrix_t const &shell_matrix,
                                                    shell_weights_t<T> const &weights,
                                                    configuration_t const &configuration) {
      using namespace helpers;
      if (weights.empty()) throw std::out_of_range("no coordination shells selected");
      auto neighbors = count(shell_matrix.reshaped());
      for (const auto &[shell, count] : neighbors) {
        auto atoms_per_shell{static_cast<T>(count) / static_cast<T>(configuration.size())};
        if (atoms_per_shell < 1)
          spdlog::warn(
              R"(The coordination shell {} contains no or only one lattice position(s). Increase either "atol" or "rtol" or set the "shell_radii" parameter manually)",
              shell);
        if (!is_close(atoms_per_shell, static_cast<T>(static_cast<usize_t>(atoms_per_shell))))
          spdlog::warn(
              "The coordination shell {} does not contain an integer number of sites. I hope you "
              "know what you are doing");
        neighbors[shell] = atoms_per_shell;
      }

      auto shell_map = std::get<1>(make_index_mapping<usize_t>(weights | views::elements<0>));
      auto conf_map = std::get<1>(make_index_mapping<usize_t>(configuration));

      auto hist = core::count_species(configuration);
      auto num_sites{configuration.size()};
      auto num_species{static_cast<long>(hist.size())};
      auto num_shells{weights.size()};

      Eigen::Tensor<T, 3> prefactors(num_shells, num_species, num_species);

      for (auto s = 0; s < num_shells; s++) {
        T M_i{static_cast<T>(neighbors[shell_map[s]])};
        for (auto a = 0; a < num_species; a++) {
          T x_a{static_cast<T>(hist[conf_map[a]]) / static_cast<T>(num_sites)};
          for (auto b = a; b < num_species; b++) {
            T x_b{static_cast<T>(hist[conf_map[b]]) / static_cast<T>(num_sites)};
            T prefactor{T(1.0) / (M_i * x_a * x_b * static_cast<T>(num_sites))};
            prefactors(s, a, b) = prefactor;
            prefactors(s, b, a) = prefactor;
          }
        }
      }
      return prefactors;
    }
  }  // namespace detail

  template <class T> std::vector<T> distances_naive(structure<T> &&structure,
                                                    T atol = std::numeric_limits<T>::epsilon(),
                                                    T rtol = 1e-9) {
    helpers::sorted_vector<T> dists(structure.distance_matrix().reshaped());
    auto reduced = helpers::fold_left(dists, std::vector<T>{T(0)}, [&](auto &&vec, auto dist) {
      if (helpers::is_close(vec.back(), dist, atol, rtol))
        vec[vec.size() - 1] = 0.5 * (dist + vec.back());
      else
        vec.push_back(dist);
      return vec;
    });
    return reduced;
  }

  template <class T>
  std::vector<T> distances_histogram(structure<T> &&structure, T bin_width, T peak_isolation) {
    auto distances = helpers::as<std::vector>{}(
        structure.distance_matrix().reshaped()
        | views::filter([](auto dist) { return dist > 0.0 && !helpers::is_close<T>(dist, 0.0); }));
    std::sort(distances.begin(), distances.end());
    auto min_dist{distances.front()}, max_dist{distances.back()};

    auto num_edges{static_cast<std::size_t>((max_dist - min_dist) / bin_width) + 2};
    auto edges
        = helpers::as<std::vector>{}(views::iota(0UL, num_edges) | views::transform([&](auto i) {
                                       return T(min_dist) + i * bin_width;
                                     }));

    if (edges.size() < 10)
      throw std::invalid_argument(
          "Not enough edges to create a histogram, please increase the bin width");
    auto freqs = std::map<usize_t, std::vector<T>>{{0, {}}};
    usize_t index{0}, bin{0};
    while (index < distances.size() && bin < num_edges - 1) {
      T lower{edges[bin]}, upper{edges[bin + 1]}, value{distances[index]};
      if (lower <= value && value < upper) {
        freqs[bin].push_back(value);
        ++index;
      } else
        freqs[++bin] = std::vector<T>{};
    }
    // make sure our histogramm contians the same number of distances as the corresponding input
    // vector
    assert(helpers::fold_left(
               views::elements<1>(freqs) | views::transform([&](auto v) { return v.size(); }), 0,
               std::plus{})
           == distances.size());
    const auto get_freq
        = [&](auto i) { return freqs.contains(i) ? freqs.at(i) : std::vector<T>{}; };
    assert(freqs.size() == num_edges - 1);
    std::vector<T> shells;
    for (auto i = 0; i <= num_edges; i++) {
      auto prev{get_freq(i - 1)}, f{get_freq(i)}, next{get_freq(i + 1)};
      auto threshold = static_cast<std::size_t>((1.0 - peak_isolation) * static_cast<T>(f.size()));
      if (threshold > prev.size() && threshold > next.size()) {
        assert(freqs.size() > 0);
        shells.push_back(*std::max_element(f.begin(), f.end()));
      }
    }

    if (shells.front() != 0.0 && !helpers::is_close<T>(shells.front(), 0.0))
      shells.insert(shells.begin(), 0.0);
    return shells;
  }

  template <class T> cube_t<T> compute_prefactors(structure<T> &&structure,
                                                  std::vector<T> const &shell_radii,
                                                  shell_weights_t<T> const &weights) {
    return detail::compute_prefactors<T>(structure.shell_matrix(shell_radii), weights,
                                         structure.species);
  }

  template <class T>
    requires std::is_arithmetic_v<T>
  class structure {
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
      requires std::is_same_v<ranges::range_value_t<R>, sqsgen::core::detail::site<T>>
    structure(const lattice_t<T> &lattice, R &&r) : lattice(lattice) {
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

    structure(const lattice_t<T> &lattice, const coords_t<T> &frac_coords,
              configuration_t const &species, const std::array<bool, 3> &pbc = {true, true, true})
        : lattice(lattice),
          frac_coords(frac_coords),
          species(species),
          pbc(pbc),
          num_species(detail::compute_num_species(species)) {
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
          num_species(detail::compute_num_species(species)) {
      if (frac_coords.rows() != species.size() || species.empty())
        throw std::invalid_argument(
            "frac coords must have the same size as the species input and must not be empty");
    };

    [[nodiscard]] const matrix_t<T> &distance_matrix() {
      if (!_distance_matrix.has_value())
        _distance_matrix = detail::distance_matrix(lattice, frac_coords);

      return _distance_matrix.value();
    }

    [[nodiscard]] shell_matrix_t shell_matrix(std::vector<T> const &shell_radii,
                                              T atol = std::numeric_limits<T>::epsilon(),
                                              T rtol = 1.0e-9) {
      return sqsgen::core::detail::shell_matrix(distance_matrix(), shell_radii, atol, rtol);
    }

    [[nodiscard]] structure supercell(std::size_t a, std::size_t b, std::size_t c) const {
      auto num_copies = a * b * c;
      if (num_copies == 0)
        throw std::invalid_argument("There must be at least one copy in each direction");
      lattice_t<T> scale = lattice_t<T>::Zero();
      scale(0, 0) = a;
      scale(1, 1) = b;
      scale(2, 2) = c;

      auto site_index{0};
      auto num_atoms{species.size()};
      coords_t<T> supercell_coords(num_atoms * num_copies, 3);
      lattice_t<T> iscale = scale.inverse();
      coords_t<T> scaled_frac_coords = frac_coords * iscale.transpose();
      std::vector<specie_t> supercell_species(num_atoms * num_copies);
      helpers::for_each(
          [&](auto i, auto j, auto k) {
            using vec3_t = Eigen::Matrix<T, 1, 3>;
            vec3_t translation = vec3_t{i, j, k} * iscale;
            helpers::for_each(
                [&](auto index) {
                  supercell_coords.row(site_index) = translation + scaled_frac_coords.row(index);
                  supercell_species[site_index] = species[index];
                  site_index++;
                },
                num_atoms);
          },
          a, b, c);

      return structure(lattice * scale, supercell_coords, supercell_species, pbc);
    }

    [[nodiscard]] std::size_t size() const { return species.size(); }

    auto sites() const {
      return ranges::iota_view(static_cast<usize_t>(0), static_cast<usize_t>(size()))
             | views::transform([&](auto i) {
                 return sqsgen::core::detail::site<T>{i, species[i],
                                                      Eigen::Vector3<T>(frac_coords.row(i))};
               });
    }

    template <class Fn> auto sorted_with_indices(Fn &&fn) const {
      auto s = helpers::as<std::vector>{}(sites());
      std::sort(s.begin(), s.end(), std::forward<Fn>(fn));
      return std::make_tuple(
          structure(lattice, s),
          helpers::as<std::vector>{}(s | views::transform([](auto site) { return site.index; })));
    }

    template <class Fn> auto sorted(Fn &&fn) const {
      return std::get<0>(sorted_with_indices(std::forward<Fn>(fn)));
    }

    structure apply_composition(std::vector<sublattice> const &composition) {
      auto copy = structure(*this);
      for (const auto &[sites, species] : composition) {
        auto index = sites.begin();
        for (auto &&[specie, amount] : species)
          for (auto _ = 0; _ < amount; _++, ++index) copy.species[*index] = specie;
      }
      copy.num_species = detail::compute_num_species(copy.species);
      return copy;
    }

    structure with_species(configuration_t const &conf) {
      if (conf.size() != size()) throw std::invalid_argument("Species size mismatch");
      return structure{lattice, frac_coords, conf, pbc};
    }

    std::vector<structure> apply_composition_and_decompose(
        std::vector<sublattice> const &composition) {
      auto with_species = apply_composition(composition);
      return helpers::as<std::vector>{}(
          composition | views::transform([&](auto &&sl) { return with_species.sliced(sl.sites); }));
    }

    template <class Fn> auto filtered(Fn &&fn) const {
      return structure(lattice, sites() | views::filter(std::forward<Fn>(fn)));
    }

    template <ranges::input_range R, class V = ranges::range_value_t<R>>
      requires std::is_integral_v<V>
    structure sliced(R &&r) const {
      auto sites = std::vector<detail::site<T>>{};
      for (auto index : r) {
        if (index >= size() || index < 0)
          throw std::out_of_range(std::format("index out of range 0 <= {} < {}", index, size()));
        sites.push_back(detail::site<T>{static_cast<usize_t>(index), species[index],
                                        Eigen::Vector3<T>(frac_coords.row(index))});
      }
      return structure(lattice, sites);
    }

    template <class Size = usize_t>
      requires std::is_integral_v<Size>
    auto pairs(std::vector<T> const &radii, shell_weights_t<T> const &weights, bool pack = true,
               T atol = std::numeric_limits<T>::epsilon(), T rtol = 1.0e-9) {
      using namespace helpers;
      auto [shell_map, reverse_map] = make_index_mapping<Size>(weights | views::elements<0>);
      std::vector<atom_pair<Size>> pairs;
      pairs.reserve(size() * size() / 2);
      auto sm = shell_matrix(radii, atol, rtol);
      for (Size i = 0; i < size(); ++i) {
        for (Size j = i + 1; j < size(); ++j) {
          if (!weights.contains(sm(i, j))) continue;
          pairs.push_back({i, j, static_cast<Size>(pack ? shell_map[sm(i, j)] : sm(i, j))});
        }
      }
      pairs.shrink_to_fit();
      return std::make_tuple(pairs, shell_map, reverse_map);
    }

    [[nodiscard]] configuration_t packed_species() const {
      auto [map, _] = helpers::make_index_mapping<specie_t>(species);
      return helpers::as<std::vector>{}(species | views::transform([&](auto z) { return map[z]; }));
    }

    [[nodiscard]] rank_t rank() const { return rank_permutation(packed_species()); }

    [[nodiscard]] std::string uuid() const {
      std::vector<char> data(16);
      ranges::fill(data, 0);
      rank().binary_save(data);
      std::string uuid;
      uuid.reserve(32);
      constexpr const char *hex_digits = "0123456789abcdef";
      const auto write_hexencoded = [&](char c) {
        uuid.push_back(hex_digits[c >> 4 & 0xF]);
        uuid.push_back(hex_digits[c & 0xF]);
      };

      auto ptr = data.begin();
      const auto write_next_char = [&] { write_hexencoded(*ptr++); };

      for (auto i = 0; i < 6; i++) write_next_char();
      char split = *ptr++;
      // the two nibbles must contain the version and the variant
      // we have to split one byte across version and variant nibble

      // version nibble
      uuid.push_back('8');
      uuid.push_back(hex_digits[split >> 4 & 0xF]);
      write_next_char();

      // variant nibble
      uuid.push_back('b');
      uuid.push_back(hex_digits[split & 0xF]);
      write_next_char();
      for (auto i = 0; i < 6; i++) write_next_char();
      assert(uuid.size() == 32);
      return uuid;
    }
  };

  template <typename T> using site_t = detail::site<T>;

}  // namespace sqsgen::core

#endif  // SQSGEN_CORE_STRUCTURE_H
