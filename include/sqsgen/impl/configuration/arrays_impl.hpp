//
// Created by Dominik Gehringer on 02.01.25.
//

#ifndef SQSGEN_IMPL_CONFIGURATION_ARRAYS_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_ARRAYS_IMPL_HPP

#include <Eigen/Core>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <expected>
#include <format>
#include <ranges>
#include <string>
#include <type_traits>
#include <variant>

#include "sqsgen/configuration/arrays.h"
#include "sqsgen/core/eigen.h"
#include "sqsgen/core/helpers/variant.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace core = sqsgen::core;
  namespace helpers = sqsgen::core::helpers;

  inline configuration_error forward_error(const std::string& key, core::tensor_shape_error err) {
    return {key, map_error_kind(err.which), err.message};
  }

  template <class T> cube_t<T> stack_matrix(const matrix_t<T>& m, std::size_t num_shells) {
    cube_t<T> result(num_shells, m.rows(), m.cols());
    for (Eigen::Index s = 0; s < static_cast<Eigen::Index>(num_shells); ++s)
      for (Eigen::Index xi = 0; xi < m.rows(); ++xi)
        for (Eigen::Index eta = 0; eta < m.cols(); ++eta) result(s, xi, eta) = m(xi, eta);
    return result;
  }

  template <class T>
  cube_t<T> stack_scalar(T value, std::size_t num_shells, std::size_t num_species) {
    cube_t<T> result(num_shells, num_species, num_species);
    result.setConstant(value);
    return result;
  }

  template <class T>
  result_t<cube_t<T>> validate_matrix(const std::string& key, const matrix_t<T>& m,
                                      std::size_t num_shells, std::size_t num_species,
                                      std::optional<std::size_t> index = std::nullopt) {
    std::string path = index.has_value() ? std::format("{}.{}", key, index.value()) : key;
    if (m.rows() != num_species || m.cols() != num_species)
      return std::unexpected(
          configuration_error{path, error_code::out_of_range,
                              std::format("matrix has invalid size {}x{} but expected {}x{}",
                                          m.rows(), m.cols(), num_species, num_species)});
    return stack_matrix(m, num_shells);
  }

  template <class T>
  result_t<cube_t<T>> validate_cube(const std::string& key, const cube_t<T>& c,
                                    std::size_t num_shells, std::size_t num_species,
                                    std::optional<std::size_t> index = std::nullopt) {
    std::string path = index.has_value() ? std::format("{}.{}", key, index.value()) : key;
    const typename cube_t<T>::Dimensions& d = c.dimensions();
    if (d[0] != num_shells || d[1] != num_species || d[2] != num_species)
      return std::unexpected(
          configuration_error{path, error_code::out_of_range,
                              std::format("array has invalid size {}x{}x{} but expected {}x{}x{}",
                                          d[0], d[1], d[2], num_shells, num_species, num_species)});
    return c;
  }

  template <class T>
  result_t<cube_t<T>> validate_array(const std::string& key, const array_spec<T>& array_spec,
                                     std::size_t num_shells, std::size_t num_species,
                                     std::optional<std::size_t> index = std::nullopt) {
    std::string path = index.has_value() ? std::format("{}.{}", key, index.value()) : key;
    return std::visit(
        helpers::overloaded{
            [&](T scalar) -> result_t<cube_t<T>> {
              return stack_scalar(scalar, num_shells, num_species);
            },
            [&](const core::nested_t<T, 2>& data) -> result_t<cube_t<T>> {
              auto matrix = core::matrix_from<T>(data);
              if (!matrix) return std::unexpected(forward_error(path, std::move(matrix).error()));
              return validate_matrix(key, matrix.value(), num_shells, num_species, index);
            },
            [&](const core::nested_t<T, 3>& data) -> result_t<cube_t<T>> {
              auto tensor = core::tensor_from<T, 3>(data);
              if (!tensor) return std::unexpected(forward_error(path, std::move(tensor).error()));
              return validate_cube(key, tensor.value(), num_shells, num_species, index);
            }},
        array_spec);
  }

  template <class T> result_t<cube_t<T>> default_pair_weights(
      const structure_t<T, sublattice_mode::interact>& structure,
      const shell_weights_t<T, sublattice_mode::interact>& weights,
      const shell_radii_t<T, sublattice_mode::interact>&) {
    Eigen::Index num_species = static_cast<Eigen::Index>(structure.num_species);
    Eigen::Index num_shells = static_cast<Eigen::Index>(weights.size());
    return stack_matrix<T>(matrix_t<T>::Constant(num_species, num_species, 1.0)
                               - matrix_t<T>::Identity(num_species, num_species),
                           num_shells);
  }

  template <class T> result_t<cube_t<T>> default_target_objective(
      const structure_t<T, sublattice_mode::interact>& structure,
      const shell_weights_t<T, sublattice_mode::interact>& weights,
      const shell_radii_t<T, sublattice_mode::interact>&) {
    return stack_scalar<T>(T(0.0), weights.size(), structure.num_species);
  }

  template <class T> result_t<cube_t<T>> default_prefactors(
      const structure_t<T, sublattice_mode::interact>& structure,
      const shell_weights_t<T, sublattice_mode::interact>& weights,
      const shell_radii_t<T, sublattice_mode::interact>& radii) {
    return core::compute_prefactors(structure, radii, weights);
  }

  template <class Fn, class T, class Element>
  concept DefaultFn = std::invocable<Fn, const structure_t<T, sublattice_mode::interact>&,
                                     const shell_weights_t<T, sublattice_mode::interact>&,
                                     const shell_radii_t<T, sublattice_mode::interact>&>
                      && std::convertible_to<
                          std::invoke_result_t<Fn, const structure_t<T, sublattice_mode::interact>&,
                                               const shell_weights_t<T, sublattice_mode::interact>&,
                                               const shell_radii_t<T, sublattice_mode::interact>&>,
                          result_t<Element>>;

  template <class T, sublattice_mode Mode, template <class, sublattice_mode> class Out,
            template <class, sublattice_mode> class In,
            DefaultFn<T, Out<T, sublattice_mode::interact>> Fn>
  result_t<Out<T, Mode>> validate_array_input(const std::string& key, const In<T, Mode>& input,
                                              Fn&& default_fn,
                                              const structure_t<T, Mode>& structure,
                                              const shell_weights_t<T, Mode>& weights,
                                              const shell_radii_t<T, Mode>& radii) {
    if constexpr (Mode == sublattice_mode::interact) {
      if (input.has_value()) {
        return validate_array(key, input.value(), weights.size(), structure.num_species);
      } else {
        return default_fn(structure, weights, radii);
      }
    } else if constexpr (Mode == sublattice_mode::split) {
      assert(structure.size() == weights.size() && structure.size() == radii.size()
                     && "validate_array split: per-sublattice vectors must be equal length "
                        "(invariant established upstream)");
      Out<T, Mode> out;
      out.reserve(structure.size());
      if (input.has_value()) {
        const auto& data = input.value();
        if (structure.size() != data.size())
          return std::unexpected(
              configuration_error{key, error_code::invalid_size,
                                  std::format("the number of {} arrays ({}) does not match "
                                              "the number of sublattices ({}).",
                                              key, data.size(), structure.size())});
        for (std::size_t index = 0; index < structure.size(); ++index) {
          auto array = validate_array(key, data[index], weights[index].size(),
                                      structure[index].num_species);
          if (!array) return std::unexpected(std::move(array).error());
          out.push_back(std::move(array).value());
        }

      } else {
        for (auto&& [s, w, r] : std::views::zip(structure, weights, radii)) {
          auto res = default_fn(s, w, r);
          if (!res) return std::unexpected(std::move(res).error());
          out.push_back(std::move(res).value());
        }
      }
      return out;
    } else
      static_assert(Mode == sublattice_mode::interact || Mode == sublattice_mode::split,
                    "unhandled sublattice_mode");
  }

  template <class T, sublattice_mode Mode>
  result_t<pair_weights_t<T, Mode>> validate_pair_weights_input(
      const pair_weights_input<T, Mode>& input, const structure_t<T, Mode>& structure,
      const shell_weights_t<T, Mode>& weights, const shell_radii_t<T, Mode>& radii) {
    return validate_array_input<T, Mode, pair_weights_t, pair_weights_input>(
        "pair_weights", input, default_pair_weights<T>, structure, weights, radii);
  }

  template <class T, sublattice_mode Mode>
  result_t<target_objective_t<T, Mode>> validate_target_objective_input(
      const target_objective_input<T, Mode>& input, const structure_t<T, Mode>& structure,
      const shell_weights_t<T, Mode>& weights, const shell_radii_t<T, Mode>& radii) {
    return validate_array_input<T, Mode, target_objective_t, target_objective_input>(
        "target_objective", input, default_target_objective<T>, structure, weights, radii);
  }

  template <class T, sublattice_mode Mode>
  result_t<prefactors_t<T, Mode>> validate_prefactors_input(const prefactors_input<T, Mode>& input,
                                                            const structure_t<T, Mode>& structure,
                                                            const shell_weights_t<T, Mode>& weights,
                                                            const shell_radii_t<T, Mode>& radii) {
    return validate_array_input<T, Mode, prefactors_t, prefactors_input>(
        "prefactors", input, default_prefactors<T>, structure, weights, radii);
  }

}  // namespace sqsgen::configuration

#endif  // SQSGEN_IMPL_CONFIGURATION_ARRAYS_IMPL_HPP
