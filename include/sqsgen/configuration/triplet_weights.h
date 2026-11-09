
#include "sqsgen/configuration/pair_weights.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {
  using shell_triple = std::array<shell_t, 3>;

  template <class T, sublattice_mode Mode> using triplet_weights_input = std::optional<
      sublattice_mode_nesting<std::optional<std::vector<std::tuple<shell_triple, T>>>, Mode>>;

  template <class T, sublattice_mode Mode> using triplet_weights_t
      = sublattice_mode_nesting<std::map<shell_triple, T>, Mode>;

  template <class T, sublattice_mode Mode>
  result_t<triplet_weights_t<T, Mode>> validate_triplet_weights_input(
      const triplet_weights_t<T, Mode>&,
      const sublattice_mode_nesting<core::triplet_class_mapping_t, Mode>& triplet_classes,
      const sublattice_mode_nesting<std::vector<T>, Mode>& shell_radii);

}  // namespace sqsgen::configuration
