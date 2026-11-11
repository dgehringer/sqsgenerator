#include "sqsgen/configuration/triplet_weights.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {
  using shell_triple = std::array<shell_t, 3>;
  using specie_input = std::variant<int, std::string>;

  template <class T> struct triplet_target_input {
    shell_triple shells;                  // shells[k] = shell of the edge opposite corner k
    std::array<specie_input, 3> species;  // species[k] = species at corner k
    T beta;
  };

  template <class T, sublattice_mode Mode> using triplet_targets_t
      = sublattice_mode_nesting<triplet_sro_t<T>, Mode>;

  template <class T, sublattice_mode Mode> using triplet_targets_input
      = std::optional<sublattice_mode_nesting<
          std::optional<std::variant<T, std::vector<triplet_target_input<T>>>>, Mode>>;

  template <class T, sublattice_mode Mode>
  result_t<triplet_targets_t<T, Mode>> validate_triplet_targets_input(
      const triplet_targets_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& present,
      const triplet_weights_t<T, Mode>& weights);

}  // namespace sqsgen::configuration
