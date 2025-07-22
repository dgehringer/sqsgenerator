//
// Created by Dominik Gehringer on 05.07.25.
//

#ifndef SQSGEN_LOG_H
#define SQSGEN_LOG_H

#include "absl/strings/str_format.h"

namespace sqsgen {

  template <class... Args> std::string format(auto const& fmt, Args&&... args) {
    return absl::StrFormat(fmt, std::forward<Args>(args)...);
  }

  namespace log {}
}  // namespace sqsgen

#endif  // SQSGEN_LOG_H
