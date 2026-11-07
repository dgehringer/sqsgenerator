//
// Created by Dominik Gehringer on 05.07.25.
//

#ifndef SQSGEN_LOG_H
#define SQSGEN_LOG_H

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <format>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>

namespace sqsgen::log {

  enum class level : std::uint8_t { trace, debug, info, warn, error, off };

  constexpr std::string_view name(level l) {
    switch (l) {
      case level::trace:
        return "trace";
      case level::debug:
        return "debug";
      case level::info:
        return "info";
      case level::warn:
        return "warning";
      case level::error:
        return "error";
      case level::off:
        return "off";
    }
    return "unknown";
  }

  using sink_fn = void (*)(level, std::string_view);

  inline void stderr_sink(level l, std::string_view message) {
    static std::mutex mutex;
    const std::string line = std::format("sqsgen {}: {}\n", name(l), message);
    std::scoped_lock lock(mutex);
    std::fwrite(line.data(), 1, line.size(), stderr);
  }

  namespace detail {
    struct settings {
      std::atomic<level> min_level{level::warn};
      std::atomic<sink_fn> sink{&stderr_sink};
    };

    inline settings& global() {
      static settings s;
      return s;
    }
  }  // namespace detail

  inline void set_level(level l) { detail::global().min_level.store(l, std::memory_order_relaxed); }

  inline level get_level() { return detail::global().min_level.load(std::memory_order_relaxed); }

  inline void set_sink(sink_fn sink) {
    detail::global().sink.store(sink != nullptr ? sink : &stderr_sink, std::memory_order_relaxed);
  }

  inline bool enabled(level l) { return l != level::off && l >= get_level(); }

  inline void write(level l, std::string_view message) {
    if (!enabled(l)) return;
    detail::global().sink.load(std::memory_order_relaxed)(l, message);
  }

  template <class... Args> void write(level l, std::format_string<Args...> fmt, Args&&... args) {
    if (!enabled(l)) return;
    const std::string message = std::format(fmt, std::forward<Args>(args)...);
    detail::global().sink.load(std::memory_order_relaxed)(l, message);
  }

  // one pair per level: plain message, or format string + arguments
  inline void trace(std::string_view message) { write(level::trace, message); }
  inline void debug(std::string_view message) { write(level::debug, message); }
  inline void info(std::string_view message) { write(level::info, message); }
  inline void warn(std::string_view message) { write(level::warn, message); }
  inline void error(std::string_view message) { write(level::error, message); }

  template <class... Args> void trace(std::format_string<Args...> fmt, Args&&... args) {
    write(level::trace, fmt, std::forward<Args>(args)...);
  }
  template <class... Args> void debug(std::format_string<Args...> fmt, Args&&... args) {
    write(level::debug, fmt, std::forward<Args>(args)...);
  }
  template <class... Args> void info(std::format_string<Args...> fmt, Args&&... args) {
    write(level::info, fmt, std::forward<Args>(args)...);
  }
  template <class... Args> void warn(std::format_string<Args...> fmt, Args&&... args) {
    write(level::warn, fmt, std::forward<Args>(args)...);
  }
  template <class... Args> void error(std::format_string<Args...> fmt, Args&&... args) {
    write(level::error, fmt, std::forward<Args>(args)...);
  }

}  // namespace sqsgen::log

#endif  // SQSGEN_LOG_H
