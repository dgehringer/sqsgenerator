
#include "sqsgen/optimization/result.h"

#include "sqsgen/types.h"

namespace sqsgen::optimization {

  template <class T, sublattice_mode Mode> T sqs_result<T, Mode>::total_objective() const {
    if constexpr (Mode == sublattice_mode::interact) {
      return objective;
    } else if constexpr (Mode == sublattice_mode::split) {
      return std::accumulate(objective.begin(), objective.end(), T{0.0});
    } else {
      static_assert(Mode == sublattice_mode::interact || Mode == sublattice_mode::split,
                    "unhandled sublattice_mode");
    }
  }

  template <> struct sqs_result<float, sublattice_mode::interact>;
  template <> struct sqs_result<double, sublattice_mode::interact>;
  template <> struct sqs_result<float, sublattice_mode::split>;
  template <> struct sqs_result<double, sublattice_mode::split>;

};  // namespace sqsgen::optimization
