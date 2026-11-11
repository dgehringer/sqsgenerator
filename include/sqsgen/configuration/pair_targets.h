
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  using specie_input = std::variant<int, std::string>;  // atomic number or chemical symbol

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

}  // namespace sqsgen::configuration
