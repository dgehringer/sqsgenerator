//
// Created by Dominik Gehringer on 05.07.25.
//

#ifndef SQSGEN_LOG_H
#define SQSGEN_LOG_H

#include "absl/base/log_severity.h"
#include "absl/log/absl_log.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/strings/str_format.h"

namespace sqsgen {

  template <class... Args>
  std::string format_string(const absl::FormatSpec<Args...>& fmt, Args&&... args) {
    return absl::StrFormat(fmt, std::forward<Args>(args)...);
  }

  namespace log {

    enum class level : int {
      info = static_cast<int>(absl::LogSeverity::kInfo),
      warn = static_cast<int>(absl::LogSeverity::kWarning),
      error = static_cast<int>(absl::LogSeverity::kError),

    };

    inline void set_level(level level) {
      absl::SetMinLogLevel(static_cast<absl::LogSeverityAtLeast>(level));
    }

    inline void info(auto const& message) { ABSL_LOG(INFO) << message; }
    inline void warn(auto const& message) { ABSL_LOG(WARNING) << message; }
    inline void error(auto const& message) { ABSL_LOG(ERROR) << message; }

    inline void debug(auto const& message) { VLOG(4) << message; }
    inline void trace(auto const& message) { VLOG(5) << message; }

    inline void initialize() { absl::InitializeLog(); }
  }  // namespace log
}  // namespace sqsgen

#endif  // SQSGEN_LOG_H
