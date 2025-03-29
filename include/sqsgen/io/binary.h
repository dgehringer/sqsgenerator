//
// Created by Dominik Gehringer on 15.03.25.
//

#ifndef SQSGEN_IO_BINARY_H
#define SQSGEN_IO_BINARY_H

#include <sqsgen/core/statistics.h>

#include "sqsgen/core/config.h"
#include "sqsgen/core/results.h"
#include "sqsgen/io/json.h"
#include "sqsgen/types.h"

namespace sqsgen::io::binary {

  namespace ranges = std::ranges;
  namespace views = ranges::views;

  template <class> struct binary_adapter;

  namespace detail {

    template <class T, std::size_t... Is>
    constexpr int product_impl(const std::array<T, sizeof...(Is)>& arr,
                               std::index_sequence<Is...>) {
      return (arr[Is] * ...);
    }

    template <class T, std::size_t N> constexpr T product(const std::array<T, N>& arr) {
      return product_impl(arr, std::make_index_sequence<N>{});
    }

  };  // namespace detail

  template <class Object> nlohmann::json save(Object const& object) {
    return binary_adapter<std::decay_t<Object>>::save(object);
  }

  template <class Object> Object load(const nlohmann::json& j) {
    return binary_adapter<Object>::load(j);
  };

  template <class T, int Rows, int Cols> struct binary_adapter<Eigen::Matrix<T, Rows, Cols>> {
    static nlohmann::json save(Eigen::Matrix<T, Rows, Cols> const& data) {
      return nlohmann::json{{"shape", std::array{data.rows(), data.cols()}},
                            {"data", std::vector<T>(data.data(), data.data() + data.size())}};
    }

    static Eigen::Matrix<T, Rows, Cols> load(const nlohmann::json& j) {
      const auto [rows, cols] = j.at("shape").get<std::array<int, 2>>();
      std::vector<T> data;
      data.reserve(rows * cols);
      j.at("data").get_to(data);
      return Eigen::Map<Eigen::Matrix<T, Rows, Cols>>(data.data(), rows, cols);
    }
  };

  template <class T, long Dims> struct binary_adapter<Eigen::Tensor<T, Dims>> {
    static nlohmann::json save(Eigen::Tensor<T, Dims> const& data) {
      return nlohmann::json{
          {"shape", std::array<long, Dims>{data.dimensions()}},
          {"data", std::vector<T>(data.data(), data.data() + data.size())},
      };
    }

    static Eigen::Tensor<T, Dims> load(const nlohmann::json& j) {
      const auto shape = j.at("shape").get<std::array<int, Dims>>();
      std::vector<T> data;
      data.reserve(detail::product(shape));
      j.at("data").get_to(data);
      return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return Eigen::TensorMap<Eigen::Tensor<T, Dims>>(data.data(), shape[Is]...);
      }(std::make_index_sequence<Dims>{});
    }
  };

  template <class T> struct binary_adapter<std::vector<T>> {
    static nlohmann::json save(std::vector<T> const& data) {
      auto array = nlohmann::json::array();
      ranges::for_each(data,
                       [&array](auto const& element) { array.push_back(binary::save(element)); });
      return array;
    }

    static std::vector<T> load(const nlohmann::json& j) {
      if (!j.is_array()) throw std::invalid_argument("JSON object is not an array");
      std::vector<T> result;
      result.reserve(j.size());
      for (const auto& element : j) result.push_back(binary::load<T>(element));
      return result;
    }
  };

  template <class T> struct binary_adapter<sorted_vector<T>> {
    static nlohmann::json save(sorted_vector<T> const& data) {
      auto array = nlohmann::json::array();
      ranges::for_each(data,
                       [&array](auto const& element) { array.push_back(binary::save(element)); });
      return array;
    }

    static sorted_vector<T> load(const nlohmann::json& j) {
      if (!j.is_array()) throw std::invalid_argument("JSON object is not an array");
      std::vector<T> result;
      result.reserve(j.size());
      for (const auto& element : j) result.push_back(binary::load<T>(element));
      return {result};
    }
  };

  template <class T> struct binary_adapter<core::structure<T>> {
    static nlohmann::json save(core::structure<T> const& data) {
      return nlohmann::json{{"lattice", binary::save(data.lattice)},
                            {"frac_coords", binary::save(data.frac_coords)},
                            {"species", data.species},
                            {"pbc", data.pbc}};
    }

    static core::structure<T> load(const nlohmann::json& j) {
      return core::structure<T>(binary::load<lattice_t<T>>(j.at("lattice")),
                                binary::load<coords_t<T>>(j.at("frac_coords")),
                                j.at("species").get<configuration_t>(),
                                j.at("pbc").get<std::array<bool, 3>>());
    }
  };

  template <class T> struct binary_adapter<core::structure_config<T>> {
    static nlohmann::json save(core::structure_config<T> const& data) {
      return nlohmann::json{{"lattice", binary::save(data.lattice)},
                            {"coords", binary::save(data.coords)},
                            {"species", data.species},
                            {"supercell", data.supercell}};
    }

    static core::structure_config<T> load(const nlohmann::json& j) {
      return core::structure_config<T>(
          binary::load<lattice_t<T>>(j.at("lattice")), binary::load<coords_t<T>>(j.at("coords")),
          j.at("species").get<configuration_t>(), j.at("supercell").get<std::array<int, 3>>());
    }
  };

  template <class T> struct binary_adapter<core::configuration<T>> {
    static nlohmann::json save(core::configuration<T> const& data) {
      return nlohmann::json{{"sublattice_mode", data.sublattice_mode},
                            {"iteration_mode", data.iteration_mode},
                            {"structure", binary::save(data.structure)},
                            {"composition", data.composition},
                            {"shell_radii", data.shell_radii},
                            {"shell_weights", data.shell_weights},
                            {"prefactors", binary::save(data.prefactors)},
                            {"pair_weights", binary::save(data.pair_weights)},
                            {"target_objective", binary::save(data.target_objective)},
                            {"iterations", data.iterations},
                            {"chunk_size", data.chunk_size},
                            {"thread_config", data.thread_config}};
    }

    static core::configuration<T> load(const nlohmann::json& j) {
      return core::configuration<T>(j.at("sublattice_mode").get<SublatticeMode>(),
                                    j.at("iteration_mode").get<IterationMode>(),
                                    binary::load<core::structure_config<T>>(j.at("structure")),
                                    j.at("composition").get<std::vector<sublattice>>(),
                                    j.at("shell_radii").get<stl_matrix_t<T>>(),
                                    j.at("shell_weights").get<std::vector<shell_weights_t<T>>>(),
                                    binary::load<std::vector<cube_t<T>>>(j.at("prefactors")),
                                    binary::load<std::vector<cube_t<T>>>(j.at("pair_weights")),
                                    binary::load<std::vector<cube_t<T>>>(j.at("target_objective")),
                                    j.at("iterations").get<std::optional<iterations_t>>(),
                                    j.at("chunk_size").get<iterations_t>(),
                                    j.at("thread_config").get<thread_config_t>());
    }
  };

  template <class T> struct binary_adapter<sqs_result<T, SUBLATTICE_MODE_INTERACT>> {
    static nlohmann::json save(sqs_result<T, SUBLATTICE_MODE_INTERACT> const& data) {
      return nlohmann::json{{"objective", data.objective},
                            {"species", data.species},
                            {"sro", binary::save(data.sro)}};
    }

    static sqs_result<T, SUBLATTICE_MODE_INTERACT> load(const nlohmann::json& j) {
      return sqs_result<T, SUBLATTICE_MODE_INTERACT>{j.at("objective").get<T>(),
                                                     j.at("species").get<configuration_t>(),
                                                     binary::load<cube_t<T>>(j.at("sro"))};
    }
  };

  template <class T> struct binary_adapter<sqs_statistics_data<T>> {
    static nlohmann::json save(sqs_statistics_data<T> const& data) {
      return nlohmann::json{{"finished", data.finished},
                            {"working", data.working},
                            {"best_rank", data.best_rank},
                            {"best_objective", data.best_objective},
                            {"timings", data.timings}};
    }

    static sqs_statistics_data<T> load(const nlohmann::json& j) {
      return sqs_statistics_data<T>{
          j.at("finished").get<iterations_t>(),
          j.at("working").get<iterations_t>(),
          j.at("best_rank").get<iterations_t>(),
          j.at("best_objective").get<T>(),
          j.at("timings").get<std::map<Timing, nanoseconds_t>>(),
      };
    }
  };

  template <class T> struct binary_adapter<sqs_result<T, SUBLATTICE_MODE_SPLIT>> {
    static nlohmann::json save(sqs_result<T, SUBLATTICE_MODE_SPLIT> const& data) {
      return nlohmann::json{
          {"objective", data.objective},
          {"sublattices", binary::save(data.sublattices)},
      };
    }

    static sqs_result<T, SUBLATTICE_MODE_SPLIT> load(const nlohmann::json& j) {
      return sqs_result<T, SUBLATTICE_MODE_SPLIT>{
          j.at("objective").get<T>(),
          binary::load<std::vector<sqs_result<T, SUBLATTICE_MODE_INTERACT>>>(j.at("sublattices"))};
    }
  };

  template <class T>
  struct binary_adapter<core::detail::sqs_result_wrapper<T, SUBLATTICE_MODE_INTERACT>> {
    static nlohmann::json save(
        core::detail::sqs_result_wrapper<T, SUBLATTICE_MODE_INTERACT> const& data) {
      return binary_adapter<sqs_result<T, SUBLATTICE_MODE_INTERACT>>::save(data);
    }
  };

  template <class T>
  struct binary_adapter<core::detail::sqs_result_wrapper<T, SUBLATTICE_MODE_SPLIT>> {
    static nlohmann::json save(
        core::detail::sqs_result_wrapper<T, SUBLATTICE_MODE_SPLIT> const& data) {
      return binary_adapter<sqs_result<T, SUBLATTICE_MODE_SPLIT>>::save(
          {data.objective,
           core::helpers::as<std::vector>{}(
               data.sublattices
               | views::transform(
                   [](auto&& sl) -> sqs_result<T, SUBLATTICE_MODE_INTERACT> { return sl; }))});
    }
  };

  template <class T, SublatticeMode Mode> struct binary_adapter<core::sqs_result_pack<T, Mode>> {
    static nlohmann::json save(core::sqs_result_pack<T, Mode> const& data) {
      auto flattened
          = core::helpers::as<std::vector>{}(data.results | views::elements<1> | views::join);
      return nlohmann::json{
          {"statistics", binary::save(data.statistics)},
          {"config", binary::save(data.config)},
          {"results", binary::save(flattened)},
      };
    }

    static core::sqs_result_pack<T, Mode> load(const nlohmann::json& j) {
      core::sqs_result_collection<T, Mode> results;
      for (auto&& r : binary::load<std::vector<sqs_result<T, Mode>>>(j.at("results")))
        results.insert_result(std::move(r));

      return core::sqs_result_pack<T, Mode>{
          binary::load<core::configuration<T>>(j.at("config")), results.results(),
          binary::load<sqs_statistics_data<T>>(j.at("statistics"))};
    }
  };

};  // namespace sqsgen::io::binary

#endif  // SQSGEN_IO_BINARY_H
