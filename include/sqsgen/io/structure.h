//
// Created by Dominik Gehringer on 04.03.25.
//

#ifndef SQSGEN_IO_STRUCTURE_H
#define SQSGEN_IO_STRUCTURE_H

#include <expected>
#include <numeric>
#include <ranges>

#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"

// PI is not defined on standard C++ headers when using MSVC
#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

namespace sqsgen::io {
  namespace ranges = std::ranges;
  namespace views = ranges::views;
  namespace core = sqsgen::core;

  namespace detail {

    template <ranges::range Range>
      requires std::is_same_v<ranges::range_value_t<Range>, std::string>
    std::string join(Range&& crumbs, std::string const& delimiter = " ") {
      auto csize = ranges::size(crumbs);
      if (csize == 0) return "";
      std::string result;
      auto total_size
          = std::accumulate(crumbs | views::transform([](auto&& s) { return s.size(); }), 0);
      result.reserve(total_size + (csize - 1) * delimiter.size());
      for (auto it = ranges::begin(crumbs); it != ranges::end(crumbs); ++it) {
        if (it != ranges::begin(crumbs)) result.append(delimiter);
        result.append(*it);
      }
      result.shrink_to_fit();
      return result;
    }

    template <class T> std::array<T, 3> lengths(lattice_t<T> const& m);

    template <class T> T clip(T val, T lower = -1.0, T upper = 1.0);

    template <class T> std::array<T, 3> angles(lattice_t<T> const& m);

    inline std::string rjust(const std::string& input, std::size_t width, char fillchar = ' ');
    inline std::vector<std::string_view> split(std::string_view str, std::string_view delimeters);

    template <class T> std::expected<T, parser_error> parse_number(std::string_view view);

  }  // namespace detail

  template <class, structure_format> struct structure_adapter {};

  template <class T> struct structure_adapter<T, structure_format::pymatgen> {
    static std::string format(const core::structure<T>& structure,
                              wire_format format = wire_format::json);

    static std::expected<core::structure<T>, parser_error> read(std::string_view data,
                                                                wire_format format
                                                                = wire_format::json);
  };

  template <class T> struct structure_adapter<T, structure_format::sqsgen> {
    static std::string format(const core::structure<T>& structure,
                              wire_format format = wire_format::json);

    static std::expected<core::structure<T>, parser_error> read(std::string_view data,
                                                                wire_format format
                                                                = wire_format::json);
  };

  template <class T> struct structure_adapter<T, structure_format::poscar> {
    static std::string format(const core::structure<T>& structure);
    static std::expected<core::structure<T>, parser_error> read(const std::string& input);
  };

  template <class T> struct structure_adapter<T, structure_format::cif> {
    static std::string format(const core::structure<T>& structure);
  };

  template <class T> struct structure_adapter<T, structure_format::pdb> {
    static std::string format(const core::structure<T>& structure);
  };

  template <class T> struct structure_adapter<T, structure_format::ase> {
    static std::string format(const core::structure<T>& structure,
                              wire_format format = wire_format::json);
  };

}  // namespace sqsgen::io
#endif  // SQSGEN_IO_STRUCTURE_H
