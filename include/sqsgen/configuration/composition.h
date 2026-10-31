//
// Created by Dominik Gehringer on 23.11.24.
//

#ifndef SQSGEN_CONFIGURATION_COMPOSITION_H
#define SQSGEN_CONFIGURATION_COMPOSITION_H

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  using indices_t = core::sorted_vector<std::size_t>;

  using site_spec = std::variant<std::vector<int>, std::vector<std::string>, std::string, int>;
  using composition_spec = std::map<std::string, site_spec>;
  using composition_input = std::variant<std::vector<composition_spec>, composition_spec>;

  struct sublattice_helper {
    std::optional<indices_t> sites = std::nullopt;
    composition_t composition;
  };

  result_t<std::vector<sublattice>> validate_composition_input(const composition_input&,
                                                               const configuration_t& conf);

}  // namespace sqsgen::configuration

#include "sqsgen/impl/configuration/composition_impl.hpp"

#endif  // SQSGEN_CONFIGURATION_COMPOSITION_H
