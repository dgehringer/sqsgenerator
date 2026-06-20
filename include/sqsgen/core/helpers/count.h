//
// Created by Dominik Gehringer on 20.06.26.
//

#ifndef SQSGEN_CORE_HELPERS_COUNT_H
#define SQSGEN_CORE_HELPERS_COUNT_H

#include <ranges>

#include "sqsgen/types.h"

namespace sqsgen::core::helpers {
  namespace ranges = std::ranges;
  namespace views = ranges::views;

  template <ranges::range R, class T = ranges::range_value_t<R>> counter<T> count(R&& r) {
    counter<T> result{};
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
