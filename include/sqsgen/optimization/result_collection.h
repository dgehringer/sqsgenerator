#ifndef SQSGEN_OPTIMIZATION_RESULT_COLLECTION_H
#define SQSGEN_OPTIMIZATION_RESULT_COLLECTION_H

#include <ankerl/unordered_dense.h>

#include <cstddef>
#include <map>
#include <mutex>
#include <vector>

#include "sqsgen/optimization/result.h"

namespace sqsgen::optimization {

  template <class T, sublattice_mode Mode> class sqs_result_collection {
    using result_t = sqs_result<T, Mode>;
    using bucket_t = ankerl::unordered_dense::set<result_t, sqs_result_hash>;

    struct bucket {
      mutable std::mutex mutex;
      bucket_t results;
    };

  public:
    sqs_result_collection() = default;

    bool insert(result_t&& r);

    void merge(sqs_result_collection&& other);

    std::size_t count_for_objective(T objective) const;

    [[nodiscard]] T nth_best_objective(std::size_t n) const;

    [[nodiscard]] std::size_t num_results() const;

    std::vector<std::pair<T, std::vector<result_t>>> snapshot() const;

  private:
    bucket* bucket_for(T key);

    mutable std::mutex structure_mutex_;
    std::map<T, std::unique_ptr<bucket>> buckets_;
  };

};  // namespace sqsgen::optimization

#endif
