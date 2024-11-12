//
// Created by Dominik Gehringer on 23.10.24.
//

#ifndef SQSGEN_IO_JSON_H
#define SQSGEN_IO_JSON_H

#include <nlohmann/json.hpp>

#include "sqsgen/core/structure.h"
#include "sqsgen/types.h"
#include "sqsgen/config.h"

using namespace sqsgen;

// partial specialization (full specialization works too)
NLOHMANN_JSON_NAMESPACE_BEGIN
template <class T> struct adl_serializer<matrix_t<T>> {
  static void to_json(json& j, const matrix_t<T>& m) { j = core::helpers::eigen_to_stl(m); }

  static void from_json(const json& j, matrix_t<T>& m) {
    m = std::move(core::helpers::stl_to_eigen<matrix_t<T>>(j.get<stl_matrix_t<T>>()));
  }
};

template <class T> struct adl_serializer<lattice_t<T>> {
  static void to_json(json& j, const lattice_t<T>& m) { j = core::helpers::eigen_to_stl(m); }

  static void from_json(const json& j, lattice_t<T>& m) {
    m = std::move(core::helpers::stl_to_eigen<lattice_t<T>>(j.get<stl_matrix_t<T>>()));
  }
};

template <class T> struct adl_serializer<coords_t<T>> {
  static void to_json(json& j, const coords_t<T>& m) { j = core::helpers::eigen_to_stl(m); }

  static void from_json(const json& j, coords_t<T>& m) {
    m = std::move(core::helpers::stl_to_eigen<coords_t<T>>(j.get<stl_matrix_t<T>>()));
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
  }

};
NLOHMANN_JSON_NAMESPACE_END

namespace sqsgen {

  using json = nlohmann::json;

  NLOHMANN_JSON_SERIALIZE_ENUM(Prec, {
    {PREC_SINGLE, "single"},
    {PREC_DOUBLE, "double"}
  })


  template<core::helpers::string_literal key, class T>
  std::optional<T> get_optional(json const& json) {
    if (json.contains(key.data)) {
      return std::make_optional(json.at(key.data).template get<T>());
    }
    return std::nullopt;
  }

}

#endif  // SQSGEN_IO_JSON_H
