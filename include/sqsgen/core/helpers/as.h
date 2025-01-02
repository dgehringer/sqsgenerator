//
// Created by Dominik Gehringer on 10.11.24.
//

#ifndef SQSGEN_CORE_HELPERS_AS_H
#define SQSGEN_CORE_HELPERS_AS_H

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "sqsgen/core/helpers/sorted_vector.h"

namespace sqsgen::core::helpers {
  namespace ranges = std::ranges;
  template <template <class...> class> struct as {};

  template <> struct as<std::vector> {

    template <ranges::range R>
    std::vector<std::decay_t<ranges::range_value_t<R>>> operator()(R&& r) {
      return {std::begin(r), std::end(r)};
    }
  };

  template <> struct as<std::unordered_set> {
    template <class hash, ranges::range R>
    std::unordered_set<std::decay_t<ranges::range_value_t<R>>, hash> operator()(R&& r) {
      return {std::begin(r), std::end(r)};
    }

    template <ranges::range R> auto operator()(R&& r) {
      using value_t = std::decay_t<ranges::range_value_t<R>>;
      return operator()<std::hash<value_t>, R>(std::forward<R>(r));
    }
  };

  template <> struct as<sorted_vector> {
    template <ranges::range R> auto operator()(R&& r) {
      using value_t = std::decay_t<ranges::range_value_t<R>>;
      sorted_vector<value_t> result{};
      result.merge(std::forward<R>(r));
      return result;
    }
  };

  template <> struct as<std::set> {
    template <class compare, ranges::range R>
    std::set<std::decay_t<ranges::range_value_t<R>>, compare> operator()(R&& r) {
      return {std::begin(r), std::end(r)};
    }

    template <ranges::range R> auto operator()(R&& r) {
      using value_t = std::decay_t<ranges::range_value_t<R>>;
      return operator()<std::less<value_t>, R>(std::forward<R>(r));
    }
  };

  template <> struct as<std::unordered_map> {
    template <ranges::range R> auto operator()(R&& r) {
      using element_t = ranges::range_value_t<R>;
      using key_t = std::tuple_element_t<0, element_t>;
      using value_t = std::tuple_element_t<1, element_t>;
      using return_t = std::unordered_map<key_t, value_t>;
      return_t result{};
      for (const auto& [key, value] : r) result[key] = value;
      ;
      return result;
    }
  };

  template <> struct as<std::map> {
    template <ranges::range R> auto operator()(R&& r) {
      using element_t = ranges::range_value_t<R>;
      using key_t = std::tuple_element_t<0, element_t>;
      using value_t = std::tuple_element_t<1, element_t>;
      using return_t = std::map<key_t, value_t>;
      return_t result{};
      for (const auto& [key, value] : r) result[key] = value;
      return result;
    }
  };

}  // namespace sqsgen::core::helpers
#endif  // SQSGEN_CORE_HELPERS_AS_H
