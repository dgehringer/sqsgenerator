//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/vector.h>

#include <array>
#include <cstddef>
#include <expected>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "sqsgen/configuration.h"
#include "sqsgen/core/atom.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/glaze.h"
#include "sqsgen/types.h"

namespace nb = nanobind;
using namespace nb::literals;

namespace sqsgen::python {
  // core::atom has a private constructor, so glaze cannot read it back; this plain
  // aggregate is its wire form. The element is fully determined by its ordinal Z, so
  // deserialization rebuilds the canonical atom through atom::from_z. It needs external
  // linkage (not an anonymous namespace), as glaze's reflection takes its address.
  struct atom_dto {
    sqsgen::specie_t z;
    std::string name;
    std::string symbol;
    std::string electronic_configuration;
    double atomic_radius;
    double mass;
    double electronegativity;
  };
}  // namespace sqsgen::python

namespace {

  namespace core = sqsgen::core;
  namespace io = sqsgen::io;
  namespace cfg_ns = sqsgen::configuration;
  namespace ranges = std::ranges;

  using sqsgen::configuration_t;
  using sqsgen::coords_t;
  using sqsgen::lattice_t;
  using sqsgen::io::wire_format;

  template <class T> using site_t = core::detail::site<T>;

  // ========================================================================= serialization

  // The five wire formats of sqsgen/io/glaze.h, mapped to glaze's format ids so the
  // write_supported / read_supported concepts can gate each branch.
  template <class DTO>
  concept all_write_formats
      = glz::write_supported<DTO, glz::JSON> && glz::write_supported<DTO, glz::YAML>
        && glz::write_supported<DTO, glz::CBOR> && glz::write_supported<DTO, glz::MSGPACK>
        && glz::write_supported<DTO, glz::BEVE>;

  template <class DTO>
  concept all_read_formats
      = glz::read_supported<DTO, glz::JSON> && glz::read_supported<DTO, glz::YAML>
        && glz::read_supported<DTO, glz::CBOR> && glz::read_supported<DTO, glz::MSGPACK>
        && glz::read_supported<DTO, glz::BEVE>;

  // Prefer io::write_any (one runtime switch over all formats). When glaze cannot
  // instantiate every format for this DTO, io::write_any would not compile, so fall
  // back to a manual switch that only emits the per-format writers that are supported.
  template <class DTO>
  std::expected<std::string, io::parser_error> write_dto(const DTO& dto, wire_format fmt) {
    if constexpr (all_write_formats<DTO>) {
      return io::write_any(dto, fmt);
    } else {
      switch (fmt) {
        case wire_format::json:
          if constexpr (glz::write_supported<DTO, glz::JSON>)
            return io::write_format<DTO, wire_format::json>(dto);
          else
            break;
        case wire_format::yaml:
          if constexpr (glz::write_supported<DTO, glz::YAML>)
            return io::write_format<DTO, wire_format::yaml>(dto);
          else
            break;
        case wire_format::cbor:
          if constexpr (glz::write_supported<DTO, glz::CBOR>)
            return io::write_format<DTO, wire_format::cbor>(dto);
          else
            break;
        case wire_format::msgpack:
          if constexpr (glz::write_supported<DTO, glz::MSGPACK>)
            return io::write_format<DTO, wire_format::msgpack>(dto);
          else
            break;
        case wire_format::beve:
          if constexpr (glz::write_supported<DTO, glz::BEVE>)
            return io::write_format<DTO, wire_format::beve>(dto);
          else
            break;
      }
      return std::unexpected(
          io::parser_error{sqsgen::error_code::parsing, "wire format not supported for this type"});
    }
  }

  template <class DTO>
  std::expected<DTO, io::parser_error> read_dto(std::string_view data, wire_format fmt) {
    if constexpr (all_read_formats<DTO>) {
      return io::read_any<DTO>(data, fmt);
    } else {
      switch (fmt) {
        case wire_format::json:
          if constexpr (glz::read_supported<DTO, glz::JSON>)
            return io::read_format<DTO, wire_format::json>(data);
          else
            break;
        case wire_format::yaml:
          if constexpr (glz::read_supported<DTO, glz::YAML>)
            return io::read_format<DTO, wire_format::yaml>(data);
          else
            break;
        case wire_format::cbor:
          if constexpr (glz::read_supported<DTO, glz::CBOR>)
            return io::read_format<DTO, wire_format::cbor>(data);
          else
            break;
        case wire_format::msgpack:
          if constexpr (glz::read_supported<DTO, glz::MSGPACK>)
            return io::read_format<DTO, wire_format::msgpack>(data);
          else
            break;
        case wire_format::beve:
          if constexpr (glz::read_supported<DTO, glz::BEVE>)
            return io::read_format<DTO, wire_format::beve>(data);
          else
            break;
      }
      return std::unexpected(
          io::parser_error{sqsgen::error_code::parsing, "wire format not supported for this type"});
    }
  }

  // from_string accepts both str (text formats) and bytes (binary formats); copy the
  // payload into an owning buffer the caller can feed to read_dto as a string_view.
  std::string payload(nb::handle data) {
    if (nb::isinstance<nb::bytes>(data)) {
      const auto b = nb::cast<nb::bytes>(data);
      return std::string(b.c_str(), b.size());
    }
    if (nb::isinstance<nb::str>(data)) return nb::cast<std::string>(data);
    throw std::invalid_argument("expected str or bytes");
  }

  // Serialize a DTO to a given wire format; binary formats (cbor/msgpack/beve) are
  // returned as raw bytes, so the Python side always receives `bytes`.
  template <class DTO> nb::bytes dump_bytes(const DTO& dto, wire_format fmt) {
    auto out = write_dto(dto, fmt);
    if (!out) throw std::runtime_error(out.error().msg);
    return nb::bytes(out->data(), out->size());
  }

  // ========================================================================= atom

  using sqsgen::python::atom_dto;

  atom_dto to_atom_dto(const core::atom& a) {
    return {a.Z, a.name, a.symbol, a.electronic_configuration, a.atomic_radius, a.mass, a.en};
  }

  core::atom atom_from_z(int z) {
    auto a = core::atom::from_z(z);
    if (!a) throw std::invalid_argument("unknown atomic number");
    return a.value();
  }

  void bind_atom(nb::module_& m) {
    nb::class_<core::atom>(m, "Atom")
        .def_ro("z", &core::atom::Z)
        .def_ro("name", &core::atom::name)
        .def_ro("symbol", &core::atom::symbol)
        .def_ro("electronic_configuration", &core::atom::electronic_configuration)
        .def_ro("atomic_radius", &core::atom::atomic_radius)
        .def_ro("mass", &core::atom::mass)
        .def_ro("electronegativity", &core::atom::en)
        .def_static("from_z", &atom_from_z, "z"_a)
        .def_static(
            "from_symbol",
            [](const std::string& symbol) {
              auto a = core::atom::from_symbol(symbol);
              if (!a) throw std::invalid_argument("unknown element symbol");
              return a.value();
            },
            "symbol"_a)
        .def(
            "dump",
            [](const core::atom& a, wire_format fmt) { return dump_bytes(to_atom_dto(a), fmt); },
            "format"_a = wire_format::json)
        .def_static(
            "from_string",
            [](nb::handle data, wire_format fmt) {
              auto dto = read_dto<atom_dto>(payload(data), fmt);
              if (!dto) throw std::runtime_error(dto.error().msg);
              return atom_from_z(dto->z);
            },
            "data"_a, "format"_a = wire_format::json)
        .def("__repr__", [](const core::atom& a) {
          return "Atom(" + a.symbol + ", Z=" + std::to_string(a.Z) + ")";
        });
  }

  // ========================================================================= structure / site

  template <class T> std::vector<site_t<T>> site_list(const core::structure<T>& s) {
    return s.sites() | ranges::to<std::vector<site_t<T>>>();
  }

  template <class T> std::vector<core::atom> structure_atoms(const core::structure<T>& s) {
    std::vector<core::atom> out;
    out.reserve(s.species.size());
    for (const auto z : s.species) out.push_back(atom_from_z(z));
    return out;
  }

  template <class T>
  void bind_structure_types(nb::module_& m, const char* site_name, const char* structure_name) {
    nb::class_<site_t<T>>(m, site_name)
        .def_ro("index", &site_t<T>::index)
        .def_ro("specie", &site_t<T>::specie)
        .def_ro("frac_coords", &site_t<T>::frac_coords)
        .def("atom", &site_t<T>::atom)
        .def(
            "__lt__", [](const site_t<T>& a, const site_t<T>& b) { return a < b; },
            nb::is_operator())
        .def(
            "__eq__", [](const site_t<T>& a, const site_t<T>& b) { return a == b; },
            nb::is_operator())
        .def("__hash__", [](const site_t<T>& s) { return typename site_t<T>::hasher{}(s); });

    nb::class_<core::structure<T>>(m, structure_name)
        .def(nb::init<const lattice_t<T>&, const coords_t<T>&, const configuration_t&,
                      const std::array<bool, 3>&>(),
             "lattice"_a, "coords"_a, "species"_a, "pbc"_a)
        .def(nb::init<const lattice_t<T>&, const coords_t<T>&, const configuration_t&>(),
             "lattice"_a, "coords"_a, "species"_a)
        .def_ro("lattice", &core::structure<T>::lattice)
        .def_ro("frac_coords", &core::structure<T>::frac_coords)
        .def_ro("species", &core::structure<T>::species)
        .def_ro("pbc", &core::structure<T>::pbc)
        .def_ro("num_species", &core::structure<T>::num_species)
        .def("distance_matrix", &core::structure<T>::distance_matrix)
        .def("shell_matrix", &core::structure<T>::shell_matrix, "shell_radii"_a,
             "atol"_a = std::numeric_limits<T>::epsilon(), "rtol"_a = static_cast<T>(1.0e-9))
        .def("supercell", &core::structure<T>::supercell, "a"_a, "b"_a, "c"_a)
        .def("without_vacancies", &core::structure<T>::without_vacancies)
        .def_prop_ro("sites", &site_list<T>)
        .def_prop_ro("atoms", &structure_atoms<T>)
        .def_prop_ro("symbols",
                     [](const core::structure<T>& s) {
                       std::vector<std::string> out;
                       out.reserve(s.species.size());
                       for (const auto z : s.species)
                         out.push_back(core::atom::from_z(z).value().symbol);
                       return out;
                     })
        .def(
            "dump",
            [](const core::structure<T>& s, wire_format fmt) {
              return dump_bytes(s.template to_dto<core::array_mode::nested>(), fmt);
            },
            "format"_a = wire_format::json)
        .def_static(
            "from_string",
            [](nb::handle data, wire_format fmt) {
              auto dto
                  = read_dto<core::structure_dto<T, core::array_mode::nested>>(payload(data), fmt);
              if (!dto) throw std::runtime_error(dto.error().msg);
              auto s = core::structure<T>::template from_dto<core::array_mode::nested>(dto.value());
              if (!s) throw std::runtime_error(s.error().msg);
              return s.value();
            },
            "data"_a, "format"_a = wire_format::json)
        .def("__len__", [](const core::structure<T>& s) { return s.size(); })
        .def("__mul__",
             [](const core::structure<T>& s, const std::array<std::size_t, 3>& scale) {
               return s.supercell(scale[0], scale[1], scale[2]);
             })
        .def(
            "__eq__",
            [](const core::structure<T>& a, const core::structure<T>& b) {
              return site_list(a) == site_list(b);
            },
            nb::is_operator())
        .def("__iter__", [](nb::handle self) {
          // Delegate to the materialized `sites` list so the iterator owns its data.
          return self.attr("sites").attr("__iter__")();
        });
  }

  // ========================================================================= configuration

  template <class T, sqsgen::sublattice_mode Mode>
  void bind_configuration_types(nb::module_& m, const char* input_name, const char* config_name) {
    using cfg_in = cfg_ns::configuration_input<T, Mode>;
    using cfg = cfg_ns::configuration<T, Mode>;

    nb::class_<cfg_in>(m, input_name)
        .def_ro("sublattice_mode", &cfg_in::sublattice_mode)
        .def_ro("prec", &cfg_in::prec)
        .def(
            "dump", [](const cfg_in& c, wire_format fmt) { return dump_bytes(c, fmt); },
            "format"_a = wire_format::json)
        .def_static(
            "from_string",
            [](nb::handle data, wire_format fmt) {
              auto r = read_dto<cfg_in>(payload(data), fmt);
              if (!r) throw std::runtime_error(r.error().msg);
              return std::move(r).value();
            },
            "data"_a, "format"_a = wire_format::json);

    nb::class_<cfg>(m, config_name)
        .def_ro("sublattice_mode", &cfg::sublattice_mode)
        .def_ro("prec", &cfg::prec)
        .def_ro("pairs", &cfg::pairs)
        .def_ro("triplets", &cfg::triplets)
        .def("to_input", &cfg::to_input)
        // configuration owns Eigen data and is not glaze-serializable; its input form is.
        .def(
            "dump", [](const cfg& c, wire_format fmt) { return dump_bytes(c.to_input(), fmt); },
            "format"_a = wire_format::json)
        .def_static(
            "from_string",
            [](nb::handle data, wire_format fmt) {
              auto input = read_dto<cfg_in>(payload(data), fmt);
              if (!input) throw std::runtime_error(input.error().msg);
              auto parsed = cfg_ns::parse_configuration(
                  cfg_ns::configuration_input_variant_t{std::move(input).value()});
              if (!parsed) throw std::runtime_error(parsed.error().msg);
              return std::get<cfg>(std::move(parsed).value());
            },
            "data"_a, "format"_a = wire_format::json);
  }

}  // namespace

NB_MODULE(_core, m) {
  m.doc() = "sqsgenerator core extension (nanobind, stable ABI >= 3.12)";

  nb::enum_<sqsgen::sublattice_mode>(m, "SublatticeMode")
      .value("interact", sqsgen::sublattice_mode::interact)
      .value("split", sqsgen::sublattice_mode::split);

  nb::enum_<sqsgen::prec>(m, "Prec")
      .value("single", sqsgen::prec::Single)
      .value("double", sqsgen::prec::Double);

  nb::enum_<sqsgen::io::wire_format>(m, "WireFormat")
      .value("json", sqsgen::io::wire_format::json)
      .value("yaml", sqsgen::io::wire_format::yaml)
      .value("cbor", sqsgen::io::wire_format::cbor)
      .value("msgpack", sqsgen::io::wire_format::msgpack)
      .value("beve", sqsgen::io::wire_format::beve);

  bind_atom(m);

  bind_structure_types<float>(m, "SiteFloat", "StructureFloat");
  bind_structure_types<double>(m, "SiteDouble", "StructureDouble");

  bind_configuration_types<float, sqsgen::sublattice_mode::interact>(
      m, "ConfigurationInputFloatInteract", "ConfigurationFloatInteract");
  bind_configuration_types<float, sqsgen::sublattice_mode::split>(m, "ConfigurationInputFloatSplit",
                                                                  "ConfigurationFloatSplit");
  bind_configuration_types<double, sqsgen::sublattice_mode::interact>(
      m, "ConfigurationInputDoubleInteract", "ConfigurationDoubleInteract");
  bind_configuration_types<double, sqsgen::sublattice_mode::split>(
      m, "ConfigurationInputDoubleSplit", "ConfigurationDoubleSplit");
}
