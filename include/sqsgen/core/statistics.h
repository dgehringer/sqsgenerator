//
// Created by Dominik Gehringer on 19.01.25.
//

#ifndef SQSGEN_CORE_STATISTICS_H
#define SQSGEN_CORE_STATISTICS_H

#include "sqsgen/core/helpers.h"
#include "sqsgen/types.h"

namespace sqsgen::core {
  using namespace std::chrono;
  using namespace core::helpers;

  template <Timing> struct tick {
    high_resolution_clock::time_point now{steady_clock::now()};
  };

  template <class T> class sqs_statistics {
    /**
     * Thin wrapper threadsafe wrapper class around sqsgen::sqs_statistics_data to make insertions
     * safe
     */
    std::mutex _mutex_timing;
    std::atomic<iterations_t> _finished{0};
    std::atomic<iterations_t> _working{0};
    std::atomic<iterations_t> _best_rank{0};
    std::atomic<T> _best_objective{std::numeric_limits<T>::infinity()};
    sqs_statistics_data<T> _data{};

  public:
    sqs_statistics() = default;
    explicit sqs_statistics(sqs_statistics_data<T> data)
        : _data(data),
          _finished(data.finished),
          _working(data.working),
          _best_rank(data.best_rank),
          _best_objective(data.best_objective) {}

    void merge(sqs_statistics_data<T>&& other) {
      {
        std::scoped_lock l{_mutex_timing};
        _data.finished += other.finished;
        for (auto const& [what, nanoseconds] : other.timings) _data.timings[what] += nanoseconds;
      }
      _finished.fetch_add(other.finished);
      _working.fetch_add(other.working);
      if (other.best_objective < _best_objective.load()) {
        _best_objective.exchange(other.best_objective);
        _best_rank.exchange(other.best_rank);
      }
    }

    void log_result(iterations_t iteration, T objective) {
      if (objective < _best_objective.load()) {
        _best_objective.exchange(objective);
        _best_rank.exchange(iteration);
      }
    }

    template <Timing Time> void tock(tick<Time> t) {
      std::scoped_lock l{_mutex_timing};
      _data.timings[Time]
          += std::chrono::duration_cast<nanoseconds>(steady_clock::now() - t.now).count();
    }

    iterations_t add_working(long long finished) { return _working.fetch_add(finished); }

    iterations_t add_finished(iterations_t finished) { return _finished.fetch_add(finished); }

    sqs_statistics_data<T> data() {
      _data.finished = _finished.load();
      _data.working = _working.load();
      _data.best_objective = _best_objective.load();
      _data.best_rank = _best_rank.load();
      return _data;
    }
  };

}  // namespace sqsgen::optimization

#endif  // SQSGEN_CORE_STATISTICS_H
