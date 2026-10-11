

#ifndef SQSGEN_CORE_HELPERS_EXPECTED_H
#define SQSGEN_CORE_HELPERS_EXPECTED_H

#include <expected>
#include <optional>
#include <tuple>

namespace sqsgen::core::helpers {

  template <class E, class... Args>
  std::expected<std::tuple<Args...>, E> combine(std::expected<Args, E>&&... vals) {
    std::optional<E> first_error;
    auto record = [&](auto&& v) {
      if (!first_error && !v.has_value()) first_error = std::move(v).error();
    };
    (record(vals), ...);

    if (first_error) return std::unexpected(std::move(*first_error));

    return std::tuple<Args...>{std::move(vals).value()...};
  }

}  // namespace sqsgen::core::helpers

#endif
