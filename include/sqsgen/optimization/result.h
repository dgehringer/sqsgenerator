#ifndef SQSGEN_OPTIMIZATION_RESULT_H
#define SQSGEN_OPTIMIZATION_RESULT_H

#include <ankerl/unordered_dense.h>

#include <cstddef>

#include "sqsgen/types.h"

namespace sqsgen::optimization {

  template <class T, sublattice_mode Mode> struct sqs_result {
    sublattice_mode_nesting<T, Mode> objective;
    sublattice_mode_nesting<configuration_t, Mode> configuration;
    sublattice_mode_nesting<cube_t<T>, Mode> sro;

    bool operator==(const sqs_result& o) const { return configuration == o.configuration; }

    [[nodiscard]] T total_objective() const;
  };

  struct sqs_result_hash {
    template <class T, sublattice_mode Mode>
    std::size_t operator()(const sqs_result<T, Mode>& r) const noexcept {
      if constexpr (Mode == sublattice_mode::interact) {
        const auto& c = r.configuration;
        return ankerl::unordered_dense::detail::wyhash::hash(c.data(), c.size());
      } else {
        std::size_t seed = 0;
        for (const auto& sublattice : r.configuration)
          seed = hash_combine(seed, ankerl::unordered_dense::detail::wyhash::hash(
                                        sublattice.data(), sublattice.size()));
        return seed;
      }
    }

  private:
    // 64-bit hash combine (boost-style, golden-ratio constant)
    static std::size_t hash_combine(std::size_t seed, std::size_t value) noexcept {
      return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
    }
  };
};  // namespace sqsgen::optimization

#include "sqsgen/impl/optimization/result_impl.hpp"

#endif
