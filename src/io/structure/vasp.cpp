
#include <expected>

#include "sqsgen/core/helpers/as.h"
#include "sqsgen/core/helpers/misc.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::io {

  namespace helpers = sqsgen::core::helpers;
  template <class T> struct structure_adapter_<T, structure_format::poscar> {
    using row_t = Eigen::Vector3<T>;
    using tokens_t = std::vector<std::string_view>;
    static std::string format(core::structure<T> const& structure) {
      auto filtered = structure.without_vacancies();
      // one coordinate line holds about 72 characters, to be safe we multiply by two
      constexpr absl::string_view row_format = "%23.16f %23.16f %23.16f";
      auto sorted = filtered.sorted([](auto&& a, auto&& b) { return a.specie < b.specie; });
      auto unique_species = core::sorted_vector<specie_t>{sorted.species};
      auto num_species = core::count_species(sorted.species);

      std::string result;
      result.reserve(filtered.size() * 144);

      const auto println
          = [&result](std::string const& line) { result.append(format_string("%s\n", line)); };
      const auto format_row
          = [&](auto&& row) { return format_string(row_format, row(0), row(1), row(2)); };

      const auto z_to_symbol = [](auto&& z) { return core::atom::from_z(z).symbol; };
      // generate first line
      std::string composition_string
          = detail::join(unique_species | views::transform([&](auto&& s) {
                           return format_string("%s%i", z_to_symbol(s), num_species[s]);
                         }),
                         "");
      println(composition_string);
      println("1.0");
      for (auto&& row : sorted.lattice.rowwise()) println(format_row(row));

      std::string species_list = detail::join(unique_species | views::transform(z_to_symbol), " ");
      println(species_list);

      std::string species_amount = detail::join(
          views::values(num_species)
              | views::transform([](auto&& amount) { return format_string("%i", amount); }),
          " ");
      println(species_amount);
      println("Direct");
      for (auto const& site : sorted.sites())
        println(format_string("%s %s", format_row(site.frac_coords), z_to_symbol(site.specie)));
      result.shrink_to_fit();
      return result;
    }

    static std::expected<core::structure<T>, parser_error> from_string(std::string const& input) {
      using result_t = std::expected<core::structure<T>, parser_error>;
      std::istringstream ss(input);
      std::string_view contents{ss.view()};

      std::map<int, tokens_t> lines = helpers::as<std::map>{}(
          detail::split(contents, "\n") | views::transform([lineno = 0](auto&& s) mutable {
            return std::make_pair(lineno++, detail::split(s, " \t"));
          }));

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
            parser_error{error_code::out_of_range, format_string("Line %i not found", l)});
      };

      const auto get_row = [&](int l) -> std::expected<row_t, parser_error> {
        return get_line(l).and_then(parse_row);
      };

      auto lattice = get_row(2).combine(get_row(3)).combine(get_row(4)).and_then([&](auto&& m) {
        auto [a, b, c] = m;
        lattice_t<T> l;
        l.row(0) = a;
        l.row(1) = b;
        l.row(2) = c;
        return get_line(1)
            .and_then(parse_scaling)
            .template collapse<lattice_t<T>>(
                [&](T&& scaling) -> parse_result<lattice_t<T>> {
                  if (scaling < 0)
                    return lattice_t<T>{l * std::pow(-scaling / l.determinant(), 1.0 / 3.0)};
                  else
                    return lattice_t<T>{l * scaling};
                },
                [&](row_t&& s) -> parse_result<lattice_t<T>> {
                  lattice_t<T> ll(std::move(l));
                  ll.row(0) *= s(0);
                  ll.row(1) *= s(1);
                  ll.row(2) *= s(2);
                  return ll;
                });
      });

      auto species
          = get_line(5)
                .and_then(parse_species_decl)
                .combine(get_line(6).and_then(parse_species_amount))
                .and_then([&](auto&& specs_and_decls) -> parse_result<configuration_t> {
                  auto&& [species, amounts] = specs_and_decls;
                  if (species.size() != amounts.size())
                    return parse_error::from_msg<KEY_NONE, CODE_OUT_OF_RANGE>(
                        "Species and number of ions per species must have the same length");
                  configuration_t conf;
                  conf.reserve(sum(amounts));
                  for (auto i : range(species.size()))
                    for (auto _ : range(amounts[i])) conf.push_back(species[i]);
                  return conf;
                });

      return lattice.combine(std::move(species))
          .combine(get_coords_line())
          .and_then([&](auto&& data) -> result_t {
            auto&& [lattice, configuration, start_line_number] = data;
            coords_t<T> coords(configuration.size(), 3);
            for (auto i : range(configuration.size())) {
              auto fc = get_row(start_line_number + i);
              if (fc.ok())
                coords.row(i) = fc.result();
              else
                return fc.error();
            }
            return core::structure<T>{lattice, coords, configuration};
          });
    }

  private:
    static parse_result<configuration_t> parse_species_decl(tokens_t const& tokens) {
      using result_t = parse_result<configuration_t>;
      auto result = core::helpers::fold_left(
          tokens, result_t{configuration_t{}}, [](auto&& conf_result, auto&& symbol) -> result_t {
            if (conf_result.failed()) return conf_result;
            auto sym = std::string{symbol};
            if (!core::SYMBOL_MAP.contains(sym))
              return parse_error::from_msg<KEY_NONE, CODE_OUT_OF_RANGE>(
                  format_string("Unknown element \"%s\"", symbol));
            else {
              auto species = conf_result.result();
              species.push_back(core::atom::from_symbol(sym).Z);
              return {species};
            }
          });
      if (result.ok() && result.result().size() == 0)
        return parse_error::from_msg<KEY_NONE, CODE_OUT_OF_RANGE>(
            "Could not find and species. sqsgen has no mechanism to detect the species from "
            "POTCAR. For me the 'optional' species declaration line is mandatory. Please read "
            "\"https://www.vasp.at/wiki/index.php/POSCAR\" for further information.");
      else
        return result;
    }

    static parse_result<std::vector<int>> parse_species_amount(tokens_t const& tokens) {
      using result_t = parse_result<std::vector<int>>;
      auto result = core::helpers::fold_left(tokens, result_t{std::vector<int>{}},
                                             [](auto&& result, auto&& token) -> result_t {
                                               if (result.ok()) {
                                                 auto amount = detail::parse_number<int>(token);
                                                 if (amount.ok()) {
                                                   auto amounts = result.result();
                                                   amounts.push_back(amount.result());
                                                   return result_t{amounts};
                                                 } else
                                                   return amount.error();
                                               } else
                                                 return result;
                                             });
      if (result.ok() && result.result().size() == 0)
        return parse_error::from_msg<KEY_NONE, CODE_OUT_OF_RANGE>(
            "No number of ions per species found");
      return result;
    }

    static std::expected<row_t, parser_error> parse_row(tokens_t const& tokens) {
      if (tokens.size() >= 3) {
        auto sa = detail::parse_number<T>(tokens[0]);
        if (!sa) return std::unexpected(std::move(sa).error());
        auto sb = detail::parse_number<T>(tokens[1]);
        if (!sb) return std::unexpected(std::move(sb).error());
        auto sc = detail::parse_number<T>(tokens[2]);
        if (!sc) return std::unexpected(std::move(sc).error());
        return row_t{*sa, *sb, *sc};
      } else
        return std::unexpected(parser_error{
            error_code::out_of_range,
            format_string("A vector row must contain three entries, but got %u", tokens.size())});
    }

    static parse_result<T, row_t> parse_scaling(tokens_t const& tokens) {
      using result_t = parse_result<T, row_t>;
      if (tokens.size() == 1)
        return detail::parse_number<T>(tokens.front()).and_then([](auto&& scale) -> result_t {
          return scale;
        });
      if (tokens.size() == 3)
        return parse_row(tokens).and_then([](auto&& scale) -> result_t { return scale; });
      return parse_error::from_msg<KEY_NONE, CODE_BAD_ARGUMENT>(format_string(
          "Scaling must be a single float or a triplet of floats, but got %u", tokens.size()));
    }
  };
}  // namespace sqsgen::io
