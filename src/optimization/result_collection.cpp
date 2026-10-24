
#include "sqsgen/optimization/result_collection.h"

#include <limits>

#include "sqsgen/core/helpers/inst.h"

namespace sqsgen::optimization {

  template <class T, sublattice_mode Mode>
  bool sqs_result_collection<T, Mode>::insert(result_t&& r) {
    const T key = r.total_objective();
    bucket* b = bucket_for(key);
    std::scoped_lock lock(b->mutex);
    return b->results.insert(std::move(r)).second;
  }

  template <class T, sublattice_mode Mode>
  void sqs_result_collection<T, Mode>::merge(sqs_result_collection&& other) {
    // lock the source wholesale (it is no longer being written) and re-insert.
    std::scoped_lock src_lock(other.structure_mutex_);
    for (auto& [key, src_bucket] : other.buckets_) {
      bucket* dst = bucket_for(key);
      std::scoped_lock dst_lock(dst->mutex);
      for (auto it = src_bucket->results.begin(); it != src_bucket->results.end(); ++it)
        dst->results.insert(std::move(const_cast<result_t&>(*it)));  // set iters are const
    }
  }

  template <class T, sublattice_mode Mode>
  std::size_t sqs_result_collection<T, Mode>::count_for_objective(T objective) const {
    std::scoped_lock lock(structure_mutex_);
    auto it = buckets_.find(objective);
    if (it == buckets_.end()) return 0;
    std::scoped_lock bl(it->second->mutex);
    return it->second->results.size();
  }

  template <class T, sublattice_mode Mode>
  T sqs_result_collection<T, Mode>::nth_best_objective(std::size_t n) const {
    std::scoped_lock lock(structure_mutex_);
    if (n >= buckets_.size()) return std::numeric_limits<T>::infinity();
    auto it = buckets_.begin();
    std::advance(it, n);
    return it->first;
  }

  template <class T, sublattice_mode Mode>
  std::size_t sqs_result_collection<T, Mode>::num_results() const {
    std::scoped_lock lock(structure_mutex_);
    std::size_t total = 0;
    for (auto const& [key, b] : buckets_) {
      std::scoped_lock bl(b->mutex);
      total += b->results.size();
    }
    return total;
  }
  template <class T, sublattice_mode Mode>
  std::vector<std::pair<T, std::vector<typename sqs_result_collection<T, Mode>::result_t>>>
  sqs_result_collection<T, Mode>::snapshot() const {
    std::scoped_lock lock(structure_mutex_);
    std::vector<std::pair<T, std::vector<result_t>>> out;
    out.reserve(buckets_.size());
    for (auto const& [key, b] : buckets_) {  // map iterates in objective order
      std::scoped_lock bl(b->mutex);
      out.emplace_back(key, std::vector<result_t>(b->results.begin(), b->results.end()));
    }
    return out;
  }

  template <class T, sublattice_mode Mode>
  sqs_result_collection<T, Mode>::bucket* sqs_result_collection<T, Mode>::bucket_for(T key) {
    std::scoped_lock lock(structure_mutex_);
    auto it = buckets_.find(key);
    if (it == buckets_.end()) it = buckets_.emplace(key, std::make_unique<bucket>()).first;
    return it->second.get();
  }

#define INST_SQS_RESULT_COLLECTION(PREC, SMODE) template struct sqs_result_collection<PREC, SMODE>;
  SQSGEN_INSTANTIATE_PREC_SMODE(INST_SQS_RESULT_COLLECTION)
#undef INST_SQS_RESULT

};  // namespace sqsgen::optimization
