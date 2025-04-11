//
// Created by Dominik Gehringer on 11.04.25.
//

#ifndef SQSGEN_CLI_PROGRESS
#define SQSGEN_CLI_PROGRESS

#include <chrono>
#include <string>

namespace sqsgen::cli {

  static const std::vector<std::string> _INDICATORS{"▏", "▎", "▍", "▌", "▋", "▊", "▉", "█"};

  class Progress {
    int _width{50};
    double _total;
    double _current{0};
    double _block_percent;
    std::string _prefix;
    std::string _rendered;
    std::string _remaining;
    int _fps{37};
    std::mutex _m;
    std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> _last_render_time
        = std::nullopt;

  public:
    Progress(std::string_view prefix, unsigned long long total, int width,
             std::string_view remaining = "-")
        : _prefix(prefix),
          _total(static_cast<double>(total)),
          _width(width),
          _remaining(remaining),
          _block_percent(100.0 / static_cast<double>(width)) {
      _rendered.resize(_width + _prefix.size() + 16);
    }

    void set_progress(unsigned long long int current) {
      _current = std::min(static_cast<double>(current), _total);
    }

    void rendered(std::ostream& os = std::cout, bool force = false) {
      if (_last_render_time.has_value() && !force) {
        auto time_since_last_render = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - _last_render_time.value());
        if (time_since_last_render.count() < 1000 / _fps) return;
      }
      std::scoped_lock l{_m};
      eraseLines(2);
      auto percent = _current / _total * 100;
      auto blocks_to_fill = static_cast<int>(std::floor(percent / _block_percent));
      auto last_block_index
          = static_cast<int>(std::round(std::fmod(percent, _block_percent) / _block_percent * 8))
            - 1;

      os << _prefix << ": [";
      for (int i = 0; i < blocks_to_fill; i++) std::cout << _INDICATORS.back();
      os << _INDICATORS[last_block_index];
      auto remaining_blocks = _width - blocks_to_fill - 1;
      for (int i = 0; i < remaining_blocks; i++) std::cout << _remaining.front();
      os << "]";
      os << std::format(" {:.2f}%", percent);
      os << std::endl;
      _last_render_time = std::chrono::high_resolution_clock::now();
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
