//
// Created by Dominik Gehringer on 05.11.24.
//

#ifndef SQSGEN_IMPL_CONFIGURATION_CONFIGURATION_DTO_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_CONFIGURATION_DTO_IMPL_HPP

#include <expected>
#include <ranges>
#include <vector>

#include "sqsgen/configuration.h"
#include "sqsgen/core/eigen.h"
#include "sqsgen/types.h"

#define SQSGEN_TRY_ASSIGN(var, ...)                                                   \
  auto&& _res_##var = (__VA_ARGS__);                                                  \
  if (!_res_##var.has_value()) return std::unexpected(std::move(_res_##var.error())); \
  auto var = std::move(_res_##var.value());

namespace sqsgen::configuration {

  namespace core = sqsgen::core;

  inline io::parser_error forward_error(core::tensor_shape_error err) {
    return {map_error_kind(err.which), err.message};
  }

  template <core::array_mode AMode, sublattice_mode Mode, class T, int N>
  sublattice_mode_nesting<core::tensor_representation_t<T, N, AMode>, Mode> tensor_to_dto(
      const sublattice_mode_nesting<Eigen::Tensor<T, N>, Mode>& data) {
    if constexpr (Mode == sublattice_mode::interact) {
      return core::tensor_as<AMode>(data);
    } else if constexpr (Mode == sublattice_mode::split) {
      return data | std::ranges::views::transform([](const Eigen::Tensor<T, N>& tensor) {
               return core::tensor_as<AMode>(tensor);
             })
             | std::ranges::to<std::vector>();
    } else {
      static_assert(Mode == sublattice_mode::interact || Mode == sublattice_mode::split);
    }
  }

  template <core::array_mode AMode, sublattice_mode Mode, class T, int N>
  io::result_t<sublattice_mode_nesting<Eigen::Tensor<T, N>, Mode>> tensor_from_dto(
      const sublattice_mode_nesting<core::tensor_representation_t<T, N, AMode>, Mode>& data) {
    if constexpr (Mode == sublattice_mode::interact) {
      auto tensor = core::tensor_from<AMode, T, N>(data);
      if (!tensor) return std::unexpected(forward_error(std::move(tensor).error()));
      return std::move(tensor).value();
    } else if constexpr (Mode == sublattice_mode::split) {
      sublattice_mode_nesting<Eigen::Tensor<T, N>, Mode> out;
      out.reserve(data.size());
      for (const core::tensor_representation_t<T, N, AMode>& element : data) {
        auto tensor = core::tensor_from<AMode, T, N>(element);
        if (!tensor) return std::unexpected(forward_error(std::move(tensor).error()));
        out.push_back(std::move(tensor).value());
      }
      return out;
    } else {
      static_assert(Mode == sublattice_mode::interact || Mode == sublattice_mode::split);
    }
  }

  template <class T, sublattice_mode Mode> template <core::array_mode AMode>
  configuration_dto<T, Mode, AMode> configuration<T, Mode>::to_dto() const {
    return configuration_dto<T, Mode, AMode>{
        .sublattice_mode = sublattice_mode,
        .prec = prec,
        .atol = atol,
        .rtol = rtol,
        .bin_width = bin_width,
        .peak_isolation = peak_isolation,
        .structure = structure.template to_dto<AMode>(),
        .composition = composition,
        .iteration_mode = iteration_mode,
        .shell_radii = shell_radii,
        .shell_weights = shell_weights,
        .pair_weights = tensor_to_dto<AMode, Mode, T, 3>(pair_weights),
        .target_objective = tensor_to_dto<AMode, Mode, T, 3>(target_objective),
        .prefactors = tensor_to_dto<AMode, Mode, T, 3>(prefactors),
        .iterations = iterations,
        .chunk_size = chunk_size,
        .keep = keep,
        .thread_config = thread_config,
        .max_results_per_objective = max_results_per_objective,
        .seed = seed,
    };
  }

  template <class T, sublattice_mode Mode> template <core::array_mode AMode>
  io::result_t<configuration<T, Mode>> configuration<T, Mode>::from_dto(
      const configuration_dto<T, Mode, AMode>& input) {
    SQSGEN_TRY_ASSIGN(structure, structure_config<T>::template from_dto<AMode>(input.structure));
    SQSGEN_TRY_ASSIGN(pair_weights, tensor_from_dto<AMode, Mode, T, 3>(input.pair_weights));
    SQSGEN_TRY_ASSIGN(target_objective, tensor_from_dto<AMode, Mode, T, 3>(input.target_objective));
    SQSGEN_TRY_ASSIGN(prefactors, tensor_from_dto<AMode, Mode, T, 3>(input.prefactors));

    return configuration<T, Mode>{
        .sublattice_mode = input.sublattice_mode,
        .prec = input.prec,
        .atol = input.atol,
        .rtol = input.rtol,
        .bin_width = input.bin_width,
        .peak_isolation = input.peak_isolation,
        .structure = std::move(structure),
        .composition = input.composition,
        .iteration_mode = input.iteration_mode,
        .shell_radii = input.shell_radii,
        .shell_weights = input.shell_weights,
        .pair_weights = std::move(pair_weights),
        .target_objective = std::move(target_objective),
        .prefactors = std::move(prefactors),
        .iterations = input.iterations,
        .chunk_size = input.chunk_size,
        .keep = input.keep,
        .thread_config = input.thread_config,
        .max_results_per_objective = input.max_results_per_objective,
        .seed = input.seed,
    };
  }

}  // namespace sqsgen::configuration

#undef SQSGEN_TRY_ASSIGN

#endif  // SQSGEN_IMPL_CONFIGURATION_CONFIGURATION_DTO_IMPL_HPP
