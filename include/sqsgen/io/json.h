//
// Created by Dominik Gehringer on 23.10.24.
//

#ifndef SQSGEN_IO_JSON_H
#define SQSGEN_IO_JSON_H

#include <nlohmann/json.hpp>
#include <utility>

#include "sqsgen/core/helpers.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/parsing.h"
#include "sqsgen/types.h"

// partial specialization (full specialization works too)
NLOHMANN_JSON_NAMESPACE_BEGIN

using namespace sqsgen;
template <class T> struct adl_serializer<matrix_t<T>> {
  static void to_json(json& j, const matrix_t<T>& m) { j = core::helpers::eigen_to_stl(m); }

  static void from_json(const json& j, matrix_t<T>& m) {
    m = std::move(core::helpers::stl_to_eigen<matrix_t<T>>(j.get<stl_matrix_t<T>>()));
  }
};

template <class T> struct adl_serializer<lattice_t<T>> {
  static void to_json(json& j, const lattice_t<T>& m) { j = core::helpers::eigen_to_stl(m); }

  static void from_json(const json& j, lattice_t<T>& m) {
    m = std::move(core::helpers::stl_to_eigen<lattice_t<T>>(j.get<stl_matrix_t<T>>()));
  }
};

template <class T> struct adl_serializer<coords_t<T>> {
  static void to_json(json& j, const coords_t<T>& m) { j = core::helpers::eigen_to_stl(m); }

  static void from_json(const json& j, coords_t<T>& m) {
    m = std::move(core::helpers::stl_to_eigen<coords_t<T>>(j.get<stl_matrix_t<T>>()));
  }
};

template <class T> struct adl_serializer<cube_t<T>> {
  static void to_json(json& j, const cube_t<T>& m) { j = core::helpers::eigen_to_stl(m); }

  static void from_json(const json& j, coords_t<T>& m) {
    m = std::move(core::helpers::stl_to_eigen<cube_t<T>>(j.get<stl_cube_t<T>>()));
  }
};

template <class T> struct adl_serializer<core::helpers::sorted_vector<T>> {
  static void to_json(json& j, const core::helpers::sorted_vector<T>& m) {
    adl_serializer<std::vector<T>>::to_json(j, m._values);
  }

  static void from_json(const json& j, core::helpers::sorted_vector<T>& m) {
    std::vector<T> helper;
    adl_serializer<std::vector<T>>::from_json(j, helper);
    m = core::helpers::sorted_vector<T>(helper);
  }
};

template <class T> struct adl_serializer<core::structure<T>> {
  static void to_json(json& j, core::structure<T> const& s) {
    j = json{
        {"lattice", s.lattice},
        {"species", s.species},
        {"frac_coords", s.frac_coords},
        {"pbc", s.pbc},
    };
  }

  static void from_json(const json& j, core::structure<T>& s) {
    j.at("species").get_to(s.species);
    j.at("lattice").get_to(s.lattice);
    j.at("frac_coords").get_to(s.frac_coords);
    j.at("pbc").get_to(s.pbc);
  }
};
NLOHMANN_JSON_NAMESPACE_END

namespace sqsgen::io {

  static constexpr auto KEY_NONE = string_literal("");

  NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(sublattice, sites, composition);

  NLOHMANN_JSON_SERIALIZE_ENUM(Prec, {{PREC_SINGLE, "single"}, {PREC_DOUBLE, "double"}})
  NLOHMANN_JSON_SERIALIZE_ENUM(IterationMode, {
                                                  {ITERATION_MODE_INVALID, nullptr},
                                                  {ITERATION_MODE_RANDOM, "random"},
                                                  {ITERATION_MODE_SYSTEMATIC, "systematic"},
                                              })

  template <> struct accessor<nlohmann::json> {

    static bool contains(nlohmann::json const& json, std::string && key) {
      return json.contains(std::forward<std::string>(key));
    }

    template <string_literal key = "", class Option>
    static parse_result<Option> get_as(nlohmann::json const& json) {
      try {
        if constexpr (key == KEY_NONE) {
          return json.get<Option>();
        } else {

          if (json.contains(key.data)) return {json.at(key.data).template get<Option>()};
          return parse_error::from_msg<key, CODE_NOT_FOUND>(
              std::format("could not find key '{}'", key.data));
        }
      } catch (nlohmann::json::out_of_range const& e) {
        return parse_error::from_msg<key, CODE_TYPE_ERROR>("out of range - found - {}", e.what());
      } catch (nlohmann::json::type_error const& e) {
        return parse_error::from_msg<key, CODE_TYPE_ERROR>(
            std::format("type error - cannot parse {} - {}", typeid(Option).name(), e.what()));
      } catch (std::out_of_range const& e) {
        return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(e.what());
      }
    }
  };


}  // namespace sqsgen::io

#endif  // SQSGEN_IO_JSON_H
