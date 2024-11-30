//
// Created by Dominik Gehringer on 30.11.24.
//

#ifndef SQSGEN_CORE_HELPERS_SORTED_VECTOR_H
#define SQSGEN_CORE_HELPERS_SORTED_VECTOR_H

#include <algorithm>
#include <ranges>
#include <vector>

namespace sqsgen::core::helpers {

  namespace ranges = std::ranges;

  template <class T, class Compare = std::less<T>> struct sorted_vector {
    std::vector<T> _values;
    Compare cmp;

    typedef typename std::vector<T>::iterator iterator;
    typedef typename std::vector<T>::const_iterator const_iterator;
    iterator begin() { return _values.begin(); }
    iterator end() { return _values.end(); }
    const_iterator begin() const { return _values.begin(); }
    const_iterator end() const { return _values.end(); }

    sorted_vector(const Compare& c = Compare()) : _values(), cmp(c) {}

    template <ranges::range R> explicit sorted_vector(R&& r, Compare const& c = Compare())
        : _values(), cmp(c) {
      for (auto element : r) insert(element);
    }
    template <class InputIterator>
    sorted_vector(InputIterator first, InputIterator last, const Compare& c = Compare())
        : _values(first, last), cmp(c) {
      std::sort(begin(), end(), cmp);
    }

    bool empty() const { return _values.empty(); }

    bool contains(const T& value) const { return find(value) != end(); }

    auto size() const { return _values.size(); }

    iterator insert(const T& t) {
      iterator i = std::lower_bound(begin(), end(), t, cmp);
      if (i == end() || cmp(t, *i)) _values.insert(i, t);
      return i;
    }

    const_iterator find(const T& t) const {
      const_iterator i = std::lower_bound(begin(), end(), t, cmp);
      return i == end() || cmp(t, *i) ? end() : i;
    }

    template <ranges::range R> void merge(R&& r) {
      ranges::for_each(r, [&](auto&& val) { insert(val); });
    }
  };
}  // namespace sqsgen::core::helpers

#endif  // SQSGEN_CORE_HELPERS_SORTED_VECTOR_H
