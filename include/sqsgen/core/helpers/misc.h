

#ifndef SQSGEN_CORE_HELPERS_MISC_H
#define SQSGEN_CORE_HELPERS_MISC_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <ranges>
#include <utility>
#include <vector>

namespace sqsgen::core::helpers {

  template <std::ranges::input_range R, class Predicate, class T = std::ranges::range_value_t<R>>
    requires std::predicate<Predicate&, std::ranges::range_reference_t<R>>
  std::pair<std::vector<T>, std::vector<T>> partition(Predicate&& predicate, R&& r) {
    std::pair<std::vector<T>, std::vector<T>> out;

    if constexpr (std::ranges::sized_range<R>) {
      const auto n = std::ranges::size(r);
      out.first.reserve(n);
      out.second.reserve(n);
    }

    for (auto&& e : r) {
      // forward each element: moves out of an rvalue range, copies from an lvalue one
      if (std::invoke(predicate, e))
        out.first.emplace_back(std::forward<decltype(e)>(e));
      else
        out.second.emplace_back(std::forward<decltype(e)>(e));
    }
    return out;
  }

  template <class U, std::ranges::input_range R, class T = std::ranges::range_value_t<R>>
    requires std::is_integral_v<U>
  std::map<U, T> index_map(R&& r) {
    // collect the values, then sort them and drop duplicates
    std::vector<T> elements;
    if constexpr (std::ranges::sized_range<R>) elements.reserve(std::ranges::size(r));
    for (auto&& e : r) elements.emplace_back(std::forward<decltype(e)>(e));
    std::sort(elements.begin(), elements.end());
    elements.erase(std::unique(elements.begin(), elements.end()), elements.end());

    // number the unique values 0, 1, 2, ...
    std::map<U, T> result;
    U index = 0;
    for (auto const& value : elements) result.emplace(index++, value);
    return result;
  }

  template <class T, class U>
    requires std::is_integral_v<U>
  std::map<T, U> reversed_index_map(std::map<U, T> const& m) {
    std::map<T, U> result;
    for (auto const& [k, v] : m) result.emplace(v, k);
    return result;
  }

  template <class U, std::ranges::input_range R, class T = std::ranges::range_value_t<R>>
    requires std::is_integral_v<U>
  std::map<T, U> reversed_index_map(R&& r) {
    return reversed_index_map(index_map<U>(std::forward<R>(r)));
  }

  // both directions: { reversed (T->U), forward (U->T) }
  template <class U, std::ranges::input_range R, class T = std::ranges::range_value_t<R>>
    requires std::is_integral_v<U>
  std::pair<std::map<U, T>, std::map<T, U>> make_index_mapping(R&& r) {
    std::map<U, T> forward_map = index_map<U>(std::forward<R>(r));
    return index_mapping_t<T, U>(reversed_index_map(forward_map), forward_map);
  }

  inline std::string to_uppercase(std::string_view input) {
    std::string result{input};
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
  }

  template <class T> bool is_close(T a, T b, T atol = 1.0e-5, T rtol = 1.0e-3) {
    return std::abs(a - b) <= (atol + rtol * std::abs(b));
  }

  inline constexpr std::uint64_t golden_ratio_u64 = 0x9e3779b97f4a7c15ULL;

  // Boost-style 64-bit hash combine.
  inline std::size_t hash_combine(std::size_t seed, std::size_t value) {
    return seed ^ (value + golden_ratio_u64 + (seed << 6) + (seed >> 2));
  }

  template <class T> T absolute(T a) { return a > 0 ? a : -a; }

}  // namespace sqsgen::core::helpers

#endif  // SQSGEN_CORE_HELPERS_MISC_H
