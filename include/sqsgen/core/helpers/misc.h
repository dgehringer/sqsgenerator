

#ifndef SQSGEN_CORE_HELPERS_MISC_H
#define SQSGEN_CORE_HELPERS_MISC_H

#include <ranges>

#include "absl/strings/str_format.h"
#include "sqsgen/core/helpers/as.h"
#include "sqsgen/types.h"

namespace sqsgen::core::helpers {

  template <class U, ranges::input_range R, class T = ranges::range_value_t<R>>
    requires std::is_integral_v<U>
  std::map<T, U> index_map(R&& r) {
    auto elements = as<sorted_vector>{}(r);
    std::sort(elements.begin(), elements.end());
    U index = 0;
    std::map<U, T> index_map;
    std::transform(elements.begin(), elements.end(), std::inserter(index_map, index_map.begin()),
                   [&](auto const& val) { return std::make_pair(index++, val); });
    return index_map;
  }

  template <class T, class U>
    requires std::is_integral_v<U>
  std::map<T, U> reversed_index_map(std::map<U, T> const& index_map) {
    return as<std::map>{}(index_map | std::ranges::views::transform([&](auto const& item) {
                            auto [k, v] = item;
                            return std::make_pair(v, k);
                          }));
  }

  template <class U, ranges::input_range R, class T = ranges::range_value_t<R>>
    requires std::is_integral_v<U>
  std::map<T, U> reversed_index_map(R&& r) {
    return reversed_index_map(index_map<U, R, T>(std::forward(r)));
  }

  template <class U, ranges::input_range R, class T = ranges::range_value_t<R>>
    requires std::is_integral_v<U>
  index_mapping_t<T, U> make_index_mapping(R&& r) {
    std::map<U, T> forward_map = index_map(std::forward(r));
    return index_mapping_t<T, U>(reversed_index_map(forward_map), forward_map);
  }

  inline std::string to_uppercase(std::string_view input) {
    std::string result{input};
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
  }

  template <class... Args>
  std::string format_string(const absl::FormatSpec<Args...>& fmt, Args&&... args) {
    return absl::StrFormat(fmt, std::forward<Args>(args)...);
  }

}  // namespace sqsgen::core::helpers

#endif  // SQSGEN_CORE_HELPERS_MISC_H
