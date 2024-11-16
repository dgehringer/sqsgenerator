//
// Created by Dominik Gehringer on 23.10.24.
//

#ifndef SQSGEN_IO_JSON_H
#define SQSGEN_IO_JSON_H

#include <nlohmann/json.hpp>

#include "sqsgen/config.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

using namespace sqsgen;

// partial specialization (full specialization works too)
NLOHMANN_JSON_NAMESPACE_BEGIN
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
    m = std::move(core::helpers::stl_to_eigen < cube_t < T >>> j.get<stl_cube_t<T>>());
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

namespace sqsgen {

  using json = nlohmann::json;

  NLOHMANN_JSON_SERIALIZE_ENUM(Prec, {{PREC_SINGLE, "single"}, {PREC_DOUBLE, "double"}})

  struct parse_error {
    std::string parameter;
    std::string msg;

    template <core::helpers::string_literal key>
    static parse_error from_msg(const std::string& msg) {
      return parse_error{key.data, msg};
    }
  };

  template <core::helpers::string_literal key, class T>
  std::optional<T> get_optional(json const& json) {
    if (json.contains(key.data)) {
      return std::make_optional(json.at(key.data).template get<T>());
    }
    return std::nullopt;
  }

  template <core::helpers::string_literal key, class Option>
  std::variant<parse_error, Option> get_as_helper(nlohmann::json const& json) {
    try {
      return json.at(key.data).template get<Option>();
    } catch (nlohmann::json::out_of_range const&) {
      return parse_error::from_msg<key>("out of range");
    } catch (nlohmann::json::type_error const&) {
      return parse_error::from_msg<key>("type error");
    }
  };

  template <class ... Options, class Option>
  std::variant<parse_error, Options ...> forward_variant(std::variant<parse_error, Option> && opt) {
    if (std::holds_alternative<parse_error>(opt)) {
      return std::get<parse_error>(opt);
    } else {
      return std::get<Option>(opt);
    }
  }

  template <core::helpers::string_literal key, class... Options>
  std::optional<std::variant<parse_error, Options...>> get_either(nlohmann::json const& json) {
    if (json.contains(key.data)) {
      std::variant<parse_error, Options...> result = parse_error::from_msg<key>(std::format("failed to load {}", key.data));
      ((result = forward_variant<Options...>(get_as_helper<key, Options>(json))), ...);
      return result;
    }
    return std::nullopt;
  }

}  // namespace sqsgen

#endif  // SQSGEN_IO_JSON_H
