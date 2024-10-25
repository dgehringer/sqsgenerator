//
// Created by Dominik Gehringer on 23.10.24.
//

#ifndef SQSGEN_IO_JSON_H
#define SQSGEN_IO_JSON_H

#include <nlohmann/json.hpp>

#include "core/structure.h"
#include "types.h"

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
NLOHMANN_JSON_NAMESPACE_END

namespace sqsgen {
  using json = nlohmann::json;

  /*void to_json(json& j, const person& p) {
    j = json{{"name", p.name}, {"address", p.address}, {"age", p.age}};
  }

  void from_json(const json& j, person& p) {
    j.at("name").get_to(p.name);
    j.at("address").get_to(p.address);
    j.at("age").get_to(p.age);
  }*/

  template <class T> void to_json(json& j, core::Structure<T> const& st) {
    j = json{{"lattice", core::helpers::eigen_to_stl(st.lattice)},
             {"species", st.species},
             {"frac_coords", core::helpers::eigen_to_stl(st.frac_coords)},
             {"pbc", st.pbc}};
  };

  template <class T> void from_json(const json& j, core::Structure<T>& st) {
    auto lattice = j.at("lattice").template get<std::vector<std::vector<T>>>();
    auto frac_coords = j.at("frac_coords").template get<std::vector<std::vector<T>>>();
    auto species = j.at("species").template get<std::vector<specie_t>>();
    auto pbc = j.at("pbc").template get<std::array<bool, 3>>();
    st = core::Structure<T>(std::move(core::helpers::stl_to_eigen<lattice_t<T>>(lattice)),
                            std::move(core::helpers::stl_to_eigen<coords_t<T>>(frac_coords)),
                            std::move(species),
                            std::move(pbc));
  }

}  // namespace sqsgen

#endif  // SQSGEN_IO_JSON_H
