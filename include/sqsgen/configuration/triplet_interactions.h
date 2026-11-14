
#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  using specie_input = std::variant<int, std::string>;  // atomic number or chemical symbol
  using shell_triple = std::array<shell_t, 3>;

  template <class T> struct triplet_interaction_input {
    shell_triple shells;
    std::array<specie_input, 3> species;
    T interaction;
  };

  template <class T, sublattice_mode Mode> using triplet_interactions_t
      = sublattice_mode_nesting<triplet_sro_t<T>, Mode>;

  template <class T, sublattice_mode Mode> using triplet_interactions_input
      = sublattice_mode_nesting<std::vector<triplet_interaction_input<T>>, Mode>;

  template <class T, sublattice_mode Mode>
  result_t<triplet_interactions_t<T, Mode>> validate_triplet_interactions_input(
      const triplet_interactions_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& structure,
      const sublattice_mode_nesting<core::triplet_class_mapping_t, Mode>& triplet_classes);

  template <class T, sublattice_mode Mode>
  triplet_interactions_input<T, Mode> to_triplet_interactions_input(
      const triplet_interactions_t<T, Mode>& interactions);
}  // namespace sqsgen::configuration
