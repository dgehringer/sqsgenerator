//
// Created by Dominik Gehringer on 23.11.24.
//

#ifndef SQSGEN_IO_PARSER_COMPOSITION_CONFIG_H
#define SQSGEN_IO_PARSER_COMPOSITION_CONFIG_H

#include "config.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/io/json.h"
#include "sqsgen/types.h"

namespace sqsgen::io::parser {

  using index_set_t = std::set<usize_t>;

  template <core::helpers::string_literal key>
  parse_result_t<index_set_t> validate_indices(std::vector<int> const& indices,
                                               configuration_t const& conf) {
    index_set_t parsed;
    for (auto index : indices)
      if (0 <= index && index < conf.size())
        parsed.insert(static_cast<size_t>(index));
      else
        return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(std::format(
            "Invalid index {}, the specified structure contains {} sites", index, conf.size()));

    if (parsed.empty())
      return parse_error::from_msg<key, CODE_OUT_OF_RANGE>("You cannot specify an empty slice");
    return parsed;
  }

  template <core::helpers::string_literal key>
  parse_result_t<index_set_t> validate_symbols(std::vector<std::string> const& species,
                                               configuration_t const& conf) {
    auto distinct_species = core::count_species(conf);
    std::set<specie_t> unique_species;
    for (auto s : species) {
      if (!core::SYMBOL_MAP.contains(s))
        return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
            std::format("An atomic element with name {} is not known to me", s));
      auto ordinal = core::atom::from_symbol(s).Z;
      if (!distinct_species.contains(ordinal))
        return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
            std::format("The specified structure does not contain a site with species {}", s));
      unique_species.insert(ordinal);
    }
    std::set<usize_t> indices;
    core::helpers::for_each(
        [&](auto i) {
          if (unique_species.contains(conf.at(i))) return indices.insert(static_cast<usize_t>(i));
        },
        conf.size());
    return indices;
  }

  template <core::helpers::string_literal key>
  parse_result_t<std::optional<std::set<usize_t>>> parse_which(nlohmann::json const& j,
                                                               configuration_t const& conf) {
    using out_t = parse_result_t<index_set_t>;
    return validate<index_set_t>(
        get_either_optional<key, std::vector<int>, std::vector<std::string>, std::string>(j),
        [&](std::vector<int>&& indices) -> out_t { return validate_indices<key>(indices, conf); },
        [&](std::vector<std::string>&& species) -> out_t {
          return validate_symbols<key>(species, conf);
        },
        [&](std::string&& specie) -> out_t { return validate_symbols<key>({specie}, conf); });
  }

}  // namespace sqsgen::io::parser
#endif  // SQSGEN_IO_PARSER_COMPOSITION_CONFIG_H
