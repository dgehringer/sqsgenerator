

// #include "sqsgen/configuration/composition.h"

#include "sqsgen/configuration/composition.h"

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <numeric>
#include <optional>
#include <ranges>
#include <variant>
#include <vector>

#include "sqsgen/core/atom.h"
#include "sqsgen/core/helpers/string.h"
#include "sqsgen/core/helpers/variant.h"
#include "sqsgen/core/permutation.h"
#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace ranges = std::ranges;
  namespace core = sqsgen::core;
  namespace helpers = sqsgen::core::helpers;

  using indices_t = core::sorted_vector<std::size_t>;

  struct sublattice_helper {
    std::optional<indices_t> sites = std::nullopt;
    composition_t composition;
  };

  result_t<indices_t> validate_indices(std::vector<int> const& indices, configuration_t const& conf,
                                       std::optional<int> index = std::nullopt) {
    std::string key = index.has_value()
                          ? helpers::format_string("composition.%i.sites", index.value())
                          : "composition.sites";
    indices_t parsed;
    for (auto index : indices)
      if (0 <= index && index < conf.size())
        parsed.insert(static_cast<size_t>(index));
      else
        return std::unexpected(configuration_error{
            key, error_code::out_of_range,
            helpers::format_string("Invalid index %i, the specified structure contains %i sites",
                                   index, conf.size())});
    if (parsed.empty())
      return std::unexpected(
          configuration_error{key, error_code::invalid_size, "You cannot specify an empty slice"});

    return parsed;
  }

  result_t<indices_t> validate_species_strings(std::vector<std::string> const& species,
                                               configuration_t const& conf,
                                               std::optional<int> index = std::nullopt) {
    std::string key = index.has_value()
                          ? helpers::format_string("composition.%i.sites", index.value())
                          : "composition.sites";
    auto distinct_species = core::count_species(conf);
    core::sorted_vector<specie_t> unique_species;
    for (auto s : species) {
      if (!core::SYMBOL_MAP.contains(s))
        return std::unexpected(configuration_error{
            key, error_code::out_of_range,
            helpers::format_string("An atomic element with name \"%s\" is not known to me", s)});

      auto ordinal = core::atom::from_symbol(s).Z;
      if (!distinct_species.contains(ordinal))
        return std::unexpected(configuration_error{
            key, error_code::bad_argument,
            helpers::format_string(
                "The specified structure does not contain a site with species \"%s\"", s)});

      unique_species.insert(ordinal);
    }
    indices_t indices;
    for (auto i = 0; i < conf.size(); ++i)
      if (unique_species.contains(conf.at(i))) indices.insert(static_cast<std::size_t>(i));
    if (indices.size() == 0)
      return std::unexpected(configuration_error{key, error_code::invalid_size,
                                                 "Cannot use an empty sublattice definition"});
    return indices;
  }

  result_t<indices_t> validate_sites(const site_spec& spec, const configuration_t& conf,
                                     std::optional<int> index = std::nullopt) {
    std::string key = index.has_value()
                          ? helpers::format_string("composition.%i.sites", index.value())
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

  result_t<sublattice_helper> validate_sublattice(const composition_spec& composition_spec,
                                                  const configuration_t& conf,
                                                  std::optional<int> index = std::nullopt) {
    std::string key = index.has_value() ? helpers::format_string("composition.%i", index.value())
                                        : "composition";
    std::optional<indices_t> sites = std::nullopt;
    composition_t composition;
    for (const auto& [symbol, spec] : composition_spec) {
      if (symbol == "sites") {
        auto parsed_sites = validate_sites(spec, conf, index);
        if (!parsed_sites) return std::unexpected(std::move(parsed_sites).error());
        sites = *parsed_sites;
      }
      auto symbol_key = helpers::format_string("%s.%s", key, symbol);
      if (!core::SYMBOL_MAP.contains(symbol))
        return std::unexpected(
            configuration_error{symbol_key, error_code::bad_argument,
                                helpers::format_string("Unknown atomic species \"%s\"", symbol)});

      auto amount
          = std::visit(helpers::overloaded{
                           [&](const std::vector<int>& indices) -> result_t<int> {
                             return std::unexpected(configuration_error{
                                 symbol_key, error_code::type_error,
                                 "Invalid type must be an integer number greater than 0"});
                           },
                           [&](const std::vector<std::string>& species) -> result_t<int> {
                             return std::unexpected(configuration_error{
                                 symbol_key, error_code::type_error,
                                 "Invalid type must be an integer number greater than 0"});
                           },
                           [&](const std::string& specie) -> result_t<int> {
                             return std::unexpected(configuration_error{
                                 symbol_key, error_code::type_error,
                                 "Invalid type must be an integer number greater than 0"});
                           },
                           [&](int v) -> result_t<int> {
                             if (v < 1)
                               return std::unexpected(configuration_error{
                                   symbol_key, error_code::out_of_range,
                                   helpers::format_string(
                                       "Number of atoms cannot be negative or zero got: %i", v)});
                             return v;
                           }},
                       spec);
      if (!amount) return std::unexpected(std::move(amount).error());
      composition[core::atom::from_symbol(symbol).Z] = *amount;
    }

    if (composition.size())
      return std::unexpected(configuration_error{key, error_code::invalid_size,
                                                 "You must specify at least a single species"});

    return sublattice_helper{sites, composition};
  }

  result_t<std::vector<sublattice>> validate_composition_input(const composition_input& input,
                                                               const configuration_t& conf) {
    auto helper = std::visit(
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
              for (auto index = 0; index < spec.size(); ++index) {
                auto sublattice
                    = validate_sublattice(spec.at(index), conf, std::make_optional(index));
                if (!sublattice) return std::unexpected(std::move(sublattice).error());
                out.push_back(std::move(*sublattice));
              }
              return out;
            }},
        input);
    // in case no sites have been specified we use up the whole structure conf.size()
    if (!helper) return std::unexpected(std::move(helper).error());
    auto sublattice_helpers = std::move(helper.value());

    auto sublattices_without_site_spec
        = std::ranges::count_if(helper.value(), [](const auto& h) { return !h.sites.has_value(); });
    if (sublattices_without_site_spec > 1)
      return std::unexpected(configuration_error{
          "composition",
          error_code::bad_argument,
          "If you do not specify \"sites\" with more than one sublattice you may omit it only for "
          "one composition definition",
      });

    std::vector

        sublattice out{helper->sites.has_value() ? *(helper->sites)
                                                 : indices_t(std::ranges::views::iota(conf.size())),
                       helper->composition};
    auto num_sites = out.sites.size();
    auto values_view = helper->composition | std::ranges::views::values;
    auto distributed_atoms = std::accumulate(values_view.begin(), values_view.end(), 0);
    if (distributed_atoms != num_sites)
      return std::unexpected(configuration_error{
          "composition", error_code::invalid_size,
          helpers::format_string("You tried to distribute %i atoms on a sublattice with %u sites",
                                 distributed_atoms, num_sites)});
    return out;
  }

}  // namespace sqsgen::configuration
