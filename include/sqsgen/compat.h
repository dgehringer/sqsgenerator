//
// Created by Dominik Gehringer on 05.07.25.
//

#ifndef SQSGEN_LOG_H
#define SQSGEN_LOG_H

#ifdef __EMSCRIPTEN__
#  include <emscripten/console.h>
#else
#  include <spdlog/spdlog.h>
#endif

#ifdef __EMSCRIPTEN__
#  include <format>
#  define format_ std::format
#else
#  include <fmt/format.h>
#  define format_ fmt::format
#endif

#ifdef __EMSCRIPTEN__
namespace spdlog {

  namespace level {
    enum level_enum : int {
      trace = 0,
      debug = 1,
      info = 2,
      warn = 3,
      err = 4,
      critical = 5,
      off = -1,
      n_levels
    };
  }

  static std::atomic __current_level{level::info};

  inline void error(std::string const &msg) {
    /*if (__current_level.load(std::memory_order_relaxed) >= level::critical)
      emscripten_console_error(msg.c_str());*/
  };
  inline void warn(std::string const &msg) {
    /*if (__current_level.load(std::memory_order_relaxed) >= level::warn)
      emscripten_console_warn(msg.c_str());*/
  }
  inline void info(std::string const &msg) {
    /*if (__current_level.load(std::memory_order_relaxed) >= level::info)
      emscripten_console_log(msg.c_str());*/
  }

  inline void debug(std::string const &msg) {
    /*if (__current_level.load(std::memory_order_relaxed) >= level::debug)
      emscripten_console_trace(msg.c_str());*/
  }
  inline void trace(std::string const &msg) {
    /*if (__current_level.load(std::memory_order_relaxed) >= level::trace)
      emscripten_console_trace(msg.c_str());*/
  }

  inline void set_level(level::level_enum level) { __current_level.store(level); }
}  // namespace spdlog

#endif

#endif  // SQSGEN_LOG_H
