//
// Created by Dominik Gehringer on 11.04.25.
//

#ifndef SQSGEN_CLI_PROGRESS
#define SQSGEN_CLI_PROGRESS

#include <chrono>
#include <string>

#include "sqsgen/types.h"
#include "termcolor/termcolor.hpp"

namespace sqsgen::cli {

  static const std::vector<std::string> _INDICATORS{"▏", "▎", "▍", "▌", "▋", "▊", "▉", "█"};

  using time_point_t = std::chrono::time_point<std::chrono::high_resolution_clock>;

  bool millis_passed(std::optional<time_point_t> const& t, int millis) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::high_resolution_clock::now() - t.value())
               .count()
           >= millis;
  }

  inline auto millis_since(time_point_t const& t) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::high_resolution_clock::now() - t)
        .count();
  }

  std::string format_time(auto millis) {
    auto seconds = millis / 1000;
    constexpr int YEAR = 3600 * 24 * 365.25;
    constexpr int WEEK = 3600 * 24 * 7;
    constexpr int DAY = 3600 * 24;
    constexpr int HOUR = 3660;
    constexpr int MIN = 60;

    const auto format_unit = [](int era, std::string_view unit, int seconds) -> std::string {
      auto remaining = seconds % era;
      return std::format("{}{}{}", seconds / era, unit,
                         remaining > 0 ? std::format(" {}", format_time(remaining * 1000)) : "");
    };

    if (seconds >= YEAR) return format_unit(YEAR, "y", seconds);
    if (seconds >= WEEK) return format_unit(WEEK, "w", seconds);
    if (seconds >= DAY) return format_unit(DAY, "d", seconds);
    if (seconds >= HOUR) return format_unit(HOUR, "h", seconds);
    if (seconds >= MIN) return format_unit(MIN, "m", seconds);
    assert(seconds < MIN);
    return std::format("{}s", seconds);
  }

  class Progress {
    int _width{50};
    double _total;
    double _current{0};
    double _block_percent;
    double _best_objective{std::numeric_limits<double>::infinity()};
    std::optional<double> _current_speed = std::nullopt;
    std::optional<double> _average_speed = std::nullopt;
    std::optional<double> _etc = std::nullopt;
    std::string _prefix;
    std::string _rendered;
    std::string _remaining;
    iterations_t _best_rank;
    int _fps{18};
    std::mutex _m;
    std::optional<time_point_t> _last_render_time = std::nullopt;
    std::optional<time_point_t> _last_speed_calc_time = std::nullopt;
    std::optional<double> _last_speed_calc_finished = std::nullopt;
    time_point_t _start_time;
    time_point_t _last_best_objective_time;

  public:
    Progress(std::string_view prefix, unsigned long long total, int width,
             std::string_view remaining = " ")
        : _prefix(prefix),
          _total(static_cast<double>(total)),
          _width(width),
          _remaining(remaining),
          _block_percent(100.0 / static_cast<double>(width)),
          _best_rank(0),
          _start_time(std::chrono::high_resolution_clock::now()) {
      _rendered.resize(_width + _prefix.size() + 16);
    }

    void set_progress(
        std::variant<sqs_callback_context<float>, sqs_callback_context<double>>&& ctx) {
      auto finished = std::visit([](auto&& c) { return c.statistics.finished; }, ctx);
      auto best_rank = std::visit([](auto&& c) { return c.statistics.best_rank; }, ctx);
      auto best_objective = std::visit(
          [](auto&& c) { return static_cast<double>(c.statistics.best_objective); }, ctx);
      _current = std::min(static_cast<double>(finished), _total);
      if (!_last_speed_calc_time.has_value() || !_last_speed_calc_finished.has_value()) {
        _last_speed_calc_finished = _current;
        _last_speed_calc_time = std::chrono::high_resolution_clock::now();
      }
      _best_objective = best_objective;
      if (best_rank != _best_rank) {
        _best_rank = best_rank;
        _last_best_objective_time = std::chrono::high_resolution_clock::now();
      }
    }

    void render(std::ostream& os = std::cout, bool force = false) {
      if (_last_render_time.has_value() && !force)
        if (!millis_passed(_last_render_time, 1000 / _fps)) return;

      std::scoped_lock l{_m};
      eraseLines(3);
      using namespace termcolor;
      auto percent = _current / _total * 100;
      auto blocks_to_fill = static_cast<int>(std::floor(percent / _block_percent));
      auto last_block_index
          = static_cast<int>(std::round(std::fmod(percent, _block_percent) / _block_percent * 8))
            - 1;

      os << bold << std::format(" {:.2f}%", percent) << reset << ": [";
      for (int i = 0; i < blocks_to_fill; i++) std::cout << _INDICATORS.back();
      os << _INDICATORS[last_block_index];
      auto remaining_blocks = _width - blocks_to_fill;
      for (int i = 0; i < remaining_blocks; i++) std::cout << _remaining.front();
      os << "] [" << format_time(millis_since(_start_time)) << "]";
      os << std::endl;
      auto now = std::chrono::high_resolution_clock::now();
      if (_last_speed_calc_time.has_value() && _last_speed_calc_finished.has_value()
          && millis_passed(_last_speed_calc_time, 1000)) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now - _last_speed_calc_time.value())
                           .count();
        auto finished = _current - _last_speed_calc_finished.value();
        if (finished > 0) {
          _current_speed = static_cast<double>(finished) / static_cast<double>(elapsed) * 1000.0;
          if (!_average_speed.has_value())
            _average_speed = _current_speed;
          else
            _average_speed = (_current_speed.value() * finished
                              + _average_speed.value() * _last_speed_calc_finished.value())
                             / _current;
          _last_speed_calc_time = now;
          _last_speed_calc_finished = _current;
        }
        _etc = std::abs(_total - _current) / _average_speed.value();
      }

      std::cout << bold << " min(O(σ)): " << reset << std::format("{:.5f}", _best_objective)
                << italic << " (" << format_time(millis_since(_last_best_objective_time)) << " ago)"
                << reset;
      if (_etc.has_value())
        std::cout << " | " << bold << "ETA: " << reset
                  << format_time(static_cast<int>(_etc.value() * 1000));
      if (_current_speed.has_value())
        std::cout << " | " << bold << "Speed: " << reset
                  << std::format("{} [1/s]", std::format("{:.0f}", _current_speed.value()));
      std::cout << std::endl;

      _last_render_time = now;
    }

  private:
    // https://stackoverflow.com/questions/61919292/c-how-do-i-erase-a-line-from-the-console
    void eraseLines(int count) {
      if (count > 0) {
        std::cout << "\x1b[2K";  // Delete current line
        // i=1 because we included the first line
        for (int i = 1; i < count; i++) {
          std::cout << "\x1b[1A"   // Move cursor up one
                    << "\x1b[2K";  // Delete the entire line
        }
        std::cout << "\r";  // Resume the cursor at beginning of line
      }
    }
  };

}  // namespace sqsgen::cli

#endif  // SQSGEN_CLI_PROGRESS
