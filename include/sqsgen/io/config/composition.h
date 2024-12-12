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

  template <string_literal key>
  parse_result<index_set_t> validate_indices(std::vector<int> const& indices,
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
    return {parsed};
  }

  template <string_literal key>
  parse_result<index_set_t> validate_species_strings(std::vector<std::string> const& species,
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

  template <string_literal key, class Document>
  parse_result<index_set_t> parse_sites(Document const& j, configuration_t const& conf,
                                        index_set_t&& remaining) {
    using result_t = parse_result<index_set_t>;
    return fmap(
               [&](auto&& result) {
                 return result.template collapse<index_set_t>(
                     [&](std::vector<int>&& indices) -> result_t {
                       return validate_indices<key>(indices, conf);
                     },
                     [&](std::vector<std::string>&& species) -> result_t {
                       return validate_species_strings<key>(species, conf);
                     },
                     [&](std::string&& specie) -> result_t {
                       return validate_species_strings<key>(std::vector{specie}, conf);
                     });
               },
               get_either_optional<key, std::vector<int>, std::vector<std::string>, std::string>(j))
        .value_or(result_t{remaining});
  }

  template <string_literal key, class Document>
  parse_result<composition_t> parse_composition(Document const& document, index_set_t&& sites) {
    composition_t composition;
    for (auto& [k, v] : accessor<Document>::items(document)) {
      if (core::SYMBOL_MAP.contains(k)) {
        specie_t specie = core::atom::from_symbol(k).Z;
        auto r = accessor<Document>::template get_as<KEY_NONE, int>(v).and_then(
            [&](auto&& amount) -> parse_result<int> {
              if (amount > sites.size() || amount < 0)
                return parse_error::from_key_and_msg<CODE_BAD_VALUE>(
                    k, std::format(
                           "You want to distribute {} \"{}\" atoms on a sublattice with {} sites",
                           amount, k, sites.size()));
              return amount;
            });
        if (r.failed()) return r.error().with_key(k);
        composition[specie] = r.result();
      }
    }
    if (composition.empty())
      return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
          "You have not distributed any valid species on the (sub)lattice");
    return composition;
  }

  template <string_literal key, class Document>
  parse_result<sublattice> parse_sublattice(Document const& j, configuration_t const& conf,
                                            std::vector<sublattice>&& sublattices) {
    if (!accessor<Document>::is_document(j))
      return parse_error::from_msg<key, CODE_BAD_VALUE>("the JSON value is not an object");
    index_set_t occupied_sites = fold_left(sublattices, index_set_t{}, [](auto&& occ, auto&& sl) {
      occ.merge(sl.sites);
      return occ;
    });
    index_set_t remaining_indices(
        range(conf.size()) | views::filter([&](auto i) { return !occupied_sites.contains(i); }));
    if (remaining_indices.empty())
      return parse_error::from_msg<"sites", CODE_OUT_OF_RANGE>(
          "There are no remaining species left for the  sublattice");

    return parse_sites<"sites">(j, conf, std::move(remaining_indices))
        .and_then([&](auto&& sites) -> parse_result<index_set_t> {
          for (auto&& site : sites)
            if (occupied_sites.contains(site))
              return parse_error::from_msg<"sites", CODE_BAD_VALUE>(
                  std::format("The site with index {} is contained in more than one sublattice. "
                              "Make sure that the "
                              "\"sites\" argument does not contain overlapping index ranges",
                              site));
          return sites;
        })
        .and_then([&](auto&& sites) -> parse_result<sublattice> {
          // parse composition
          auto composition = parse_composition<key>(j, std::forward<decltype(sites)>(sites));
          if (composition.failed()) return composition.error();
          auto num_sites = fold_left(composition.result() | views::elements<1>, 0UL, std::plus<>{});
          if (num_sites != sites.size())
            return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(std::format(
                "The total number of distributed atoms is {} but the sublattice contains {} sites",
                num_sites, sites.size()));
          return sublattice{sites, composition.result()};
        });
  }

  template <string_literal key, class Document>
  parse_result<std::vector<sublattice>> parse_composition(Document const& document,
                                                          configuration_t const& conf) {
    using namespace core::helpers;

    if (!accessor<Document>::contains(document, key.data))
      return parse_error::from_msg<key, CODE_NOT_FOUND>("You need to specify a composition");
    const auto doc = accessor<Document>::get(document, key.data);
    std::vector<sublattice> sublattices;
    for (auto const& d :
         (accessor<Document>::is_list(doc) ? as<std::vector>{}(doc) : std::vector{doc})) {
      auto sl
          = parse_sublattice<key>(d, conf, std::forward<std::vector<sublattice>>(sublattices));
      if (sl.failed()) return sl.error();
      sublattices.push_back(sl.result());
    }
    if (sublattices.empty())
      return parse_error::from_msg<key, CODE_OUT_OF_RANGE>("Could not parse a valid sublattice");
    return {sublattices};
  }

}  // namespace sqsgen::io::config
#endif  // SQSGEN_IO_CONFIG_COMPOSITION_H
