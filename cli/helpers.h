//
// Created by Dominik Gehringer on 13.04.25.
//

#ifndef HELPERS_H
#define HELPERS_H

#include <nlohmann/json.hpp>

#include "sqsgen/io/json.h"

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
    std::cout << j;
    return j;
  }
}  // namespace sqsgen::cli

#endif  // HELPERS_H
