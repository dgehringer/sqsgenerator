//
// Created by Dominik Gehringer on 23.10.24.
//

#ifndef SQSGEN_IO_JSON_H
#define SQSGEN_IO_JSON_H

#include <nlohmann/json.hpp>
#include <utility>

#include "sqsgen/core/helpers.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"
#include "sqsgen/io/parsing.h"

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


  template <string_literal key = "", class Option>
  parse_result_t<Option> get_as(nlohmann::json const& json) {
    try {
      if constexpr (key == KEY_NONE) {
        return json.get<Option>();
      } else
        return json.at(key.data).template get<Option>();
    } catch (nlohmann::json::out_of_range const& e) {
      return parse_error::from_msg<key, CODE_TYPE_ERROR>("out of range or not found - {}",
                                                         e.what());
    } catch (nlohmann::json::type_error const& e) {
      return parse_error::from_msg<key, CODE_TYPE_ERROR>(
          std::format("type error - cannot parse {} - {}", typeid(Option).name(), e.what()));
    } catch (std::out_of_range const& e) {
      return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(e.what());
    }
  }


  template <string_literal key, class... Options>
  std::optional<parse_result_t<Options...>> get_either_optional(nlohmann::json const& json) {
    if (json.contains(key.data)) {
      std::variant<parse_error, Options...> result
          = parse_error::from_msg<key, CODE_UNKNOWN>(std::format("failed to load {}", key.data));
      ((result
        = holds_error(result) ? forward_variant<Options...>(get_as<key, Options>(json)) : result),
       ...);
      return result;
    }
    return std::nullopt;
  }

  template <string_literal key, class... Options>
  parse_result_t<Options...> get_either(nlohmann::json const& json) {
    return get_either_optional<key, Options...>(json).value_or(
        parse_error::from_msg<key, CODE_NOT_FOUND>("could not find key {}", key.data));
  }

  template <string_literal key, class Value>
  std::optional<parse_result_t<Value>> get_optional(nlohmann::json const& json) {
    return get_either_optional<key, Value>(json);
  }

  template <class... Args>
  parse_result_t<std::tuple<Args...>> combine(parse_result_t<Args>&&... args) {
    std::optional<parse_error> error = std::nullopt;
    ((error = error.has_value()
                  ? error
                  : (holds_error<Args>(args) ? std::make_optional(std::get<parse_error>(args))
                                             : std::nullopt)),
     ...);
    if (error.has_value()) {
      return error.value();
    }
    return std::make_tuple(std::get<Args>(args)...);
  }
}  // namespace sqsgen

#endif  // SQSGEN_IO_JSON_H
