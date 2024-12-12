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

  template <class Needle, class... Haystack> using matches_any
      = std::conditional_t<(std::is_same_v<Needle, Haystack> || ...), std::true_type,
                           std::false_type>;

  namespace detail {
    template <class T, class U> struct combine_types {
      using type = std::tuple<T, U>;
      static type combine(T&& t, U&& u) {
        return std::make_tuple(std::forward<T>(t), std::forward<U>(u));
      }
    };
    template <class T, class... U> struct combine_types<T, std::tuple<U...>> {
      using type = std::tuple<T, U...>;
      static type combine(T&& t, std::tuple<U...>&& u) {
        return std::tuple_cat(std::make_tuple(std::forward<T>(t)),
                              std::forward<std::tuple<U...>>(u));
      }
    };
    template <class... T, class U> struct combine_types<std::tuple<T...>, U> {
      using type = std::tuple<T..., U>;
      static type combine(std::tuple<T...>&& t, U&& u) {
        return std::tuple_cat(std::forward<std::tuple<T...>>(t),
                              std::make_tuple(std::forward<U>(u)));
      }
    };
    template <class... T, class... U> struct combine_types<std::tuple<T...>, std::tuple<U...>> {
      using type = std::tuple<T..., U...>;
      static type combine(std::tuple<T...>&& t, std::tuple<U...>&& u) {
        return std::tuple_cat(std::forward<std::tuple<T...>>(t), std::forward<std::tuple<U...>>(u));
      }
    };

    template <std::size_t N, class... Args> using has_args
    = std::conditional_t<N == sizeof...(Args), std::true_type, std::false_type>;

    template <class... Args> using unpacked_t
        = std::conditional_t<sizeof...(Args) == 1, std::tuple_element_t<0, std::tuple<Args...>>,
                             std::variant<Args...>>;


  }  // namespace detail


  template <class... Args> struct parse_result {
  private:
    std::variant<parse_error, Args...> _value;
    template<class> struct is_parse_result : std::false_type {};
    template<class ...Other> struct is_parse_result<parse_result<Other...>> : std::true_type {};

  public:
    parse_result(std::variant<parse_error, Args...>&& value) : _value(value) {}
    parse_result(parse_error&& error) : _value(error) {}

    template <class Arg>
      requires matches_any<Arg, Args...>::value
    parse_result(Arg&& arg) : _value(arg) {}

    [[nodiscard]] bool failed() const { return std::holds_alternative<parse_error>(_value); }
    [[nodiscard]] bool ok() const { return !failed(); }
    [[nodiscard]] parse_error error() const { return std::get<parse_error>(_value); }
    detail::unpacked_t<Args...> result() const {
      if (failed()) {
        throw std::bad_variant_access{};
      }
      if constexpr (sizeof...(Args) == 1) {
        return std::get<Args...>(_value);
      } else {
        std::optional<std::variant<Args...>> v = std::nullopt;
        ((v = v.has_value()
                  ? v
                  : (std::holds_alternative<Args>(_value) ? std::get<Args>(_value) : std::nullopt)),
         ...);
        if (v.has_value()) return v.value();
        throw std::bad_variant_access{};
      }
    }
    detail::unpacked_t<Args...> value_or(detail::unpacked_t<Args...>&& value) {
      return failed() ? value : result();
    }

    template <class Collapse, class... Fn> parse_result<Collapse> collapse(Fn&&... fn) {
      return std::visit(overloaded{[](parse_error&& error) -> parse_result<Collapse> {
                                     return parse_result<Collapse>{error};
                                   },
                                   fn...},
                        std::forward<decltype(_value)>(_value));
    }

    template <class Fn> requires detail::has_args<1, Args...>::value
    std::decay_t<std::invoke_result_t<Fn, Args...>> and_then(Fn&& fn) {
      using out_t = std::decay_t<std::invoke_result_t<Fn, Args...>>;
      using forward_t = std::conditional_t<is_parse_result<out_t>::value, out_t, parse_result<out_t>>;
      return failed() ? forward_t{error()}
                      : fn(std::forward<Args...>(std::get<Args...>(_value)));
    }

    template <class Other, class Arg = std::tuple_element_t<0, std::tuple<Args...>>>
      requires detail::has_args<1, Args...>::value
    parse_result<typename detail::combine_types<Arg, Other>::type> combine(
        parse_result<Other>&& other) {
      static_assert(sizeof...(Args) == 1,
                    "combine only allows for nonvariant parse_results - requires 1 argument");
      using result_t = parse_result<typename detail::combine_types<Arg, Other>::type>;
      return (failed() ? result_t{error()}
                       : (other.failed() ? result_t{other.error()}
                                         : result_t{detail::combine_types<Arg, Other>::combine(
                                               result(), other.result())}));
    }
  };

  template <class> struct accessor {};

  namespace detail {
    template <class... Options, class Option>
    parse_result<Options...> forward_superset(parse_result<Option>&& result) {
      if (result.failed()) {
        return {result.error()};
      }
      return {result.result()};
    }
  }  // namespace detail

  template <class Fn, class A>
  std::optional<std::decay_t<std::invoke_result_t<Fn, A>>> fmap(Fn&& fn, std::optional<A>&& opt) {
    if (opt.has_value()) return fn(std::forward<A>(opt.value()));
    return std::nullopt;
  }

  template <string_literal key, class... Options, class Document,
            class Doc = std::decay_t<Document>>
  std::optional<parse_result<Options...>> get_either_optional(Document const& doc) {
    using result_t = parse_result<Options...>;
    if (accessor<Doc>::contains(doc, key.data)) {
      result_t result = {parse_error::from_msg<key, CODE_UNKNOWN>(
          std::format("unknown error failed to load {}", key.data))};
      ((result = result.failed() ? detail::forward_superset<Options...>(
                                       accessor<Doc>::template get_as<key, Options>(doc))
                                 : result),
       ...);
      return result;
    }
    return std::nullopt;
  }

  template <string_literal key, class... Options, class Document>
  parse_result<Options...> get_either(Document const& json) {
    return get_either_optional<key, Options...>(json).value_or(
        parse_error::from_msg<key, CODE_NOT_FOUND>("could not find key {}", key.data));
  }

  template <string_literal key, class As, class Document>
  parse_result<As> get_as(Document const& json) {
    return get_either<key, As>(json);
  }

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
