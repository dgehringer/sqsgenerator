//
// Created by Dominik Gehringer on 04.03.25.
//

#ifndef SQSGEN_IO_STRUCTURE_H
#define SQSGEN_IO_STRUCTURE_H

#include <map>
#include <ranges>
#include <regex>

#include "sqsgen/core/structure.h"

namespace sqsgen::io {
  namespace ranges = std::ranges;
  namespace views = ranges::views;

  namespace detail {

    template <ranges::range Range>
      requires std::is_same_v<ranges::range_value_t<Range>, std::string>
    std::string join(Range&& crumbs, std::string const& delimiter = " ") {
      auto csize = ranges::size(crumbs);
      if (csize == 0) return "";
      std::string result;
      result.reserve(
          core::helpers::sum(crumbs | views::transform([](auto&& s) { return s.size(); }))
          + (csize - 1) * delimiter.size());
      for (auto it = ranges::begin(crumbs); it != ranges::end(crumbs); ++it) {
        if (it != ranges::begin(crumbs)) result.append(delimiter);
        result.append(*it);
      }
      result.shrink_to_fit();
      return result;
    }

    template <class T> std::array<T, 3> lengths(lattice_t<T> const& m) {
      return {
          m.row(0).norm(),
          m.row(1).norm(),
          m.row(2).norm(),
      };
    }

    template <class T> T clip(T val, T lower = -1.0, T upper = 1.0) {
      if (val < lower) return lower;
      if (val > upper) return upper;
      return val;
    }

    template <class T> std::array<T, 3> angles(lattice_t<T> const& m) {
      const auto l = lengths(m);
      const auto angle = [&](auto dim) -> T {
        auto i = (dim + 1) % std::size(l);
        auto j = (dim + 2) % std::size(l);
        return std::acos(clip(m.row(i).dot(m.row(j)) / (l[i] * l[j]))) * 180.0 / M_PI;
      };
      return {angle(0), angle(1), angle(2)};
    }

    inline std::vector<std::string> resplit(const std::string& s,
                                            const std::regex& sep_regex = std::regex{"\\s+"}) {
      std::sregex_token_iterator iter(s.begin(), s.end(), sep_regex, -1);
      std::sregex_token_iterator end;
      return as<std::vector>{}(std::vector<std::string>{iter, end}
                               | views::filter([](auto&& match) { return !match.empty(); }));
    }

    template <class T> parse_result<T> parse_number(std::string const& input) {
      try {
        if constexpr (std::is_same_v<T, float>) {
          return std::stof(input);
        }
        if constexpr (std::is_same_v<T, double>) {
          return std::stod(input);
        }
        if constexpr (std::is_same_v<T, int>) {
          return std::stoi(input);
        }
        if constexpr (std::is_same_v<T, long>) {
          return std::stol(input);
        }
        if constexpr (std::is_same_v<T, unsigned long>) {
          return std::stoul(input);
        }
      } catch (const std::invalid_argument& e) {
        return parse_error::from_msg<KEY_NONE, CODE_BAD_ARGUMENT>(e.what());
      } catch (const std::out_of_range& e) {
        return parse_error::from_msg<KEY_NONE, CODE_OUT_OF_RANGE>(e.what());
      }
      return parse_error::from_msg<KEY_NONE, CODE_UNKNOWN>("Unknown error");
    }
  }  // namespace detail

  template <ranges::range Range, class GroupKeyFn>
  auto group_by(Range&& range, GroupKeyFn&& group_key_fn) {
    std::map<std::invoke_result_t<GroupKeyFn, ranges::range_reference_t<Range>>,
             std::vector<ranges::range_reference_t<Range>>>
        groups;
    for (auto&& element : range) {
      auto key = group_key_fn(element);
      groups[key].push_back(element);
    }
    return groups;
  }

  enum StructureFormat {
    STRUCTURE_FORMAT_JSON_PYMATGEN = 0,
    STRUCTURE_FORMAT_JSON_ASE = 1,
    STRUCTURE_FORMAT_CIF = 2,
    STRUCTURE_FORMAT_POSCAR = 3,
  };

  template <class, StructureFormat> struct structure_adapter {};

  template <class T> struct structure_adapter<T, STRUCTURE_FORMAT_JSON_ASE> {
    static nlohmann::json _format_array(std::string const& dtype, auto const& shape,
                                        auto const& array) {
      return nlohmann::json{
          {"array",
           {{"__ndarray__",
             {shape, dtype, std::vector<T>(array.data(), array.data() + array.size())}}}},
      };
    }

    static nlohmann::json format_json(core::structure<T> const& structure) {
      constexpr std::string dtype = "float64";
      auto cell = _format_array("float64", std::array{3, 3}, structure.lattice);
      cell["__ase_objtype__"] = "cell";

      matrix_t<T> positions(structure.size(), 3);
      for (std::size_t i = 0; i < structure.size(); ++i)
        positions.row(i) = structure.frac_coords.row(i) * structure.lattice;

      return nlohmann::json{
          {"cell", cell},
          {"unique_id", structure.uuid()},
          {"user", "sqsgen"},
          {"mtime", years_since_2000()},
          {"ctime", years_since_2000()},
          {"pbc", _format_array("bool", std::array{3}, structure.pbc)},
          {"positions",
           _format_array(dtype, std::array{structure.size(), std::size_t{3}}, positions)}};
    }

    static std::string format(core::structure<T> const& structure) {
      return format_json(structure).dump();
    }

  private:
    static double years_since_2000() {
      // January 1, 2000 00:00:00 UTC
      std::tm timeinfo = {};
      timeinfo.tm_year = 100;  // Year since 1900
      timeinfo.tm_mon = 0;     // Month [0, 11]
      timeinfo.tm_mday = 1;    // Day of the month [1, 31]

      // Convert std::tm to time_t
      std::time_t time_t_epoch = std::mktime(&timeinfo);

      // Convert time_t to std::chrono::system_clock::time_point
      auto time_point_2000 = std::chrono::system_clock::from_time_t(time_t_epoch);
      auto now = std::chrono::system_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - time_point_2000);
      double seconds_per_year = 31557600.0;  // Average number of seconds per year
      return static_cast<double>(duration.count()) / seconds_per_year;
    }
  };

  template <class T> struct structure_adapter<T, STRUCTURE_FORMAT_POSCAR> {
    using row_t = Eigen::Vector3<T>;
    using tokens_t = std::vector<std::string>;
    static std::string format(core::structure<T> const& structure) {
      // one coordinate line holds about 72 characters, to be safe we multiply by two
      constexpr std::string_view row_format = "{:22.16f} {:22.16f} {:22.16f}";
      auto sorted = structure.sorted([](auto&& a, auto&& b) { return a.specie < b.specie; });
      auto unique_species = core::helpers::sorted_vector<specie_t>{sorted.species};
      auto num_species = core::count_species(sorted.species);

      std::string result;
      result.reserve(structure.size() * 144);

      const auto println
          = [&result](std::string const& line) { result.append(std::format("{}\n", line)); };
      const auto format_row
          = [&](auto&& row) { return std::format(row_format, row(0), row(1), row(2)); };

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

    static parse_result<core::structure<T>> from_string(std::string const& input) {
      using result_t = parse_result<core::structure<T>>;
      std::istringstream ss(input);
      std::string line;
      int lineno{0};
      std::map<int, tokens_t> lines;
      while (std::getline(ss, line)) lines.emplace(lineno++, detail::resplit(line));

      // find the line which start with "Direct" or "Cartesian"
      const auto get_coords_line = [&]() -> parse_result<int> {
        for (const auto& [lineno, tokens] : lines)
          if (!tokens.empty())
            if (to_uppercase(tokens.front()) == "DIRECT"
                || to_uppercase(tokens.front()) == "CARTESIAN")
              return {lineno + 1};
        return parse_error::from_msg<KEY_NONE, CODE_NOT_FOUND>(
            "Could not find the line with the coordinates");
      };
      const auto get_line = [&](int l) -> parse_result<tokens_t> {
        if (lines.contains(l)) return {lines[l]};
        return parse_error::from_msg<KEY_NONE, CODE_OUT_OF_RANGE>(
            std::format("Line {} not found", l));
      };

      const auto get_row
          = [&](int l) -> parse_result<row_t> { return get_line(l).and_then(parse_row); };

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
            if (!core::SYMBOL_MAP.contains(symbol))
              return parse_error::from_msg<KEY_NONE, CODE_OUT_OF_RANGE>(
                  std::format("Unknown element {}", symbol));
            else {
              auto species = conf_result.result();
              species.push_back(core::atom::from_symbol(symbol).Z);
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

    static parse_result<row_t> parse_row(tokens_t const& tokens) {
      if (tokens.size() >= 3)
        return detail::parse_number<T>(tokens[0])
            .combine(detail::parse_number<T>(tokens[1]))
            .combine(detail::parse_number<T>(tokens[2]))
            .and_then([](auto&& scale) -> parse_result<row_t> {
              const auto [sa, sb, sc] = scale;
              return row_t{sa, sb, sc};
            });
      else
        return parse_error::from_msg<KEY_NONE, CODE_BAD_ARGUMENT>(
            std::format("A vector row must contain three entries, but got {}", tokens.size()));
    }

    static parse_result<T, row_t> parse_scaling(tokens_t const& tokens) {
      using result_t = parse_result<T, row_t>;
      if (tokens.size() == 1)
        return detail::parse_number<T>(tokens.front()).and_then([](auto&& scale) -> result_t {
          return scale;
        });
      if (tokens.size() == 3)
        return parse_row(tokens).and_then([](auto&& scale) -> result_t { return scale; });
      return parse_error::from_msg<KEY_NONE, CODE_BAD_ARGUMENT>(std::format(
          "Scaling must be a single float or a triplet of floats, but got {}", tokens.size()));
    }
  };

  template <class T> struct structure_adapter<T, STRUCTURE_FORMAT_JSON_PYMATGEN> {
    static nlohmann::json format_json(core::structure<T> const& structure) {
      const auto [a, b, c] = detail::lengths<T>(structure.lattice);
      const auto [alpha, beta, gamma] = detail::angles<T>(structure.lattice);
      nlohmann::json lattice = {{"matrix", structure.lattice},
                                {"pbc", structure.pbc},
                                {"a", a},
                                {"b", b},
                                {"c", c},
                                {"volume", std::abs(structure.lattice.determinant())},
                                {"alpha", alpha},
                                {"beta", beta},
                                {"gamma", gamma}};
      auto sites = nlohmann::json::array();
      ranges::for_each(structure.sites(),
                       [&](auto&& site) { sites.push_back(format_site(site, structure.lattice)); });
      return {{"properties", nlohmann::json::object()},
              {"lattice", lattice},
              {"@module", "pymatgen.core.structure"},
              {"@class", "Structure"},
              {"charge", 0},
              {"sites", sites}};
    }

    static std::string format(core::structure<T> const& structure) {
      return format_json(structure).dump();
    }

    static parse_result<core::structure<T>> from_json(std::string const& json) {
      using result_t = parse_result<core::structure<T>>;
      nlohmann::json document = nlohmann::json::parse(json);
      if (!document.contains("lattice"))
        return parse_error::from_msg<"lattice", CODE_NOT_FOUND>("missing key \"lattice\"");
      if (!document.contains("lattice"))
        return parse_error::from_msg<"sites", CODE_NOT_FOUND>("missing key \"sites\"");
      auto sites = document.at("sites");
      if (!sites.is_array())
        return parse_error::from_msg<"sites", CODE_TYPE_ERROR>("\"sites\" must be an array");
      return get_as<"matrix", lattice_t<T>>(document.at("lattice"))
          .and_then([&](auto&& lattice) -> result_t {
            auto num_atoms = sites.size();
            configuration_t species;
            species.reserve(num_atoms);
            coords_t<T> frac_coords(num_atoms, 3);
            auto index{0};
            for (auto site_json : sites) {
              auto site_result = parse_site(site_json);
              if (site_result.failed()) return site_result.error();
              auto [specie, fc] = site_result.result();
              species.push_back(specie);
              auto [a, b, c] = fc;
              frac_coords.row(index++) = Eigen::Vector3<T>{a, b, c};
            }
            return core::structure<T>(lattice, frac_coords, species);
          });
    }

  private:
    static parse_result<std::tuple<specie_t, std::array<T, 3>>> parse_site(
        nlohmann::json const& site_json) {
      using result_t = parse_result<std::tuple<specie_t, std::array<T, 3>>>;
      if (!site_json.contains("species"))
        return parse_error::from_msg<"species", CODE_NOT_FOUND>(
            "site is missing the key \"species\"");
      auto species = site_json.at("species");
      if (!species.is_array())
        return parse_error::from_msg<"species", CODE_TYPE_ERROR>("site is not an array");
      if (species.size() != 1)
        return parse_error::from_msg<"species", CODE_OUT_OF_RANGE>(
            "site must have exactly one species. I cannot handle multiple site occupancies.");
      return get_as<"element", std::string>(species.at(0))
          .combine(get_as<"abc", std::array<T, 3>>(site_json))
          .and_then([](auto&& s_and_frac) -> result_t {
            auto [symbol, frac_coords] = s_and_frac;
            if (!core::SYMBOL_MAP.contains(symbol))
              return parse_error::from_msg<"element", CODE_BAD_VALUE>(
                  std::format("I am not aware of the element {}", symbol));
            return std::make_tuple(core::SYMBOL_MAP.at(symbol), frac_coords);
          });
    }
    static nlohmann::json format_site(core::detail::site<T> const& site,
                                      lattice_t<T> const& lattice) {
      return nlohmann::json{
          {"abc", site.frac_coords},
          {"xyz", lattice.transpose() * site.frac_coords},
          {"label", std::format("{}{}", site.atom().symbol, site.index)},
          {"properties", nlohmann::json::object()},
          {"species", {nlohmann::json{{"element", site.atom().symbol}, {"occu", 1}}}}};
    }
  };

  template <class T> struct structure_adapter<T, STRUCTURE_FORMAT_CIF> {
    static std::string format(core::structure<T> const& structure) {
      auto sorted = structure.sorted([](auto&& a, auto&& b) { return a.specie < b.specie; });
      auto unique_species = core::helpers::sorted_vector<specie_t>{sorted.species};
      auto num_species = core::count_species(sorted.species);
      const auto z_to_symbol = [](auto&& z) { return core::atom::from_z(z).symbol; };
      std::string result;
      result.reserve(structure.size() * 144);

      const auto println
          = [&result](std::string const& line) { result.append(std::format("{}\n", line)); };

      const auto make_formula = [&](std::string const& delimiter) {
        return detail::join(unique_species | views::transform([&](auto&& s) {
                              return std::format("{}{}", z_to_symbol(s), num_species[s]);
                            }),
                            delimiter);
      };

      println("# generated using sqsgen");
      println(std::format("data_{}", make_formula("")));
      println("_symmetry_space_group_name_H-M   'P 1'");

      const auto [a, b, c] = detail::lengths<T>(structure.lattice);
      const auto [alpha, beta, gamma] = detail::angles<T>(structure.lattice);

      println(std::format("_cell_length_a       {:.8f}", a));
      println(std::format("_cell_length_b       {:.8f}", b));
      println(std::format("_cell_length_c       {:.8f}", c));
      println(std::format("_cell_angle_alpha     {:.8f}", alpha));
      println(std::format("_cell_angle_beta     {:.8f}", beta));
      println(std::format("_cell_angle_gamma     {:.8f}", gamma));

      println("_symmetry_Int_Tables_number   1");
      println(std::format("_chemical_formula_structural   {}", make_formula("")));
      println(std::format("_chemical_formula_sum   '{}'", make_formula(" ")));
      println(std::format("_cell_volume   {}", std::abs(structure.lattice.determinant())));
      println(std::format("_cell_formula_units_Z   {}", structure.size()));

      println("_loop");
      println(" _symmetry_equiv_pos_site_id");
      println(" _symmetry_equiv_pos_as_xyz");
      println("  1  'x, y, z'");

      println("_loop");
      println(" _atom_type_symbol");
      println(" _atom_type_oxidation_number");
      for (auto z : unique_species) println(std::format("  {}0+  0.0", z_to_symbol(z)));

      println("loop_");
      println(" _atom_site_type_symbol");
      println(" _atom_site_label");
      println(" _atom_site_symmetry_multiplicity");
      println(" _atom_site_fract_x");
      println(" _atom_site_fract_y");
      println(" _atom_site_fract_z");
      println(" _atom_site_occupancy");

      for (auto const& site : sorted.sites())
        println(std::format("  {}0+  {}{}  1  {:.8f}  {:.8f}  {:.8f}  1", z_to_symbol(site.specie),
                            z_to_symbol(site.specie), num_species[site.specie]--,
                            site.frac_coords(0), site.frac_coords(1), site.frac_coords(2)));
      println("");
      result.shrink_to_fit();
      return result;
    }
  };

}  // namespace sqsgen::io
#endif  // SQSGEN_IO_STRUCTURE_H
