//
// Created by Dominik Gehringer on 23.11.24.
//

#ifndef SQSGEN_IMPL_CONFIGURATION_COMPOSITION_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_COMPOSITION_IMPL_HPP

#include <cstddef>
#include <expected>
#include <format>
#include <map>
#include <numeric>
#include <optional>
#include <ranges>
#include <string>
#include <variant>
#include <vector>

#include "sqsgen/configuration/composition.h"
#include "sqsgen/core/atom.h"
#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/core/helpers/variant.h"
#include "sqsgen/core/permutation.h"
#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace core = sqsgen::core;
  namespace helpers = sqsgen::core::helpers;

  inline result_t<indices_t> validate_indices(std::vector<int> const& indices,
                                              configuration_t const& conf,
                                              std::optional<int> index = std::nullopt) {
    std::string key = index.has_value() ? std::format("composition.{}.sites", index.value())
                                        : "composition.sites";
    indices_t parsed;
    for (auto index : indices)
      if (0 <= index && index < conf.size())
        parsed.insert(static_cast<size_t>(index));
      else
        return std::unexpected(configuration_error{
            key, error_code::out_of_range,
            std::format("Invalid index {}, the specified structure contains {} sites", index,
                        conf.size())});
    if (parsed.empty())
      return std::unexpected(
          configuration_error{key, error_code::invalid_size, "You cannot specify an empty slice"});

    return parsed;
  }

  inline result_t<indices_t> validate_species_strings(std::vector<std::string> const& species,
                                                      configuration_t const& conf,
                                                      std::optional<int> index = std::nullopt) {
    std::string key = index.has_value() ? std::format("composition.{}.sites", index.value())
                                        : "composition.sites";
    auto distinct_species = core::count_species(conf);
    core::sorted_vector<specie_t> unique_species;
    for (auto s : species) {
      if (!core::SYMBOL_MAP.contains(s))
        return std::unexpected(configuration_error{
            key, error_code::out_of_range,
            std::format("An atomic element with name \"{}\" is not known to me", s)});

      auto ordinal = core::atom::from_symbol(s).Z;
      if (!distinct_species.contains(ordinal))
        return std::unexpected(configuration_error{
            key, error_code::bad_argument,
            std::format("The specified structure does not contain a site with species \"{}\"", s)});

      unique_species.insert(ordinal);
    }
    indices_t indices;
    for (auto i = 0; i < conf.size(); ++i)
      if (unique_species.contains(conf.at(i))) indices.insert(static_cast<std::size_t>(i));
    if (indices.empty())
      return std::unexpected(configuration_error{key, error_code::invalid_size,
                                                 "Cannot use an empty sublattice definition"});
    return indices;
  }

  inline result_t<indices_t> validate_sites(const site_spec& spec, const configuration_t& conf,
                                            std::optional<int> index = std::nullopt) {
    std::string key = index.has_value() ? std::format("composition.{}.sites", index.value())
                                        : "composition.sites";
    return std::visit(
        helpers::overloaded{[&](const std::vector<int>& indices) -> result_t<indices_t> {
                              return validate_indices(indices, conf, index);
                            },
                            [&](const std::vector<std::string>& species) -> result_t<indices_t> {
                              return validate_species_strings(species, conf, index);
                            },
                            [&](const std::string& specie) -> result_t<indices_t> {
                              return validate_species_strings(std::vector{specie}, conf, index);
                            },
                            [&](int) -> result_t<indices_t> {
                              return std::unexpected(
                                  configuration_error{key, error_code::type_error,
                                                      "Cannot specify a number to identify sites"});
                            }},
        spec);
  }

  inline result_t<sublattice_helper> validate_sublattice(const composition_spec& composition_spec,
                                                         const configuration_t& conf,
                                                         std::optional<int> index = std::nullopt) {
    std::string key
        = index.has_value() ? std::format("composition.{}", index.value()) : "composition";
    std::optional<indices_t> sites = std::nullopt;
    composition_t composition;
    for (const auto& [symbol, spec] : composition_spec) {
      if (symbol == "sites") {
        auto parsed_sites = validate_sites(spec, conf, index);
        if (!parsed_sites) return std::unexpected(std::move(parsed_sites).error());
        sites = *parsed_sites;
      }
      auto symbol_key = std::format("{}.{}", key, symbol);
      if (!core::SYMBOL_MAP.contains(symbol))
        return std::unexpected(
            configuration_error{symbol_key, error_code::bad_argument,
                                std::format("Unknown atomic species \"{}\"", symbol)});

      auto amount = std::visit(
          helpers::overloaded{
              [&](const std::vector<int>& indices) -> result_t<int> {
                return std::unexpected(
                    configuration_error{symbol_key, error_code::type_error,
                                        "Invalid type must be an integer number greater than 0"});
              },
              [&](const std::vector<std::string>& species) -> result_t<int> {
                return std::unexpected(
                    configuration_error{symbol_key, error_code::type_error,
                                        "Invalid type must be an integer number greater than 0"});
              },
              [&](const std::string& specie) -> result_t<int> {
                return std::unexpected(
                    configuration_error{symbol_key, error_code::type_error,
                                        "Invalid type must be an integer number greater than 0"});
              },
              [&](int v) -> result_t<int> {
                if (v < 1)
                  return std::unexpected(configuration_error{
                      symbol_key, error_code::out_of_range,
                      std::format("Number of atoms cannot be negative or zero got: {}", v)});
                return v;
              }},
          spec);
      if (!amount) return std::unexpected(std::move(amount).error());
      composition[core::atom::from_symbol(symbol).Z] = *amount;
    }

    if (composition.empty())
      return std::unexpected(configuration_error{key, error_code::invalid_size,
                                                 "You must specify at least a single species"});

    return sublattice_helper{sites, composition};
  }

  inline result_t<sublattice> validate_composition(sublattice_helper&& helper) {
    auto num_sites = helper.sites->size();
    auto values_view = helper.composition | std::ranges::views::values;
    auto distributed_atoms
        = std::accumulate(values_view.begin(), values_view.end(), std::size_t{0});
    if (distributed_atoms != num_sites)
      return std::unexpected(configuration_error{
          "composition", error_code::invalid_size,
          std::format("You tried to distribute {} atoms on a sublattice with {} sites",
                      distributed_atoms, num_sites)});
    return sublattice{std::move(helper.sites.value()), std::move(helper.composition)};
  }

  inline result_t<std::vector<sublattice_helper>> parse_sublattice_helpers(
      const composition_input& input, const configuration_t& conf) {
    return std::visit(
        helpers::overloaded{
            [&](const composition_spec& spec) -> result_t<std::vector<sublattice_helper>> {
              auto sublattice = validate_sublattice(spec, conf, std::nullopt);
              if (!sublattice) return std::unexpected(std::move(sublattice).error());
              return std::vector{std::move(*sublattice)};
            },
            [&](const std::vector<composition_spec>& spec)
                -> result_t<std::vector<sublattice_helper>> {
              if (spec.empty())
                return std::unexpected(
                    configuration_error{"composition", error_code::invalid_size,
                                        "You must specify one or more compositions"});
              std::vector<sublattice_helper> out;
              out.reserve(spec.size());
              for (std::size_t index = 0; index < spec.size(); ++index) {
                auto sublattice = validate_sublattice(spec[index], conf, std::make_optional(index));
                if (!sublattice) return std::unexpected(std::move(sublattice).error());
                out.push_back(std::move(*sublattice));
              }
              return out;
            }},
        input);
  }

  inline result_t<std::vector<sublattice>> validate_composition_input(
      const composition_input& input, const configuration_t& conf) {
    auto helper = parse_sublattice_helpers(input, conf);
    if (!helper) return std::unexpected(std::move(helper).error());
    auto [sublattices_with_site_spec, sublattices_without_site_spec] = helpers::partition(
        [](const auto& h) { return h.sites.has_value(); }, std::move(helper).value());

    std::vector<sublattice> out;
    out.reserve(sublattices_with_site_spec.size() + sublattices_without_site_spec.size());
    std::map<std::size_t, std::size_t> site_owner;
    std::size_t index = 0;
    for (auto&& sl : sublattices_with_site_spec) {
      for (const auto& site : sl.sites.value()) {
        auto [it, inserted] = site_owner.try_emplace(site, index);
        if (!inserted) {
          const std::size_t other_index = it->second;
          return std::unexpected(configuration_error{
              std::format("composition.{}", index), error_code::bad_argument,
              std::format(
                  "the atomic site {} is contained in at least two sublattices ({} and {}). "
                  "sublattices must be mutually exclusive",
                  site, index, other_index)});
        }
      }
      auto validated = validate_composition(std::move(sl));
      if (!validated) return std::unexpected(std::move(validated).error());
      out.emplace_back(std::move(validated).value());
      index++;
    }
    if (!sublattices_without_site_spec.empty()) {
      if (sublattices_without_site_spec.size() > 1)
        return std::unexpected(configuration_error{
            "composition",
            error_code::bad_argument,
            "If you do not specify \"sites\" with more than one sublattice you may omit it only "
            "for "
            "one composition definition",
        });
      core::sorted_vector<std::size_t> remaining(
          std::ranges::views::iota(std::size_t{0}, conf.size())
          | std::ranges::views::filter(
              [&](const auto site) { return !site_owner.contains(site); }));
      if (remaining.empty())
        return std::unexpected(configuration_error{
            "composition",
            error_code::invalid_size,
            "The sublattice definition without a site specification maps to an empty set of sites",
        });
      auto validated = validate_composition(sublattice_helper{
          .sites = std::move(remaining),
          .composition = std::move(sublattices_without_site_spec.front().composition)});
      if (!validated) return std::unexpected(std::move(validated).error());
      out.emplace_back(std::move(validated).value());
    }
    return out;
  }

}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_COMPOSITION_IMPL_HPP
