//
// Created by Dominik Gehringer on 13.04.25.
//

#ifndef HELPERS_H
#define HELPERS_H

#include <nlohmann/json.hpp>
#include <ranges>
#include <regex>

#include "sqsgen/io/json.h"
#include "termcolor.h"

namespace sqsgen::cli {

  namespace ranges = std::ranges;
  namespace views = std::ranges::views;

  template <ranges::range Range>
    requires std::is_same_v<ranges::range_value_t<Range>, std::string>
  std::string join(Range&& crumbs, std::string const& delimiter = " ") {
    auto csize = ranges::size(crumbs);
    if (csize == 0) return "";
    std::string result;
    result.reserve(core::helpers::sum(crumbs | views::transform([](auto&& s) { return s.size(); }))
                   + (csize - 1) * delimiter.size());
    for (auto it = ranges::begin(crumbs); it != ranges::end(crumbs); ++it) {
      if (it != ranges::begin(crumbs)) result.append(delimiter);
      result.append(*it);
    }
    result.shrink_to_fit();
    return result;
  }

  template <class T, class Fn> nlohmann::json fmap_to_json(Fn&& fn, std::vector<T> const& vec) {
    auto result = nlohmann::json::array();
    for (auto& el : vec) result.push_back(fn(std::forward<decltype(el)>(el)));
    return result;
  }

  inline auto fixup_compositon(sublattice const& sl) {
    nlohmann::json result;
    result["sites"] = sl.sites;
    for (auto&& [k, v] : sl.composition) result[core::atom::from_z(k).symbol] = v;
    return result;
  }

  template <class T> inline auto fixup_shell_weights(shell_weights_t<T> const& w) {
    nlohmann::json result;
    for (auto&& [k, v] : w) result[std::to_string(k)] = v;
    return result;
  }

  template <class T> nlohmann::json fixup_config_json(core::configuration<T> const& config) {
    nlohmann::json j = config;

    j["composition"] = fmap_to_json(fixup_compositon, config.composition);
    j["shell_weights"] = fmap_to_json(fixup_shell_weights<T>, config.shell_weights);
    if (config.sublattice_mode == SUBLATTICE_MODE_INTERACT) {
      j["shell_radii"] = config.shell_radii.front();
      j["prefactors"] = config.prefactors.front();
      j["shell_weights"] = fixup_shell_weights<T>(config.shell_weights.front());
      j["pair_weights"] = config.pair_weights.front();
      j["target_objective"] = config.target_objective.front();
    }
    return j;
  }

  inline std::string pad_right(std::string const& str, size_t s) {
    if (str.size() < s)
      return str + std::string(s - str.size(), ' ');
    else
      return str;
  }

  inline std::string pad_left(std::string const& str, size_t s) {
    if (str.size() < s)
      return std::string(s - str.size(), ' ') + str;
    else
      return str;
  }

  inline std::string format_hyperlink(std::string_view text, std::string_view link) {
    return std::format("\033]8;;{}\007{}\033]8;;\007", link, text);
  }

  inline std::size_t print_width(const std::string& str) {
    // Regex to match ANSI escape sequences
    static const std::regex ansi_escape("\033\\[[0-9;]*[a-zA-Z]");
    // Remove ANSI escape sequences
    std::string stripped = std::regex_replace(str, ansi_escape, "");
    // Return the length of the stripped string
    return stripped.size();
  }

  template <core::helpers::string_literal... Columns> struct table {
    using row = std::array<std::string, sizeof...(Columns)>;
    template <std::ranges::range R>
      requires std::is_same_v<row, std::ranges::range_value_t<R>>
    static void render(R&& r) {
      using namespace termcolor;
      constexpr std::size_t num_columns = sizeof...(Columns);
      std::array<std::size_t, num_columns> col_widths{(Columns.length - 1)...};
      std::array<std::string, num_columns> col_headers{std::string{Columns.data}...};
      std::vector<row> rows;
      // find maximum col width for each row in the range
      std::ranges::for_each(r, [&](auto&& row) {
        [&]<std::size_t... I>(std::index_sequence<I...>) {
          ((col_widths[I]
            = std::max(col_widths[I], print_width(std::get<I>(std::forward<decltype(row)>(row))))),
           ...);
          rows.push_back(std::forward<decltype(row)>(row));
        }(std::make_index_sequence<num_columns>{});
      });

      constexpr auto separator = "  ";

      const auto format_header = [&](std::string const& header, std::size_t w, bool sep) {
        std::cout << grey << bold << underline << pad_right(header, w) << reset;
        std::cout << (sep ? separator : "\n");
      };

      [&]<std::size_t... I>(std::index_sequence<I...>) {
        ((format_header(col_headers[I], col_widths[I], I != num_columns - 1)), ...);
      }(std::make_index_sequence<num_columns>{});

      const auto format_cell = [&](std::string const& header, std::size_t w, bool sep) {
        auto diff = header.size() > print_width(header) ? (header.size() - print_width(header)) : 0;
        std::cout << pad_right(header, w + diff);
        std::cout << (sep ? separator : "\n");
      };

      std::ranges::for_each(rows, [&](auto&& row) {
        [&]<std::size_t... I>(std::index_sequence<I...>) {
          ((format_cell(std::get<I>(row), col_widths[I], I != num_columns - 1)), ...);
        }(std::make_index_sequence<num_columns>{});
      });
    }
  };

  inline void render_error(std::string_view message, bool exit = true,
                           std::optional<std::string> parameter = std::nullopt,
                           std::optional<std::string> info = std::nullopt) {
    using namespace termcolor;
    std::cout << red << bold << underline << "Error:" << reset << " " << message << std::endl;
    if (info.has_value())
      std::cout << "       (" << blue << italic << "info: " << reset << italic << info.value()
                << reset << ")" << std::endl;
    if (parameter.has_value())
      std::cout
          << bold << blue << "Help: " << reset
          << std::format(
                 "the documentation for parameter \"{}\" is available at: {}", parameter.value(),
                 format_hyperlink(
                     std::format(
                         "https://sqsgenerator.readthedocs.io/en/latest/input_parameters.html#{}",
                         parameter.value()),
                     std::format(
                         "https://sqsgenerator.readthedocs.io/en/latest/input_parameters.html#{}",
                         parameter.value())))
          << std::endl;
    if (exit) std::exit(1);
  }

  inline void write_file(std::string const& path, std::string const& content) {}

  inline std::string read_file(std::string const& filename) {
    std::ifstream ifs(filename);
    if (!ifs) throw std::runtime_error(std::format("Could not open file: {}", filename));
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
  }

  nlohmann::json read_msgpack(std::string const& filename) {
    if (!std::filesystem::exists(filename))
      render_error(std::format("File '{}' does not exist", filename), true);
    std::ifstream ifs(filename, std::ios::in | std::ios::binary);
    nlohmann::json config_json;
    try {
      config_json = nlohmann::json::from_msgpack(ifs);
    } catch (nlohmann::json::parse_error& e) {
      render_error(std::format("'{}' is not a valid msgpack file", filename), true, std::nullopt,
                   e.what());
    }
    return config_json;
  }

  nlohmann::json read_json(std::string const& filename) {
    if (!std::filesystem::exists(filename)) render_error("File '{}' does not exist", true);
    std::string data;
    try {
      data = read_file(filename);
    } catch (const std::exception& e) {
      render_error(std::format("Error reading file '{}'", filename), true, std::nullopt, e.what());
    }

    nlohmann::json config_json;
    try {
      config_json = nlohmann::json::parse(data);
    } catch (nlohmann::json::parse_error& e) {
      render_error(std::format("'{}' is not a valid JSON file", filename), true, std::nullopt,
                   e.what());
    }
    return config_json;
  }

  int validate_index(auto const& raw, auto size) {
    int index{-1};
    try {
      index = std::stoi(raw);
    } catch (const std::invalid_argument& e) {
      render_error(std::format("Invalid index '{}'", raw), true, std::nullopt, e.what());
    }
    if (index < 0 || index >= size)
      render_error(std::format("Invalid index '{}'", raw), true, std::nullopt,
                   std::format("index must be between 0 and {}", size - 1));
    return index;
  }

}  // namespace sqsgen::cli

#endif  // HELPERS_H
