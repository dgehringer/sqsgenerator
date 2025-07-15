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
namespace spdlog {
  void error(std::string const &msg) { emscripten_console_error(msg.c_str()); };
  void warn(std::string const &msg) { emscripten_console_warn(msg.c_str()); }
  void info(std::string const &msg) { emscripten_console_log(msg.c_str()); }

  void debug(std::string const &msg) { emscripten_console_trace(msg.c_str()); }
  void trace(std::string const &msg) { emscripten_console_trace(msg.c_str()); }

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

  void set_level(level::level_enum level) {}
}  // namespace spdlog
#endif

#endif  // SQSGEN_LOG_H
