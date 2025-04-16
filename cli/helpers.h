//
// Created by Dominik Gehringer on 13.04.25.
//

#ifndef HELPERS_H
#define HELPERS_H

#include <nlohmann/json.hpp>

#include "sqsgen/io/json.h"
#include "termcolor.h"

namespace sqsgen::cli {

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
