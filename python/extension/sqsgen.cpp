//
// Created by Dominik Gehringer on 08.03.25.
//

#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <nlohmann/detail/exceptions.hpp>

#include "sqsgen/core/helpers.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/parsing.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/types.h"
#include "utils.h"

namespace py = pybind11;

using namespace sqsgen::core::helpers;

template <string_literal Name, class T>
  requires std::is_arithmetic_v<T>
constexpr auto full_name() {
  if constexpr (std::is_same_v<double, T>)
    return (Name + string_literal("Double"));
  else if constexpr (std::is_same_v<float, T>)
    return (Name + string_literal("Float"));
};

template <class... Args>
sqsgen::io::detail::unpacked_t<Args...> unwrap(sqsgen::io::parse_result<Args...> &&result) {
  if (result.ok()) {
    return result.result();
  } else {
    sqsgen::io::parse_error error = result.error();
    switch (error.code) {
      case sqsgen::io::CODE_UNKNOWN:
        throw std::runtime_error(error.msg);
      case sqsgen::io::CODE_OUT_OF_RANGE:
        throw py::index_error(error.msg);
      case sqsgen::io::CODE_TYPE_ERROR:
        throw py::type_error(error.msg);
      case sqsgen::io::CODE_NOT_FOUND:
        throw py::key_error(error.msg);
      case sqsgen::io::CODE_BAD_VALUE:
        throw py::value_error(error.msg);
      case sqsgen::io::CODE_BAD_ARGUMENT:
        throw py::value_error(error.msg);
      default:
        throw std::runtime_error(error.msg);
    }
  }
}

template <string_literal Name, class T> void bind_sqs_statistics_data(py::module &m) {
  py::class_<sqsgen::sqs_statistics_data<T>>(m, full_name<Name, T>().c_str())
      .def_readonly("finished", &sqsgen::sqs_statistics_data<T>::finished)
      .def_readonly("working", &sqsgen::sqs_statistics_data<T>::working)
      .def_readonly("best_rank", &sqsgen::sqs_statistics_data<T>::best_rank)
      .def_readonly("best_objective", &sqsgen::sqs_statistics_data<T>::best_objective)
      .def_readonly("timings", &sqsgen::sqs_statistics_data<T>::timings);
}

template <string_literal Name, class T> void bind_structure(py::module &m) {
  using namespace sqsgen;
  using namespace sqsgen::core;
  py::class_<structure<T>>(m, full_name<Name, T>().c_str())
      .def(py::init<lattice_t<T>, coords_t<T>, configuration_t, std::array<bool, 3>>(),
           py::arg("lattice"), py::arg("coords"), py::arg("species"), py::arg("pbc"))
      .def(py::init<lattice_t<T>, coords_t<T>, configuration_t>(), py::arg("lattice"),
           py::arg("coords"), py::arg("species"))
      .def_readonly("lattice", &structure<T>::lattice)
      .def_readonly("species", &structure<T>::species)
      .def_readonly("frac_coords", &structure<T>::frac_coords)
      .def_readonly("pbc", &structure<T>::pbc)
      .def_readonly("num_species", &structure<T>::num_species)
      .def("sites", [](structure<T> &s) { return as<std::vector>{}(s.sites()); })
      .def("distance_matrix", &structure<T>::distance_matrix)
      .def("shell_matrix", &structure<T>::shell_matrix, py::arg("shell_radii"),
           py::arg("atol") = std::numeric_limits<T>::epsilon(), py::arg("rtol") = 1.0e-9)
      .def("supercell", &structure<T>::supercell, py::arg("sa"), py::arg("sb"), py::arg("sc"))
      .def("__len__", [](structure<T> &s) { return s.size(); })
      .def("dump",
           [](structure<T> &s, StructureFormat format) {
             switch (format) {
               case STRUCTURE_FORMAT_JSON_SQSGEN:
                 return io::structure_adapter<T, STRUCTURE_FORMAT_JSON_SQSGEN>::format(s);
               case STRUCTURE_FORMAT_JSON_PYMATGEN:
                 return io::structure_adapter<T, STRUCTURE_FORMAT_JSON_PYMATGEN>::format(s);
               case STRUCTURE_FORMAT_JSON_ASE:
                 return io::structure_adapter<T, STRUCTURE_FORMAT_JSON_ASE>::format(s);
               case STRUCTURE_FORMAT_CIF:
                 return io::structure_adapter<T, STRUCTURE_FORMAT_CIF>::format(s);
               case STRUCTURE_FORMAT_POSCAR:
                 return io::structure_adapter<T, STRUCTURE_FORMAT_POSCAR>::format(s);
               default:
                 throw py::value_error("Unknown Structure format");
             };
           })
      .def_static(
          "from_poscar",
          [](std::string const &string) {
            return unwrap(io::structure_adapter<T, STRUCTURE_FORMAT_POSCAR>::from_string(string));
          },
          py::arg("string"))
      .def_static("from_json", [](std::string const &string, StructureFormat format) {
        switch (format) {
          case STRUCTURE_FORMAT_JSON_SQSGEN:
            return unwrap(
                io::structure_adapter<T, STRUCTURE_FORMAT_JSON_SQSGEN>::from_json(string));
          case STRUCTURE_FORMAT_JSON_PYMATGEN:
            return unwrap(
                io::structure_adapter<T, STRUCTURE_FORMAT_JSON_PYMATGEN>::from_json(string));
          default:
            throw py::value_error("Unknown Structure format");
        }
      });
}

template <string_literal Name, class T> void bind_configuration(py::module &m) {
  py::class_<sqsgen::core::configuration<T>>(m, full_name<Name, T>().c_str())
      .def_readwrite("sublattice_mode", &sqsgen::core::configuration<T>::sublattice_mode)
      .def_readwrite("iteration_mode", &sqsgen::core::configuration<T>::iteration_mode)
      .def("structure",
           [](sqsgen::core::configuration<T> &conf) { return conf.structure.structure(); })
      .def_readwrite("shell_radii", &sqsgen::core::configuration<T>::shell_radii)
      .def_readwrite("shell_weights", &sqsgen::core::configuration<T>::shell_weights)
      .def_readwrite("prefactors", &sqsgen::core::configuration<T>::prefactors)
      .def_readwrite("pair_weights", &sqsgen::core::configuration<T>::pair_weights)
      .def_readwrite("target_objective", &sqsgen::core::configuration<T>::target_objective)
      .def_readwrite("iterations", &sqsgen::core::configuration<T>::iterations)
      .def_readwrite("chunk_size", &sqsgen::core::configuration<T>::chunk_size)
      .def_readwrite("thread_config", &sqsgen::core::configuration<T>::thread_config);
}

PYBIND11_MODULE(_sqsgen, m) {
  using namespace sqsgen;
  m.doc() = "pybind11 example plugin";  // Optional module docstring

  py::enum_<Timing>(m, "Timing")
      .value("undefined", TIMING_UNDEFINED)
      .value("total", TIMING_TOTAL)
      .value("chunk_setup", TIMING_CHUNK_SETUP)
      .value("loop", TIMING_LOOP)
      .value("comm", TIMING_COMM)
      .export_values();

  py::enum_<Prec>(m, "Prec")
      .value("single", PREC_SINGLE)
      .value("double", PREC_DOUBLE)
      .export_values();

  py::enum_<IterationMode>(m, "IterationMode")
      .value("random", ITERATION_MODE_RANDOM)
      .value("systematic", ITERATION_MODE_SYSTEMATIC)
      .export_values();

  py::enum_<ShellRadiiDetection>(m, "ShellRadiiDetection")
      .value("naive", SHELL_RADII_DETECTION_NAIVE)
      .value("peak", SHELL_RADII_DETECTION_PEAK)
      .export_values();

  py::enum_<SublatticeMode>(m, "SublatticeMode")
      .value("interact", SUBLATTICE_MODE_INTERACT)
      .value("split", SUBLATTICE_MODE_SPLIT)
      .export_values();

  py::enum_<StructureFormat>(m, "StructureFormat")
      .value("json_sqsgen", STRUCTURE_FORMAT_JSON_SQSGEN)
      .value("json_ase", STRUCTURE_FORMAT_JSON_ASE)
      .value("json_pymatgen", STRUCTURE_FORMAT_JSON_PYMATGEN)
      .value("cif", STRUCTURE_FORMAT_CIF)
      .value("poscar", STRUCTURE_FORMAT_POSCAR)
      .export_values();

  py::enum_<io::parse_error_code>(m, "ParseErrorCode")
      .value("unknown", io::CODE_UNKNOWN)
      .value("bad_value", io::CODE_BAD_VALUE)
      .value("bad_argument", io::CODE_BAD_ARGUMENT)
      .value("out_of_range", io::CODE_OUT_OF_RANGE)
      .value("type_error", io::CODE_TYPE_ERROR)
      .value("not_found", io::CODE_NOT_FOUND)
      .export_values();

  py::class_<io::parse_error>(m, "ParseError")
      .def_readonly("key", &io::parse_error::key)
      .def_readonly("msg", &io::parse_error::msg)
      .def_readonly("code", &io::parse_error::code)
      .def_readonly("parameter", &io::parse_error::parameter);

  py::class_<core::atom>(m, "Atom")
      .def_readonly("name", &core::atom::name)
      .def_readonly("mass", &core::atom::mass)
      .def_readonly("atomic_radius", &core::atom::atomic_radius)
      .def_readonly("electronic_configuration", &core::atom::electronic_configuration)
      .def_readonly("symbol", &core::atom::symbol)
      .def_readonly("z", &core::atom::Z)
      .def_readonly("electronegativity", &core::atom::en)
      .def_static("from_z", &core::atom::from_z<int>, py::arg("ordinal"))
      .def_static("from_symbol", &core::atom::from_symbol, py::arg("symbol"));

  py::class_<vset<usize_t>>(m, "Indices")
      .def(py::init<>())
      .def(
          "__or__",
          [](vset<usize_t> &v, std::vector<usize_t> other) {
            v.merge(other);
            return v;
          },
          py::arg("other"), py::is_operator())
      .def(
          "__or__",
          [](vset<usize_t> &v, vset<usize_t> const &other) {
            v.merge(other);
            return v;
          },
          py::arg("other"), py::is_operator())
      .def(
          "__or__",
          [](vset<usize_t> &v, py::iterable const &other) {
            for (auto item : other) v.insert(item.cast<usize_t>());
            return v;
          },
          py::arg("other"), py::is_operator())
      .def("empty", &vset<usize_t>::empty)
      .def("size", &vset<usize_t>::size)
      .def(
          "add", [](vset<usize_t> &v, usize_t value) { v.insert(value); }, py::arg("value"))
      .def("contains", &vset<usize_t>::contains)
      .def("__len__", [](const vset<usize_t> &v) { return v.size(); })
      .def(
          "__iter__", [](vset<usize_t> &v) { return py::make_iterator(v.begin(), v.end()); },
          py::keep_alive<0, 1>());

  py::class_<sublattice>(m, "Sublattice")
      .def(py::init<vset<usize_t>, composition_t>())
      .def_readwrite("sites", &sublattice::sites)
      .def_readwrite("composition", &sublattice::composition);

  bind_sqs_statistics_data<"SqsStatisticsData", float>(m);
  bind_sqs_statistics_data<"SqsStatisticsData", double>(m);

  bind_structure<"Structure", float>(m);
  bind_structure<"Structure", double>(m);

  bind_configuration<"Configuration", float>(m);
  bind_configuration<"Configuration", double>(m);
}