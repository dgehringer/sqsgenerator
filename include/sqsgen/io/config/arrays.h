//
// Created by Dominik Gehringer on 02.01.25.
//

#ifndef SQSGEN_IO_CONFIG_ARRAYS_H
#define SQSGEN_IO_CONFIG_ARRAYS_H

#include <ranges>

#include "arrays.h"
#include "shared.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/config/shared.h"
#include "sqsgen/io/parsing.h"

namespace sqsgen::io::config {
  namespace ranges = std::ranges;
  namespace views = ranges::views;

  namespace detail {
    template <class T> using array_t = std::vector<cube_t<T>>;

    template <string_literal key, class T>
    parse_result<matrix_t<T>> parse_stl(stl_matrix_t<T> const& m) {
      try {
        return stl_to_eigen<matrix_t<T>>(m);
      } catch (std::invalid_argument const& e) {
        return parse_error::from_msg<key, CODE_BAD_VALUE>(
            format("Could not convert to 2D array - %s", e.what()));
      }
    }

    template <string_literal key, class T>
    parse_result<cube_t<T>> parse_stl(stl_cube_t<T> const& c) {
      try {
        return stl_to_eigen<cube_t<T>>(c);
      } catch (std::invalid_argument const& e) {
        return parse_error::from_msg<key, CODE_BAD_VALUE>(
            format("Could not convert to 3D array - %s", e.what()));
      }
    }

    template <class T> cube_t<T> stack_matrix(matrix_t<T>&& m, auto num_shells) {
      cube_t<T> result(num_shells, m.rows(), m.cols());
      for_each([&](auto&& s, auto&& xi, auto&& eta) { result(s, xi, eta) = m(xi, eta); },
               num_shells, m.rows(), m.cols());
      return result;
    }

    template <class T> cube_t<T> stack_scalar(T value, auto num_shells, auto num_species) {
      cube_t<T> result(num_shells, num_species, num_species);
      result.setConstant(value);
      return result;
    }

    template <string_literal key, class T>
    parse_result<cube_t<T>> parse_matrix(matrix_t<T>&& m, auto num_shells, auto num_species) {
      if (m.rows() != num_species || m.cols() != num_species)
        return parse_error::from_msg<key, CODE_BAD_VALUE>(
            format("matrix has invalid size %ix%i but expected %ix%i", m.rows(), m.cols(),
                   num_species, num_species));
      return stack_matrix(std::forward<matrix_t<T>>(m), num_shells);
    }

    template <string_literal key, class T>
    parse_result<cube_t<T>> parse_cube(cube_t<T>&& c, auto num_shells, auto num_species) {
      const typename cube_t<T>::Dimensions& d = c.dimensions();
      if (d[0] != num_shells || d[1] != num_species || d[2] != num_species)
        return parse_error::from_msg<key, CODE_BAD_VALUE>(
            format("array has invalid size %ix%ix%i but expected %ix%ix%i", d[0], d[1], d[2],
                   num_shells, num_species, num_species));
      return c;
    }

    template <string_literal key, class T, class Document>
    parse_result<cube_t<T>> parse_array_generic(Document const& document, auto num_shells,
                                                auto num_species) {
      using result_t = parse_result<cube_t<T>>;
      auto r = get_either<KEY_NONE, T, stl_matrix_t<T>, stl_cube_t<T>, matrix_t<T>, cube_t<T>>(
          document);
      if (r.failed()) return r.error().with_key(key.data);
      return r.template collapse<cube_t<T>>(
          [&](T&& val) -> result_t { return detail::stack_scalar(val, num_shells, num_species); },
          [&](stl_matrix_t<T>&& raw) -> result_t {
            return parse_stl<key>(raw).and_then([&](auto&& m) -> result_t {
              return parse_matrix<key>(std::forward<matrix_t<T>>(m), num_shells, num_species);
            });
          },
          [&](stl_cube_t<T>&& raw) -> result_t {
            return parse_stl<key>(raw).and_then([&](auto&& c) -> result_t {
              return parse_cube<key>(std::forward<cube_t<T>>(c), num_shells, num_species);
            });
          },
          [&](matrix_t<T>&& m) -> result_t {
            return parse_matrix<key>(std::forward<matrix_t<T>>(m), num_shells, num_species);
          },
          [&](cube_t<T>&& c) -> result_t {
            return parse_cube<key>(std::forward<cube_t<T>>(c), num_shells, num_species);
          });
    }

    template <class T> cube_t<T> default_pair_weights(auto num_shells, auto num_species) {
      return stack_matrix<T>(matrix_t<T>::Constant(num_species, num_species, 1.0)
                                 - matrix_t<T>::Identity(num_species, num_species),
                             num_shells);
    }

    template <class T> std::vector<core::structure<T>> decompose_structure(
        core::structure<T>&& structure, std::vector<sublattice> const& composition) {
      return core::helpers::as<std::vector>{}(
          composition | views::transform([&](auto&& sl) { return structure.sliced(sl.sites); }));
    }

    template <string_literal key, class T, class Document, class DefaultFn, ranges::range... R>
    parse_result<array_t<T>> parse_array_with_default(
        DefaultFn&& fn, Document const& document, SublatticeMode mode,
        core::structure<T>&& structure, std::vector<sublattice> const& composition,
        std::vector<shell_weights_t<T>> const& shell_weights, R&&... r) {
      using result_t = parse_result<array_t<T>>;
      auto value_present = accessor<Document>::contains(document, key.data);
      auto defaults = [&]<class... RT>(auto&& st, auto&& w, RT&&... rv) -> parse_result<cube_t<T>> {
        return fn(std::forward<core::structure<T>>(st), w, rv...);
      };
      auto parser = [&](auto&& subdoc, auto&& st, auto&& w) -> parse_result<cube_t<T>> {
        return detail::parse_array_generic<key, T>(subdoc, w.size(), st.num_species);
      };

      return parse_for_mode<key>(
          [&]() -> result_t {
            auto structs = std::vector{structure.apply_composition(composition)};
            if (value_present) {
              auto doc = accessor<Document>::get(document, key.data);
              return lift<key>(parser, std::vector{doc}, structs, shell_weights);
            }
            return lift<key>(defaults, structs, shell_weights, r...);
          },
          [&]() -> result_t {
            auto structs = structure.apply_composition_and_decompose(composition);
            if (value_present) {
              auto doc = accessor<Document>::get(document, key.data);
              using accessor_t = accessor<std::decay_t<decltype(doc)>>;
              if (!accessor_t::is_list(doc))
                return parse_error::from_msg<key, CODE_TYPE_ERROR>(
                    "You have specified \"split\" mode. In split mode array type parameter must be "
                    "specified per sublattice");
              return lift<key>(parser, accessor_t::range(doc), structs, shell_weights);
            }
            return lift<key>(defaults, structs, shell_weights, r...);
          },
          mode);
    }
  }  // namespace detail

  template <string_literal key, class T, class Document>
  parse_result<detail::array_t<T>> parse_prefactors(
      Document const& document, SublatticeMode mode, core::structure<T>&& structure,
      std::vector<sublattice> const& composition, stl_matrix_t<T> const& shell_radii,
      std::vector<shell_weights_t<T>> const& shell_weights) {
    return detail::parse_array_with_default<key, T>(
        [](auto&& st, auto&& w, auto&& radii) -> parse_result<cube_t<T>> {
          return core::compute_prefactors(std::forward<core::structure<T>>(st), radii, w);
        },
        document, mode, std::forward<core::structure<T>>(structure), composition, shell_weights,
        shell_radii);
  }

  template <string_literal key, class T, class Document>
  parse_result<detail::array_t<T>> parse_pair_weights(
      Document const& document, SublatticeMode mode, core::structure<T>&& structure,
      std::vector<sublattice> const& composition,
      std::vector<shell_weights_t<T>> const& shell_weights) {
    return detail::parse_array_with_default<key, T>(
        [](auto&& st, auto&& w) -> parse_result<cube_t<T>> {
          return detail::default_pair_weights<T>(w.size(), st.num_species);
        },
        document, mode, std::forward<core::structure<T>>(structure), composition, shell_weights);
  }

  template <string_literal key, class T, class Document>
  parse_result<detail::array_t<T>> parse_target_objective(
      Document const& document, SublatticeMode mode, core::structure<T>&& structure,
      std::vector<sublattice> const& composition,
      std::vector<shell_weights_t<T>> const& shell_weights) {
    return detail::parse_array_with_default<key, T>(
        [](auto&& st, auto&& w) -> parse_result<cube_t<T>> {
          return detail::stack_scalar<T>(0.0, w.size(), st.num_species);
        },
        document, mode, std::forward<core::structure<T>>(structure), composition, shell_weights);
  }

}  // namespace sqsgen::io::config

#endif  // SQSGEN_IO_CONFIG_ARRAYS_H
