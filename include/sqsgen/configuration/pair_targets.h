
#include "sqsgen/configuration/pair_weights.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  using specie_input = std::variant<int, std::string>;

  template <class T> struct pair_target_input {
    std::optional<shell_t> shell;  // std::nullopt = all shells
    std::array<specie_input, 2> species;
    T alpha;
  };

  template <class T, sublattice_mode Mode> using pair_targets_input
      = std::optional<sublattice_mode_nesting<
          std::optional<std::variant<T, std::vector<pair_target_input<T>>>>, Mode>>;

  template <class T, sublattice_mode Mode> using pair_targets_t
      = sublattice_mode_nesting<pair_sro_t<T>, Mode>;

  template <class T, sublattice_mode Mode>
  result_t<pair_targets_t<T, Mode>> validate_pair_targets_input(
      const pair_targets_input<T, Mode>& input,
      const sublattice_mode_nesting<core::structure<T>, Mode>& present,
      const pair_weights_t<T, Mode>& weights);

}  // namespace sqsgen::configuration
