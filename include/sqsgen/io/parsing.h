//
// Created by Dominik Gehringer on 07.12.24.
//

#ifndef SQSGEN_IO_PARSING_H
#define SQSGEN_IO_PARSING_H

#include <optional>
#include <string>

#include "sqsgen/core/helpers.h"

namespace sqsgen::io {
  using namespace sqsgen::core::helpers;

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

    template <string_literal key, parse_error_code code>
    static parse_error from_msg(const std::string& msg,
                                std::optional<std::string> parameter = std::nullopt) {
      return parse_error{key.data, msg, code, parameter};
    }

    template <parse_error_code code>
    static parse_error from_key_and_msg(std::string const& key, const std::string& msg,
                                        std::optional<std::string> parameter = std::nullopt) {
      return parse_error{key, msg, code, std::move(parameter)};
    }
  };

  template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
  };

  template <class... Args> using unpacked_t
      = std::conditional_t<sizeof...(Args) == 1, std::tuple_element_t<0, Args...>,
                           std::variant<Args...>>;

  template <class... Args> struct parse_result {
  private:
    std::variant<parse_error, Args...> _value;

  public:
    [[nodiscard]] bool failed() const { return std::holds_alternative<parse_error>(_value); }
    [[nodiscard]] bool ok() const { return !failed(); }
    [[nodiscard]] parse_error error() const { return std::get<parse_error>(_value); }
    unpacked_t<Args...> result() const {
      if (failed()) {throw std::bad_variant_access{};}
      if constexpr (sizeof...(Args) == 1) {
        return std::get<Args...>(_value);
      } else {
        std::optional<std::variant<Args...>> v = std::nullopt;
        ((v = v.has_value() ? v : (std::holds_alternative<Args>(_value) ?  std::get<Args>(_value) : std::nullopt)), ...);
        if (v.has_value()) return v.value();
        throw std::bad_variant_access{};
      }
    }
    unpacked_t<Args...> value_or(unpacked_t<Args...>&& value) { return failed() ? value : result(); }
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

  template <class A, class Fn>
  parse_result_t<A> validate(std::optional<parse_result_t<A>>&& a, A&& default_value, Fn&& fn) {
    if (a.has_value()) {
      if (holds_result(a.value())) {
        auto result = fn(std::forward<std::decay_t<A>>(get_result(a.value())));
        if (holds_result(result)) return get_result(result);
        return std::get<parse_error>(result);
      }
      return std::get<parse_error>(a.value());
    }
    return default_value;
  }

  template <class Collapse, class... Args, class... Fn>
  parse_result_t<Collapse> validate(std::optional<parse_result_t<Args...>>&& a,
                                    Collapse&& default_value, Fn&&... fn) {
    if (a.has_value()) {
      if (holds_result(a.value())) {
        auto error_case = [](parse_error&& error) -> parse_result_t<Collapse> { return error; };
        auto result = std::visit(overloaded{error_case, fn...},
                                 std::forward<parse_result_t<Args...>>(a.value()));
        if (holds_result(result)) return get_result(result);
        return std::get<parse_error>(result);
      }
      return std::get<parse_error>(a.value());
    }
    return default_value;
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

}  // namespace sqsgen::io

#endif  // SQSGEN_IO_PARSING_H
