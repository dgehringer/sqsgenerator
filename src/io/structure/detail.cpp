
#include <expected>

#include "sqsgen/io/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::io::detail {
  template <class T> std::array<T, 3> lengths(lattice_t<T> const& m) {
    return {
        m.row(0).norm(),
        m.row(1).norm(),
        m.row(2).norm(),
    };
  }

  template <class T> T clip(T val, T lower, T upper) {
    if (val < lower) return lower;
    if (val > upper) return upper;
    return val;
  }

  template <class T> std::array<T, 3> angles(lattice_t<T> const& m) {
    const auto l = lengths(m);
    const auto angle = [&](auto dim) -> T {
      auto i = (dim + 1) % std::size(l);
      auto j = (dim + 2) % std::size(l);
      return std::acos(clip(m.row(i).dot(m.row(j)) / (l[i] * l[j]))) * 180.0 / M_PI;
    };
    return {angle(0), angle(1), angle(2)};
  }

  inline std::string rjust(const std::string& input, std::size_t width, char fillchar) {
    if (input.size() >= width) return input;
    return std::string(width - input.size(), fillchar) + input;
  }

  inline std::vector<std::string_view> split(std::string_view str, std::string_view delimeters) {
    std::vector<std::string_view> res;
    res.reserve(str.length() / 2);
    const char* ptr = str.data();

    size_t size = 0;

    for (const char c : str) {
      for (const char d : delimeters) {
        if (c == d) {
          res.emplace_back(ptr, size);
          ptr += size + 1;
          size = 0;
          goto next;
        }
      }
      ++size;
    next:
      continue;
    }

    if (size) res.emplace_back(ptr, size);
    std::erase_if(res, [](auto&& s) { return s.empty(); });
    return res;
  }

  template <class T> std::expected<T, parser_error> parse_number(std::string_view view) {
    std::string input{view};
    try {
      if constexpr (std::is_same_v<T, float>) {
        return std::stof(input);
      }
      if constexpr (std::is_same_v<T, double>) {
        return std::stod(input);
      }
      if constexpr (std::is_same_v<T, int>) {
        return std::stoi(input);
      }
      if constexpr (std::is_same_v<T, long>) {
        return std::stol(input);
      }
      if constexpr (std::is_same_v<T, unsigned long>) {
        return std::stoul(input);
      }
    } catch (const std::invalid_argument& e) {
      return std::unexpected(parser_error{error_code::bad_argument, e.what()});
    } catch (const std::out_of_range& e) {
      return std::unexpected(parser_error{error_code::out_of_range, e.what()});
    }
    return std::unexpected(parser_error{error_code::unknown, "unknown error"});
  }

  template <> std::expected<float, parser_error> parse_number(std::string_view view);
  template <> std::expected<double, parser_error> parse_number(std::string_view view);

  template <> std::array<float, 3> angles(lattice_t<float> const& m);
  template <> std::array<double, 3> angles(lattice_t<double> const& m);

  template <> std::array<float, 3> lengths(lattice_t<float> const& m);
  template <> std::array<double, 3> lengths(lattice_t<double> const& m);

  template <> float clip(float val, float lower, float upper);
  template <> double clip(double val, double lower, double upper);
}  // namespace sqsgen::io::detail
