//
// Created by Dominik Gehringer on 23.10.24.
//

#ifndef SQSGEN_IO_JSON_H
#define SQSGEN_IO_JSON_H

#include <nlohmann/json.hpp>

#include "sqsgen/core/helpers.h"
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
    m = std::move(core::helpers::stl_to_eigen<cube_t<T>>(j.get<stl_cube_t<T>>()));
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

  static constexpr auto KEY_NONE = core::helpers::string_literal("");

  NLOHMANN_JSON_SERIALIZE_ENUM(Prec, {{PREC_SINGLE, "single"}, {PREC_DOUBLE, "double"}})
  NLOHMANN_JSON_SERIALIZE_ENUM(IterationMode, {
                                                  {ITERATION_MODE_INVALID, nullptr},
                                                  {ITERATION_MODE_RANDOM, "random"},
                                                  {ITERATION_MODE_SYSTEMATIC, "systematic"},
                                              })
  enum parse_error_code {
    CODE_UNKNOWN = -1,
    CODE_NOT_FOUND = 0,
    CODE_TYPE_ERROR = 1,
    CODE_OUT_OF_RANGE = 2,
    CODE_BAD_VALUE = 3,
    CODE_BAD_ARGUMENT = 4,
  };

  struct parse_error {
    std::string key;
    std::string msg;
    parse_error_code code;
    std::optional<std::string> parameter = std::nullopt;

    parse_error with_key(std::string const& new_key) {
      return parse_error{new_key, msg, code, parameter};
    }

    template <core::helpers::string_literal key, parse_error_code code>
    static parse_error from_msg(const std::string& msg,
                                std::optional<std::string> parameter = std::nullopt) {
      return parse_error{key.data, msg, code, parameter};
    }

    template <parse_error_code code>
    static parse_error from_key_and_msg(std::string const& key, const std::string& msg,
                                        std::optional<std::string> parameter = std::nullopt) {
      return parse_error{key, msg, code, parameter};
    }
  };
  template <class... Args> using parse_result_t = std::variant<parse_error, Args...>;

  template <class... Args> bool holds_result(parse_result_t<Args...> const& a) {
    return !std::holds_alternative<parse_error>(a);
  }

  template <class... Args> bool holds_error(parse_result_t<Args...> const& a) {
    return std::holds_alternative<parse_error>(a);
  }

  template <class A> A get_result(parse_result_t<A> const& a) { return std::get<A>(a); }

  template <class A, class Fn>
  parse_result_t<std::optional<A>> validate(std::optional<parse_result_t<A>>&& a, Fn&& fn) {
    if (a.has_value()) {
      if (holds_result(a.value())) {
        auto result = fn(std::forward<std::decay_t<A>>(get_result(a.value())));
        if (holds_result(result)) return std::make_optional(get_result(result));
        return std::get<parse_error>(result);
      }
      return std::get<parse_error>(a.value());
    }
    return std::nullopt;
  }

  template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
  };

  template <class Collapse, class... Args, class... Fn>
  parse_result_t<std::optional<Collapse>> validate(std::optional<parse_result_t<Args...>>&& a,
                                                   Fn&&... fn) {
    if (a.has_value()) {
      if (holds_result(a.value())) {
        auto error_case = [](parse_error&& error) -> parse_result_t<Collapse> { return error; };
        auto result = std::visit(overloaded{error_case, fn...},
                                 std::forward<parse_result_t<Args...>>(a.value()));
        if (holds_result(result)) return std::make_optional(get_result(result));
        return std::get<parse_error>(result);
      }
      return std::get<parse_error>(a.value());
    }
    return std::nullopt;
  }
  template <class Collapse, class... Args, class... Fn>
  parse_result_t<Collapse> validate(parse_result_t<Args...>&& a, Fn&&... fn) {
    if (holds_result(a)) {
      auto error_case = [](parse_error&& error) -> parse_result_t<Collapse> { return error; };
      return std::visit(overloaded{error_case, fn...}, std::forward<parse_result_t<Args...>>(a));
    }
    return std::get<parse_error>(a);
  }

  template <class... Options, class Option>
  parse_result_t<Options...> forward_variant(std::variant<parse_error, Option>&& opt) {
    if (std::holds_alternative<parse_error>(opt)) {
      return std::get<parse_error>(opt);
    }
    return std::get<Option>(opt);
  }

  template <core::helpers::string_literal key = "", class Option>
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

  template <class Option>
  parse_result_t<Option> get_as(std::string const& key, nlohmann::json const& json) {
    try {
      return json.at(key).get<Option>();
    } catch (nlohmann::json::out_of_range const& e) {
      return parse_error::from_key_and_msg<CODE_TYPE_ERROR>(key, "out of range or not found - {}",
                                                            e.what());
    } catch (nlohmann::json::type_error const& e) {
      return parse_error::from_key_and_msg<CODE_TYPE_ERROR>(
          key, std::format("type error - cannot parse {} - {}", typeid(Option).name(), e.what()));
    } catch (std::out_of_range const& e) {
      return parse_error::from_key_and_msg<CODE_OUT_OF_RANGE>(key, e.what());
    }
  };

  template <core::helpers::string_literal key, class... Options>
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
