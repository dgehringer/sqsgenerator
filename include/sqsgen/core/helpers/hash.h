//
// Created by Dominik Gehringer on 04.11.24.
//

#ifndef SQSGEN_CORE_HELPERS_HASH_H
#define SQSGEN_CORE_HELPERS_HASH_H

namespace sqsgen::core::helpers {

  template <class T> void hash_combine(std::size_t& s, const T& v) {
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
  }

}  // namespace sqsgen::core::helpers

#endif  // SQSGEN_CORE_HELPERS_HASH_H
