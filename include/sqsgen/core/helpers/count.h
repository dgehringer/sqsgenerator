//
// Created by Dominik Gehringer on 20.06.26.
//

#ifndef SQSGEN_CORE_HELPERS_COUNT_H
#define SQSGEN_CORE_HELPERS_COUNT_H

#include <cstddef>
#include <map>
#include <ranges>

namespace sqsgen::core::helpers {

  template <std::ranges::range R, class T = std::ranges::range_value_t<R>>
  std::map<T, std::size_t> count(R&& r) {
    std::map<T, std::size_t> result{};
    for (auto e : r) {
      if (result.contains(e))
        ++result[e];
      else
        result.emplace(e, 1);
    }
    return result;
  }
}  // namespace sqsgen::core::helpers

#endif  // SQSGEN_CORE_HELPERS_COUNT_H
