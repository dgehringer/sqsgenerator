//
// Created by Dominik Gehringer on 23.10.24.
//

#ifndef SQSGEN_IO_JSON_H
#define SQSGEN_IO_JSON_H

#include <nlohmann/json.hpp>

#include "sqsgen/core/structure.h"
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

  NLOHMANN_JSON_SERIALIZE_ENUM(Prec, {{PREC_SINGLE, "single"}, {PREC_DOUBLE, "double"}})

  enum parse_error_code { CODE_UNKNOWN = -1, CODE_NOT_FOUND = 0, CODE_TYPE_ERROR = 1 };

  struct parse_error {
    std::string key;
    std::string msg;
    parse_error_code code;
    std::optional<std::string> parameter = std::nullopt;

    template <core::helpers::string_literal key, parse_error_code code>
    static parse_error from_msg(const std::string& msg,
                                std::optional<std::string> parameter = std::nullopt) {
      return parse_error{key.data, msg, code, parameter};
    }
  };

  template <class... Args> using parse_result_t = std::variant<parse_error, Args...>;

  template <class... Args> bool holds_result(parse_result_t<Args...> const& a) {
    return !std::holds_alternative<parse_error>(a);
  }

  template <class... Args> bool holds_error(parse_result_t<Args...> const& a) {
    return std::holds_alternative<parse_error>(a);
  }

  template <core::helpers::string_literal key, class Option>
  parse_result_t<Option> get_as(nlohmann::json const& json) {
    try {
      return json.at(key.data).template get<Option>();
    } catch (nlohmann::json::out_of_range const&) {
      return parse_error::from_msg<key, CODE_TYPE_ERROR>("out of range");
    } catch (nlohmann::json::type_error const&) {
      return parse_error::from_msg<key, CODE_TYPE_ERROR>("type error");
    }
  };

  template <class... Options, class Option>
  parse_result_t<Options...> forward_variant(std::variant<parse_error, Option>&& opt) {
    if (std::holds_alternative<parse_error>(opt)) {
      return std::get<parse_error>(opt);
    } else {
      return std::get<Option>(opt);
    }
  }

  template <core::helpers::string_literal key, class... Options>
  std::optional<parse_result_t<Options...>> get_either_optional(nlohmann::json const& json) {
    if (json.contains(key.data)) {
      std::variant<parse_error, Options...> result
          = parse_error::from_msg<key, CODE_UNKNOWN>(std::format("failed to load {}", key.data));
      ((result = forward_variant<Options...>(get_as<key, Options>(json))), ...);
      return result;
    }
    return std::nullopt;
  }

  template <core::helpers::string_literal key, class... Options>
  parse_result_t<Options...> get_either(nlohmann::json const& json) {
    return get_either_optional<key, Options...>(json).value_or(
        parse_error::from_msg<key, CODE_NOT_FOUND>("could not find key {}", key.data));
  }

  template <core::helpers::string_literal key, class Value>
  std::optional<parse_result_t<Value>> get_optional(nlohmann::json const& json) {
    return get_either_optional<key, Value>(json);
  }

  template <class... Args>
  parse_result_t<std::tuple<Args...>> combine(parse_result_t<Args> const& ...args) {
    std::optional<parse_error> error = std::nullopt;
    ((error = error.has_value()
                  ? error
                  : (holds_error<Args>(args) ? std::get<parse_error>(args) : std::nullopt)),
     ...);
    if (error.has_value()) {
      return error.value();
    } else {
      return std::make_tuple(std::get<Args>(args)...);
    }

  }
}  // namespace sqsgen

#endif  // SQSGEN_IO_JSON_H
