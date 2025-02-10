//
// Created by Dominik Gehringer on 19.01.25.
//

#ifndef SQSGEN_OPTIMIZATION_STATISTICS_H
#define SQSGEN_OPTIMIZATION_STATISTICS_H

#include <chrono>
#include "sqsgen/types.h"
#include "sqsgen/core/helpers.h"

namespace sqsgen::optimization {
  using namespace core::helpers;

  template <string_literal> struct tick {
    std::chrono::high_resolution_clock::time_point now{std::chrono::steady_clock::now()};
  };

  template <class T> class sqs_statistics {
    /**
     * Thin wrapper class around sqsgen::sqs_statistics_data to make insertions threadsafe
     */
    std::mutex _mutex_timing;
    std::mutex _mutex_history;
    sqs_statistics_data<T> _data;

  public:
    void merge(sqs_statistics&& other) {
      {
        std::scoped_lock l{_mutex_history};
        _data.history.insert(other._data.history.begin(), other._data.history.end());
      }
      {
        std::scoped_lock l{_mutex_timing};
        for (auto const& [what, nanoseconds] : other._data.timings)
          _data.timings[what] += nanoseconds;
      }
    }

    void log_result(iterations_t iteration, T objective) {
      std::scoped_lock l{_mutex_history};
      _data.timings.emplace(iteration, objective);
    }

    template <string_literal Name> void tock(tick<Name>&& t) {
      std::scoped_lock l{_mutex_timing};
      _data[Name.data] += std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::steady_clock::now() - t.now)
                              .count();
    }
  };

}  // namespace sqsgen::optimization

#endif  // SQSGEN_OPTIMIZATION_STATISTICS_H
