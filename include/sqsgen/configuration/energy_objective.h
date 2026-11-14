
#include "sqsgen/configuration/pair_interactions.h"
#include "sqsgen/configuration/triplet_interactions.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  template <class T, sublattice_mode Mode> struct energy_objective {
    std::optional<pair_interactions_t<T, Mode>> pairs;
    std::optional<triplet_interactions_t<T, Mode>> triplets;
  };

  template <class T, sublattice_mode Mode> struct energy_objective_input {
    std::optional<pair_interactions_input<T, Mode>> pairs;
    std::optional<triplet_interactions_input<T, Mode>> triplets;
  };

  template <class T, sublattice_mode Mode>
  result_t<energy_objective<T, Mode>> validate_energy_objective_input(
      const energy_objective_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& structure,
      const sublattice_mode_nesting<core::triplet_class_mapping_t, Mode>& triplet_classes,
      bool pairs, bool triplets);

  template <class T, sublattice_mode Mode>
  energy_objective_input<T, Mode> to_energy_objective_input(const energy_objective<T, Mode>&);
}  // namespace sqsgen::configuration
