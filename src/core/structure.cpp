//
// Created by Dominik Gehringer on 20.04.26.
//

#include "sqsgen/core/structure.h"

#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/core/helpers/numeric.h"
#include "sqsgen/core/structure_utils.h"

namespace sqsgen::core {

  template <class T> std::vector<T> structure<T>::distances_naive(T atol, T rtol) {
    helpers::sorted_vector<T> dists(distance_matrix().reshaped());
    std::vector<T> reduced{0};
    for (auto dist : dists)
      if (helpers::is_close(reduced.back(), dist, atol, rtol))
        reduced[reduced.size() - 1] = 0.5 * (dist + reduced.back());
      else
        reduced.push_back(dist);
    return reduced;
  }

  template <class T>
  std::vector<T> structure<T>::distances_histogram(T bin_width, T peak_isolation) {
    std::vector<T> distances;
    for (auto dist : distance_matrix().reshaped)
      if (dist > 0.0 && !helpers::is_close<T>(dist, 0.0)) distances.push_back(dist);

    std::sort(distances.begin(), distances.end());
    T min_dist{distances.front()}, max_dist{distances.back()};

    auto num_edges{static_cast<std::size_t>((max_dist - min_dist) / bin_width) + 2};

    // for some reason as<std::vector> does not work here
    auto edges = std::vector<T>(num_edges);
    for (std::size_t i = 0; i < num_edges; i++) edges[i] = min_dist + static_cast<T>(i) * bin_width;

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
    // make sure our histogram contains the same number of distances as the corresponding input
    // vector

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

  template <class T> const matrix_t<T> &structure<T>::distance_matrix() {
    if (!_distance_matrix.has_value())
      _distance_matrix = structure_utils::distance_matrix(lattice, frac_coords);

    return _distance_matrix.value();
  }

  template <class T>
  shell_matrix_t structure<T>::shell_matrix(std::vector<T> const &shell_radii, T atol, T rtol) {
    return structure_utils::shell_matrix(distance_matrix(), shell_radii, atol, rtol);
  }

  template <class T>
  structure<T> structure<T>::supercell(std::size_t a, std::size_t b, std::size_t c) const {
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
    for (auto i = 0; i < a; ++i)
      for (auto j = 0; j < b; ++j)
        for (auto k = 0; k < c; ++k) {
          using vec3_t = Eigen::Matrix<T, 1, 3>;
          vec3_t translation
              = vec3_t{static_cast<T>(i), static_cast<T>(j), static_cast<T>(k)} * iscale;
          for (auto index = 0; index < num_atoms; ++index) {
            supercell_coords.row(site_index) = translation + scaled_frac_coords.row(index);
            supercell_species[site_index] = species[index];
            site_index++;
          }
        }

    return structure(lattice * scale, supercell_coords, supercell_species, pbc);
  }

  template <class T> std::size_t structure<T>::size() const { return species.size(); }

  template <class T> std::vector<site<T>> structure<T>::sites() const {
    std::vector<site<T>> sites(size());
    for (auto index = 0; index < size(); ++index)
      sites[index] = site<T>{index, species[index], Eigen::Vector3<T>(frac_coords.row(index))};
    return sites;
  }

  template <class T>
  structure<T> structure<T>::apply_composition(std::vector<sublattice> const &composition) {
    auto copy = structure(*this);
    for (const auto &[sites, species_] : composition) {
      auto index = sites.begin();
      for (auto &&[specie, amount] : species_)
        for (auto _ = 0; _ < amount; _++, ++index) copy.species[*index] = specie;
    }
    copy.num_species = helpers::count(copy.species).size();
    return copy;
  }

  template <class T> structure<T> structure<T>::with_species(configuration_t const &conf) const {
    if (conf.size() != size()) throw std::invalid_argument("Species size mismatch");
    return structure{lattice, frac_coords, conf, pbc};
  }

  template <class T> std::vector<structure<T>> structure<T>::apply_composition_and_decompose(
      std::vector<sublattice> const &composition) {
    auto with_species = apply_composition(composition);
    std::vector<structure<T>> result;
    for (const auto &sublattice : composition)
      result.push_back(with_species.sliced(sublattice.sites));
    return result;
  }

  template <class T> structure<T> structure<T>::without_vacancies() const {
    return filtered([](auto site) { return site.specie != 0; });
  }

  template <class T> auto structure<T>::pairs(std::vector<T> const &radii,
                                              shell_weights_t<T> const &weights, bool pack, T atol,
                                              T rtol) {
    using namespace helpers;
    auto [shell_map, reverse_map] = make_index_mapping<usize_t>(weights | views::elements<0>);
    std::vector<atom_pair<usize_t>> pairs;
    pairs.reserve(size() * size() / 2);
    auto sm = shell_matrix(radii, atol, rtol);
    for (usize_t i = 0; i < size(); ++i) {
      for (usize_t j = i + 1; j < size(); ++j) {
        if (!weights.contains(sm(i, j))) continue;
        pairs.push_back({i, j, static_cast<usize_t>(pack ? shell_map[sm(i, j)] : sm(i, j))});
      }
    }
    pairs.shrink_to_fit();
    return std::make_tuple(pairs, shell_map, reverse_map);
  }

  template <class T> configuration_t structure<T>::packed_species() const {
    auto [map, _] = helpers::make_index_mapping<specie_t>(species);
    configuration_t packed;
    packed.reserve(species.size());
    std::transform(species.begin(), species.end(), std::back_inserter(packed),
                   [&map](auto z) { return map[z]; });
    return packed;
  }

  template <class T> rank_t structure<T>::rank() const {
    return rank_permutation(packed_species());
  }

  template <class T> std::string structure<T>::uuid() const {
    // TODO: fix for boost::cpp_int
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
  };
}  // namespace sqsgen::core
