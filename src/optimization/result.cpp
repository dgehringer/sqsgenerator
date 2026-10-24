
#include "sqsgen/optimization/result.h"

#include "sqsgen/core/helpers/inst.h"
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

#define INST_SQS_RESULT(PREC, SMODE) template struct sqs_result<PREC, SMODE>;
  SQSGEN_INSTANTIATE_PREC_SMODE(INST_SQS_RESULT)
#undef INST_SQS_RESULT

};  // namespace sqsgen::optimization
