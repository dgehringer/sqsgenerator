//
// Created by Dominik Gehringer on 23.11.24.
//

#ifndef SQSGEN_IO_CONFIG_COMPOSITION_H
#define SQSGEN_IO_CONFIG_COMPOSITION_H

#include "sqsgen/core/atom.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/io/json.h"
#include "sqsgen/types.h"

namespace sqsgen::io::config {

  namespace ranges = std::ranges;
  namespace views = ranges::views;

  using index_set_t = vset<usize_t>;

  class composition_config {
    std::vector<sublattice> sublattices;
  };

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
    vset<specie_t> unique_species;
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
    vset<usize_t> indices;
    core::helpers::for_each(
        [&](auto i) {
          if (unique_species.contains(conf.at(i))) indices.insert(static_cast<usize_t>(i));
        },
        conf.size());
    return indices;
  }

  template <core::helpers::string_literal key>
  parse_result_t<std::optional<index_set_t>> parse_sites(nlohmann::json const& j,
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

  template <core::helpers::string_literal key>
  parse_result_t<sublattice> parse_sublattice(nlohmann::json const& j, configuration_t const& conf,
                                              std::vector<sublattice>&& sublattices) {
    using namespace core::helpers;
    if (!j.is_object())
      return parse_error::from_msg<key, CODE_BAD_VALUE>("the JSON value is not an object");
    auto sites_result = parse_sites<"sites">(j, conf);
    if (holds_error(sites_result)) return std::get<parse_error>(sites_result);
    auto sites = get_result(sites_result);
    index_set_t occupied_sites = fold_left(sublattices, index_set_t{}, [](auto&& occ, auto&& sl) {
      occ.merge(sl.sites);
      return occ;
    });
    index_set_t indices;
    if (sites.has_value()) {
      // make sure that sublattices do not overlap
      for (auto&& site : sites.value())
        if (occupied_sites.contains(site))
          return parse_error::from_msg<"sites", CODE_BAD_VALUE>(std::format(
              "The site with index {} is contained in more than one sublattice. Make sure that the "
              "\"sites\" argument does not contain overlapping index ranges",
              site));
      indices = sites.value();
    } else {
      index_set_t remaining_indices(
          range(conf.size()) | views::filter([&](auto i) { return !occupied_sites.contains(i); }));
      if (remaining_indices.empty())
        return parse_error::from_msg<"sites", CODE_OUT_OF_RANGE>(
            "There are no remaining species left for the sublattice");
      indices = remaining_indices;
    }
    composition_t composition;
    for (auto& [k, value] : j.items()) {
      if (core::SYMBOL_MAP.contains(k)) {
        specie_t specie = core::atom::from_symbol(k).Z;
        auto num_result = get_as<int>(k, value);
        if (holds_error(num_result)) return std::get<parse_error>(num_result);
        auto amount = get_result(num_result);
        if (amount > indices.size() || amount < 0)
          return parse_error::from_key_and_msg<CODE_BAD_VALUE>(
              k,
              std::format("You want to distribute {} \"{}\" atoms on the sublattice", amount, k));
        composition[specie] = amount;
      }
    }

    if (composition.empty())
      return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
          "Could not find any valid species in the composition definition");

    return sublattice{indices, composition};
  }

  template <core::helpers::string_literal key>
  parse_result_t<std::vector<sublattice>> parse_composition(nlohmann::json const& jj,
                                                            configuration_t const& conf) {
    if (!jj.count(key.data))
      return parse_error::from_msg<key, CODE_NOT_FOUND>("You need to specify a composition");
    const nlohmann::json& j = jj.at(key.data);
    std::vector<sublattice> sublattices(j.is_array() ? j.size() : 1);
    for (auto const& json : (j.is_array() ? core::helpers::as<std::vector>{}(j) : std::vector{j})) {
      auto sl
          = parse_sublattice<key>(json, conf, std::forward<std::vector<sublattice>>(sublattices));
      if (holds_error(sl)) return std::get<parse_error>(sl);
      sublattices.push_back(get_result(sl));
    }
    if (sublattices.empty())
      return parse_error::from_msg<key, CODE_OUT_OF_RANGE>("Could find a species definition");
    return sublattices;
  }

}  // namespace sqsgen::io::parser
#endif  // SQSGEN_IO_CONFIG_COMPOSITION_H
