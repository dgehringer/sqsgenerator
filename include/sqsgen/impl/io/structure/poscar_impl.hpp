//
// Created by Dominik Gehringer on 04.03.25.
//

#ifndef SQSGEN_IMPL_IO_STRUCTURE_POSCAR_IMPL_HPP
#define SQSGEN_IMPL_IO_STRUCTURE_POSCAR_IMPL_HPP

#include <algorithm>
#include <cmath>
#include <expected>
#include <format>
#include <map>
#include <numeric>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

#include "sqsgen/core/atom.h"
#include "sqsgen/core/helpers/expected.h"
#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/core/helpers/variant.h"
#include "sqsgen/core/permutation.h"
#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::io {

  namespace views = std::ranges::views;
  namespace helpers = sqsgen::core::helpers;

  template <class T> using row_t = Eigen::Vector3<T>;
  using tokens_t = std::vector<std::string_view>;

  inline std::expected<configuration_t, parser_error> parse_species_decl(tokens_t const& tokens) {
    configuration_t species{};
    for (const auto& symbol : tokens) {
      auto sym = std::string{symbol};
      if (!core::SYMBOL_MAP.contains(sym))
        return std::unexpected(
            parser_error{error_code::out_of_range, std::format("Unknown element \"{}\"", symbol)});
      species.push_back(core::atom::from_symbol(sym).Z);
    }

    if (species.size() == 0)
      return std::unexpected(parser_error{
          error_code::out_of_range,
          "Could not find and species. sqsgen has no mechanism to detect the species from "
          "POTCAR. For me the 'optional' species declaration line is mandatory. Please read "
          "\"https://www.vasp.at/wiki/index.php/POSCAR\" for further information."});
    return species;
  }

  inline std::expected<std::vector<int>, parser_error> parse_species_amount(
      tokens_t const& tokens) {
    std::vector<int> result;
    for (const auto& token : tokens) {
      auto amount = detail::parse_number<int>(token);
      if (!amount) return std::unexpected(std::move(amount).error());
      result.push_back(*amount);
    }

    auto number_of_ions = std::accumulate(result.begin(), result.end(), 0);
    if (number_of_ions < 1)
      return std::unexpected(
          parser_error{error_code::invalid_size, "No ions found in the number specification"});
    return result;
  }

  template <class T> std::expected<row_t<T>, parser_error> parse_row(tokens_t const& tokens) {
    if (tokens.size() >= 3) {
      return helpers::combine(detail::parse_number<T>(tokens[0]),
                              detail::parse_number<T>(tokens[1]),
                              detail::parse_number<T>(tokens[2]))
          .transform([](auto&& scale) {
            const auto [sa, sb, sc] = scale;
            return row_t<T>{sa, sb, sc};
          });
    } else
      return std::unexpected(parser_error{
          error_code::out_of_range,
          std::format("A vector row must contain three entries, but got {}", tokens.size())});
  }

  template <class T>
  std::expected<std::variant<T, row_t<T>>, parser_error> parse_scaling(tokens_t const& tokens) {
    if (tokens.size() == 1)
      return detail::parse_number<T>(tokens.front())
          .transform([](auto&& scale) -> std::variant<T, row_t<T>> { return scale; });
    if (tokens.size() == 3)
      return parse_row<T>(tokens).transform(
          [](auto&& scale) -> std::variant<T, row_t<T>> { return scale; });
    return std::unexpected(parser_error{
        error_code::invalid_size,
        std::format("Scaling must be a single float or a triplet of floats, but got {}",
                    tokens.size())});
  }

  template <class T> std::string structure_adapter<T, structure_format::poscar>::format(
      const core::structure<T>& structure) {
    auto filtered = structure.without_vacancies();
    auto sorted = filtered.sorted([](auto&& a, auto&& b) { return a.specie < b.specie; });
    auto unique_species = core::sorted_vector<specie_t>{sorted.species};
    auto num_species = core::count_species(sorted.species);

    std::string result;
    result.reserve(filtered.size() * 144);

    const auto println
        = [&result](std::string const& line) { result.append(std::format("{}\n", line)); };
    // one coordinate line holds about 72 characters
    const auto format_row = [&](auto&& row) {
      return std::format("{:23.16f} {:23.16f} {:23.16f}", row(0), row(1), row(2));
    };

    const auto z_to_symbol = [](auto&& z) { return core::atom::from_z(z).symbol; };
    // generate first line
    std::string composition_string
        = detail::join(unique_species | views::transform([&](auto&& s) {
                         return std::format("{}{}", z_to_symbol(s), num_species[s]);
                       }),
                       "");
    println(composition_string);
    println("1.0");
    for (auto&& row : sorted.lattice.rowwise()) println(format_row(row));

    std::string species_list = detail::join(unique_species | views::transform(z_to_symbol), " ");
    println(species_list);

    std::string species_amount = detail::join(
        views::values(num_species)
            | views::transform([](auto&& amount) { return std::format("{}", amount); }),
        " ");
    println(species_amount);
    println("Direct");
    for (auto const& site : sorted.sites())
      println(std::format("{} {}", format_row(site.frac_coords), z_to_symbol(site.specie)));
    result.shrink_to_fit();
    return result;
  }

  template <class T> std::expected<core::structure<T>, parser_error>
  structure_adapter<T, structure_format::poscar>::read(std::string const& input) {
    std::istringstream ss(input);
    std::string_view contents{ss.view()};

    auto lines = detail::split(contents, "\n") | views::transform([lineno = 0](auto&& s) mutable {
                   return std::make_pair(lineno++, detail::split(s, " \t"));
                 })
                 | std::ranges::to<std::map<int, tokens_t>>();

    // find the line which start with "Direct" or "Cartesian"
    const auto get_coords_line = [&]() -> std::expected<int, parser_error> {
      for (const auto& [lineno, tokens] : lines)
        if (!tokens.empty())
          if (helpers::to_uppercase(tokens.front()) == "DIRECT"
              || helpers::to_uppercase(tokens.front()) == "CARTESIAN")
            return {lineno + 1};
      return std::unexpected(
          parser_error{error_code::not_found, "Could not find the line with the coordinates"});
    };
    const auto get_line = [&](int l) -> std::expected<tokens_t, parser_error> {
      if (lines.contains(l)) return {lines[l]};
      return std::unexpected(
          parser_error{error_code::out_of_range, std::format("Line {} not found", l)});
    };

    const auto get_row = [&](int l) -> std::expected<row_t<T>, parser_error> {
      return get_line(l).and_then(parse_row<T>);
    };

    std::expected<lattice_t<T>, parser_error> lattice
        = helpers::combine(get_row(2), get_row(3), get_row(4))
              .and_then([&](std::tuple<row_t<T>, row_t<T>, row_t<T>>&& m)
                            -> std::expected<lattice_t<T>, parser_error> {
                auto [a, b, c] = m;
                lattice_t<T> l;
                l.row(0) = a;
                l.row(1) = b;
                l.row(2) = c;
                return get_line(1)
                    .and_then(parse_scaling<T>)
                    .transform([&l](std::variant<T, row_t<T>>&& lattice_scaling) {
                      return std::visit(
                          helpers::overloaded{
                              [&l](T&& scaling) -> lattice_t<T> {
                                if (scaling < 0)
                                  return lattice_t<T>{
                                      l * std::pow(-scaling / l.determinant(), 1.0 / 3.0)};
                                else
                                  return lattice_t<T>{l * scaling};
                              },
                              [&](row_t<T>&& s) -> lattice_t<T> {
                                lattice_t<T> ll(std::move(l));
                                ll.row(0) *= s(0);
                                ll.row(1) *= s(1);
                                ll.row(2) *= s(2);
                                return ll;
                              }},
                          std::move(lattice_scaling));
                    });
              });

    std::expected<configuration_t, parser_error> species
        = helpers::combine(get_line(5).and_then(parse_species_decl),
                           get_line(6).and_then(parse_species_amount))
              .and_then(
                  [&](const auto& specs_and_decls) -> std::expected<configuration_t, parser_error> {
                    auto [species, amounts] = specs_and_decls;
                    if (species.size() != amounts.size())
                      return std::unexpected(parser_error{
                          error_code::out_of_range,
                          "Species and number of ions per species must have the same length"});

                    configuration_t conf;
                    conf.reserve(std::accumulate(amounts.begin(), amounts.end(), 0));
                    for (std::size_t i = 0; i < species.size(); ++i)
                      conf.insert(conf.end(), amounts[i], species[i]);
                    return conf;
                  });

    return helpers::combine(std::move(lattice), std::move(species), get_coords_line())
        .and_then([&](const auto& data) -> std::expected<core::structure<T>, parser_error> {
          const auto& [lattice, configuration, start_line_number] = data;
          coords_t<T> coords(configuration.size(), 3);
          for (std::size_t i = 0; i < configuration.size(); ++i) {
            auto fc = get_row(start_line_number + i);
            if (!fc)
              return std::unexpected(std::move(fc).error());
            else
              coords.row(i) = std::move(fc).value();
          }
          return core::structure<T>(lattice, coords, configuration);
        });
  }

}  // namespace sqsgen::io

#endif  // SQSGEN_IMPL_IO_STRUCTURE_POSCAR_IMPL_HPP
