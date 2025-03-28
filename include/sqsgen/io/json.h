//
// Created by Dominik Gehringer on 23.10.24.
//

#ifndef SQSGEN_IO_JSON_H
#define SQSGEN_IO_JSON_H

#include <nlohmann/json.hpp>
#include <utility>

#include "sqsgen/core/config.h"
#include "sqsgen/core/helpers.h"
#include "sqsgen/core/results.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/parsing.h"
#include "sqsgen/types.h"

// partial specialization (full specialization works too)
NLOHMANN_JSON_NAMESPACE_BEGIN

using namespace sqsgen;
template <class T> struct adl_serializer<matrix_t<T>> {
  static void to_json(json& j, const matrix_t<T>& m) { j = core::helpers::eigen_to_stl(m); }

  static void from_json(const json& j, matrix_t<T>& m) {
    m = std::move(core::helpers::stl_to_eigen<matrix_t<T>>(j.get<stl_matrix_t<T>>()));
  }
};

template <class T> struct adl_serializer<lattice_t<T>> {
  static void to_json(json& j, const lattice_t<T>& m) {
    j = core::helpers::eigen_to_stl<lattice_t<T>>(m);
  }

  static void from_json(const json& j, lattice_t<T>& m) {
    m = std::move(core::helpers::stl_to_eigen<lattice_t<T>>(j.get<stl_matrix_t<T>>()));
  }
};

template <class T> struct adl_serializer<coords_t<T>> {
  static void to_json(json& j, const coords_t<T>& m) {
    j = core::helpers::eigen_to_stl<coords_t<T>>(m);
  }

  static void from_json(const json& j, coords_t<T>& m) {
    m = std::move(core::helpers::stl_to_eigen<coords_t<T>>(j.get<stl_matrix_t<T>>()));
  }
};

template <class T> struct adl_serializer<cube_t<T>> {
  static void to_json(json& j, const cube_t<T>& m) { j = core::helpers::eigen_to_stl(m); }

  static void from_json(const json& j, cube_t<T>& m) {
    m = std::move(core::helpers::stl_to_eigen<cube_t<T>>(j.get<stl_cube_t<T>>()));
  }
};

template <class T> struct adl_serializer<core::helpers::sorted_vector<T>> {
  static void to_json(json& j, const core::helpers::sorted_vector<T>& m) {
    adl_serializer<std::vector<T>>::to_json(j, m._values);
  }

  static void from_json(const json& j, core::helpers::sorted_vector<T>& m) {
    std::vector<T> helper;
    adl_serializer<std::vector<T>>::from_json(j, helper);
    m = core::helpers::sorted_vector<T>(helper);
  }
};

template <class T> struct adl_serializer<sqs_statistics_data<T>> {
  static void to_json(json& j, const sqs_statistics_data<T>& m) {
    j = json{{"best_objective", m.best_objective},
             {"best_rank", m.best_rank},
             {"finished", m.finished},
             {"working", m.working},
             {"timings", m.timings}};
  }

  static void from_json(const json& j, sqs_statistics_data<T>& m) {
    j.at("best_objective").get_to(m.best_objective);
    j.at("best_rank").get_to(m.best_rank);
    j.at("finished").get_to(m.finished);
    j.at("working").get_to(m.working);
    j.at("timings").get_to(m.timings);
  }
};

template <class T> struct adl_serializer<core::structure<T>> {
  static void to_json(json& j, core::structure<T> const& s) {
    j = json{
        {"lattice", s.lattice},
        {"species", s.species},
        {"frac_coords", s.frac_coords},
        {"pbc", s.pbc},

    };
  }

  static void from_json(const json& j, core::structure<T>& s) {
    j.at("species").get_to(s.species);
    j.at("lattice").get_to(s.lattice);
    j.at("frac_coords").get_to(s.frac_coords);
    j.at("pbc").get_to(s.pbc);
    s.num_species = core::detail::compute_num_species(s.species);
  }
};

template <class T> struct adl_serializer<std::optional<T>> {
  static void to_json(json& j, const std::optional<T>& opt) {
    if (opt.has_value()) {
      j = *opt;
    } else {
      j = nullptr;
    }
  }

  static void from_json(const json& j, std::optional<T>& opt) {
    if (j.is_null()) {
      opt = std::nullopt;
    } else {
      opt = j.get<T>();
    }
  }
};

NLOHMANN_JSON_NAMESPACE_END

namespace sqsgen {
  NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(sublattice, sites, composition);

  NLOHMANN_JSON_SERIALIZE_ENUM(Prec, {{PREC_INVALID, nullptr},
                                      {PREC_SINGLE, "single"},
                                      {PREC_DOUBLE, "double"}})
  NLOHMANN_JSON_SERIALIZE_ENUM(ShellRadiiDetection, {{SHELL_RADII_DETECTION_INVALID, nullptr},
                                                     {SHELL_RADII_DETECTION_NAIVE, "naive"},
                                                     {SHELL_RADII_DETECTION_PEAK, "peak"}})
  NLOHMANN_JSON_SERIALIZE_ENUM(IterationMode, {
                                                  {ITERATION_MODE_INVALID, nullptr},
                                                  {ITERATION_MODE_RANDOM, "random"},
                                                  {ITERATION_MODE_SYSTEMATIC, "systematic"},
                                              })

  NLOHMANN_JSON_SERIALIZE_ENUM(SublatticeMode, {
                                                   {SUBLATTICE_MODE_INVALID, nullptr},
                                                   {SUBLATTICE_MODE_INTERACT, "interact"},
                                                   {SUBLATTICE_MODE_SPLIT, "split"},
                                               })

  NLOHMANN_JSON_SERIALIZE_ENUM(Timing, {
                                           {TIMING_UNDEFINED, nullptr},
                                           {TIMING_COMM, "comm"},
                                           {TIMING_LOOP, "loop"},
                                           {TIMING_TOTAL, "total"},
                                           {TIMING_CHUNK_SETUP, "chunk_setup"},
                                       })
  namespace io {
    template <> struct accessor<nlohmann::json> {
      static bool contains(nlohmann::json const& json, std::string&& key) {
        return json.contains(std::forward<std::string>(key));
      }

      static auto get(nlohmann::json const& json, std::string&& key) { return json.at(key); }

      static bool is_document(nlohmann::json const& json) { return json.is_object(); }

      static bool is_list(nlohmann::json const& json) { return json.is_array(); }

      static auto items(nlohmann::json const& json) { return json.items(); }

      static auto range(nlohmann::json const& json) {
        std::vector<nlohmann::json> items;
        items.reserve(json.size());
        for (auto& item : json) {
          items.push_back(item);
        }
        return items;
      }

      template <string_literal key = "", class Option>
      static parse_result<Option> get_as(nlohmann::json const& json) {
        try {
          if constexpr (key == KEY_NONE) {
            return json.get<Option>();
          } else {
            if (json.contains(key.data)) return {json.at(key.data).template get<Option>()};
            return parse_error::from_msg<key, CODE_NOT_FOUND>(
                std::format("could not find key {}", key.data));
          }
        } catch (nlohmann::json::out_of_range const& e) {
          return parse_error::from_msg<key, CODE_TYPE_ERROR>("out of range - found - {}", e.what());
        } catch (nlohmann::json::type_error const& e) {
          return parse_error::from_msg<key, CODE_TYPE_ERROR>(
              std::format("type error - cannot parse {} - {}", typeid(Option).name(), e.what()));
        } catch (std::out_of_range const& e) {
          return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(e.what());
        }
      }
    };
  }  // namespace io

}  // namespace sqsgen

#endif  // SQSGEN_IO_JSON_H
