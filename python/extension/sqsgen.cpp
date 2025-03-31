//
// Created by Dominik Gehringer on 08.03.25.
//
#include <pybind11/eigen.h>
#include <pybind11/eigen/tensor.h>
#include <pybind11/embed.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "sqsgen/core/results.h"
#include "sqsgen/io/config/combined.h"
#include "sqsgen/io/dict.h"
#include "sqsgen/io/json.h"
#include "sqsgen/io/parsing.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/sqs.h"
#include "sqsgen/types.h"
#include "utils.h"

#define stringify_helper(x) #x
#define stringify(x) stringify_helper(x)

namespace py = pybind11;

using namespace sqsgen::core::helpers;

template <string_literal Name, class T>
  requires std::is_arithmetic_v<T>
constexpr auto format_prec() {
  if constexpr (std::is_same_v<double, T>)
    return (Name + string_literal("Double"));
  else if constexpr (std::is_same_v<float, T>)
    return (Name + string_literal("Float"));
};

template <class T> py::bytes to_bytes(T &value) {
  using namespace sqsgen;
  auto data = nlohmann::json::to_msgpack(nlohmann::json{{value}});
  std::string_view view(reinterpret_cast<char *>(data.data()), data.size());
  return py::bytes(view);
}

template <class T> T from_bytes(std::string_view view) {
  using namespace sqsgen;
  auto data = nlohmann::json::from_msgpack(view);
  std::cerr << data << std::endl;
  T result = data.get<T>();
  return result;
}

template <string_literal Name, sqsgen::SublatticeMode Mode> constexpr auto format_sublattice() {
  if constexpr (sqsgen::SUBLATTICE_MODE_INTERACT == Mode)
    return (Name + string_literal("Interact"));
  else if constexpr (sqsgen::SUBLATTICE_MODE_SPLIT == Mode)
    return (Name + string_literal("Split"));
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
  py::class_<sqsgen::sqs_statistics_data<T>>(m, format_prec<Name, T>().c_str())
      .def_readonly("finished", &sqsgen::sqs_statistics_data<T>::finished)
      .def_readonly("working", &sqsgen::sqs_statistics_data<T>::working)
      .def_readonly("best_rank", &sqsgen::sqs_statistics_data<T>::best_rank)
      .def_readonly("best_objective", &sqsgen::sqs_statistics_data<T>::best_objective)
      .def_readonly("timings", &sqsgen::sqs_statistics_data<T>::timings);
}

template <string_literal Name, class T> void bind_site(py::module &m) {
  using bind_t = sqsgen::core::detail::site<T>;
  py::class_<bind_t>(m, format_prec<Name, T>().c_str())
      .def_readonly("index", &bind_t::index)
      .def_readonly("frac_coords", &bind_t::frac_coords)
      .def_readonly("specie", &bind_t::specie)
      .def("atom", &bind_t::atom)
      .def("__hash__", [](bind_t &self) { return typename bind_t::hasher{}(self); })
      .def(
          "__eq__", [](bind_t &a, bind_t &b) { return a == b; }, py::is_operator())
      .def("__lt__", [](bind_t &a, bind_t &b) { return a < b; }, py::is_operator());
}

template <string_literal Name, class T> void bind_structure(py::module &m) {
  using namespace sqsgen;
  using namespace sqsgen::core;
  py::class_<structure<T>>(m, format_prec<Name, T>().c_str())
      .def(py::init<lattice_t<T>, coords_t<T>, configuration_t, std::array<bool, 3>>(),
           py::arg("lattice"), py::arg("coords"), py::arg("species"), py::arg("pbc"))
      .def(py::init<lattice_t<T>, coords_t<T>, configuration_t>(), py::arg("lattice"),
           py::arg("coords"), py::arg("species"))
      .def_readonly("lattice", &structure<T>::lattice)
      .def_readonly("species", &structure<T>::species)
      .def_readonly("frac_coords", &structure<T>::frac_coords)
      .def_readonly("num_species", &structure<T>::num_species)
      .def_property_readonly(
          "symbols",
          [](structure<T> &self) {
            return as<std::vector>{}(
                self.species | views::transform([](auto &&z) { return atom::from_z(z).symbol; }));
          })
      .def_property_readonly(
          "atoms",
          [](structure<T> &self) {
            return as<std::vector>{}(self.species
                                     | views::transform([](auto &&z) { return atom::from_z(z); }));
          })
      .def_property_readonly("uuid",
                             [](structure<T> &self) {
                               py::scoped_interpreter guard{};
                               auto uuid = py::module_::import("uuid");
                               return uuid.attr("to_string")(self.uuid());
                             })
      .def_property_readonly("sites", [](structure<T> &s) { return as<std::set>{}(s.sites()); })
      .def_property_readonly("distance_matrix", &structure<T>::distance_matrix)
      .def("shell_matrix", &structure<T>::shell_matrix, py::arg("shell_radii"),
           py::arg("atol") = std::numeric_limits<T>::epsilon(), py::arg("rtol") = 1.0e-9)
      .def("supercell", &structure<T>::supercell, py::arg("sa"), py::arg("sb"), py::arg("sc"))
      .def("__len__", [](structure<T> &s) { return s.size(); })
      .def("__mul__",
           [](structure<T> &a, py::tuple const &scale) {
             return a.supercell(scale[0].cast<std::size_t>(), scale[1].cast<std::size_t>(),
                                scale[2].cast<std::size_t>());
           })
      .def("__eq__",
           [](structure<T> &a, structure<T> &b) {
             return as<std::set>{}(a.sites()) == as<std::set>{}(b.sites());
           })
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
      .def("bytes", &to_bytes<structure<T>>)
      .def_static("from_bytes", &from_bytes<structure<T>>, py::arg("bytes"))
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
  using namespace sqsgen::core;
  py::class_<configuration<T>>(m, format_prec<Name, T>().c_str())
      .def_readwrite("sublattice_mode", &configuration<T>::sublattice_mode)
      .def_readwrite("iteration_mode", &configuration<T>::iteration_mode)
      .def("structure", [](configuration<T> &conf) { return conf.structure.structure(); })
      .def_readwrite("shell_radii", &configuration<T>::shell_radii)
      .def_readwrite("shell_weights", &configuration<T>::shell_weights)
      .def_readwrite("prefactors", &configuration<T>::prefactors)
      .def_readwrite("pair_weights", &configuration<T>::pair_weights)
      .def_readwrite("target_objective", &configuration<T>::target_objective)
      .def_readwrite("iterations", &configuration<T>::iterations)
      .def_readwrite("chunk_size", &configuration<T>::chunk_size)
      .def_readwrite("thread_config", &configuration<T>::thread_config)
      .def_readwrite("composition", &configuration<T>::composition)
      .def("bytes", &to_bytes<configuration<T>>)
      .def_static("from_bytes", &from_bytes<configuration<T>>);
}

template <string_literal Name, class T> void bind_sro_parameter(py::module &m) {
  using namespace sqsgen::python::helpers;
  py::class_<sqsgen::sro_parameter<T>>(m, format_prec<Name, T>().c_str())
      .def_readonly("shell", &sqsgen::sro_parameter<T>::shell)
      .def_readonly("i", &sqsgen::sro_parameter<T>::i)
      .def_readonly("j", &sqsgen::sro_parameter<T>::j)
      .def_readonly("value", &sqsgen::sro_parameter<T>::value)
      .def("__float__", [](sqsgen::sro_parameter<T> &p) { return p.value; })
      .def("__repr__", [](sqsgen::sro_parameter<T> &p) -> std::wstring {
        return std::format(L"α{}{}₋{}({})", format_ordinal<false>(p.shell), format_ordinal(p.i),
                           format_ordinal(p.j), p.value);
      });
}

template <string_literal Name, class T> void bind_callback_context(py::module &m) {
  py::class_<sqsgen::sqs_callback_context<T>>(m, format_prec<Name, T>().c_str())
      .def("stop", &sqsgen::sqs_callback_context<T>::stop)
      .def_readonly("statistics", &sqsgen::sqs_callback_context<T>::statistics);
}

template <string_literal Name, class T, sqsgen::SublatticeMode Mode>
void bind_result(py::module &m) {
  using namespace sqsgen;
  using namespace sqsgen::core;
  using namespace sqsgen::core::detail;

  if constexpr (Mode == SUBLATTICE_MODE_INTERACT) {
    py::class_<sqs_result_wrapper<T, Mode>>(
        m, format_prec<format_sublattice<Name, Mode>(), T>().c_str())
        .def_readonly("objective", &sqs_result_wrapper<T, Mode>::objective)
        .def("structure", &sqs_result_wrapper<T, Mode>::structure)
        .def(
            "sro",
            [](sqs_result_wrapper<T, Mode> &r, usize_t shell, std::string const &i,
               std::string const &j) { return r.parameter(shell, i, j); },
            py::arg("shell"), py::arg("i"), py::arg("j"))
        .def(
            "sro",
            [](sqs_result_wrapper<T, Mode> &r, usize_t shell, specie_t i, specie_t j) {
              return r.parameter(shell, i, j);
            },
            py::arg("shell"), py::arg("i"), py::arg("j"))
        .def(
            "sro", [](sqs_result_wrapper<T, Mode> &r, usize_t shell) { return r.parameter(shell); },
            py::arg("shell"))
        .def(
            "sro",
            [](sqs_result_wrapper<T, Mode> &r, specie_t i, specie_t j) {
              return r.parameter(i, j);
            },
            py::arg("i"), py::arg("j"))
        .def(
            "sro",
            [](sqs_result_wrapper<T, Mode> &r, std::string const &i, std::string const &j) {
              return r.parameter(i, j);
            },
            py::arg("i"), py::arg("j"))
        .def("sro", [](sqs_result_wrapper<T, Mode> &r) { return r.parameter(); })
        .def("shell_index", &sqs_result_wrapper<T, Mode>::shell_index, py::arg("shell"))
        .def("species_index", &sqs_result_wrapper<T, Mode>::species_index, py::arg("species"))
        .def("rank", &sqs_result_wrapper<T, Mode>::rank, py::arg("base") = 10);
  }
  if constexpr (Mode == SUBLATTICE_MODE_SPLIT) {
    py::class_<sqs_result_wrapper<T, Mode>>(
        m, format_prec<format_sublattice<Name, Mode>(), T>().c_str())
        .def_readonly("objective", &sqs_result_wrapper<T, Mode>::objective)
        .def("structure", &sqs_result_wrapper<T, Mode>::structure)
        .def("sublattices", [](sqs_result_wrapper<T, Mode> &self) { return self.sublattices; });
  }
}

template <string_literal Name, class T, sqsgen::SublatticeMode Mode>
void bind_result_pack(py::module &m) {
  using namespace sqsgen;
  using namespace sqsgen::core;
  using namespace sqsgen::core::detail;
  py::class_<sqs_result_pack<T, Mode>>(m, format_prec<format_sublattice<Name, Mode>(), T>().c_str())
      //.def("results", &sqs_result_pack<T, Mode>::results)
      .def_readonly("statistics", &sqs_result_pack<T, Mode>::statistics)
      .def_readonly("config", &sqs_result_pack<T, Mode>::config)
      .def("__iter__",
           [](sqs_result_pack<T, Mode> &self) {
             return py::make_iterator(self.begin(), self.end());
           })
      .def("__len__", [](sqs_result_pack<T, Mode> &self) { return self.size(); })
      .def("num_objectives", &sqs_result_pack<T, Mode>::size)
      .def("num_results", &sqs_result_pack<T, Mode>::num_results)
      //.def("bytes", &to_bytes<sqs_result_pack<T, Mode>>)
      //.def_static("from_bytes", &from_bytes<sqs_result_pack<T, Mode>>, py::arg("bytes"))
      .def("best", [](sqs_result_pack<T, Mode> &self) {
        if (self.results.empty()) throw std::out_of_range("Cannot access empty result set");
        auto [_, set] = *self.begin();
        if (set.empty()) throw std::out_of_range("Cannot access empty result set");
        return set.front();
      });
}

PYBIND11_MODULE(_core, m) {
  using namespace sqsgen;
  m.doc() = "pybind11 example plugin";  // Optional module docstring

#if defined(SQSGEN_MAJOR_VERSION) && defined(SQSGEN_MINOR_VERSION) && defined(SQSGEN_BUILD_NUMBER) \
    && defined(SQSGEN_BUILD_BRANCH) && defined(SQSGEN_BUILD_COMMIT)
  m.attr("__version__")
      = py::make_tuple(SQSGEN_MAJOR_VERSION, SQSGEN_MINOR_VERSION, SQSGEN_BUILD_NUMBER,
                       stringify(SQSGEN_BUILD_BRANCH), stringify(SQSGEN_BUILD_COMMIT));
#endif

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

  py::enum_<spdlog::level::level_enum>(m, "LogLevel")
      .value("debug", spdlog::level::debug)
      .value("info", spdlog::level::info)
      .value("warn", spdlog::level::warn)
      .value("trace", spdlog::level::trace)
      .value("critical", spdlog::level::critical)
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
      .def_static("from_symbol", &core::atom::from_symbol, py::arg("symbol"))
      .def(
          "__lt__", [](core::atom &a, core::atom &b) { return a.Z < b.Z; }, py::is_operator())
      .def(
          "__eq__", [](core::atom &a, core::atom &b) { return a.Z == b.Z; }, py::is_operator())
      .def("__repr__", [](core::atom &a) -> std::string {
        return std::format("Atom(symbol=\"{}\", Z={}, mass={})", a.symbol, a.Z, a.mass);
      });

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

  py::class_<core::atom_pair<usize_t>>(m, "AtomPair")
      .def_readonly("i", &core::atom_pair<usize_t>::i)
      .def_readonly("j", &core::atom_pair<usize_t>::j)
      .def_readonly("shell", &core::atom_pair<usize_t>::shell);

  bind_sqs_statistics_data<"SqsStatisticsData", float>(m);
  bind_sqs_statistics_data<"SqsStatisticsData", double>(m);

  bind_structure<"Structure", float>(m);
  bind_structure<"Structure", double>(m);

  bind_callback_context<"SqsCallbackContext", float>(m);
  bind_callback_context<"SqsCallbackContext", double>(m);

  bind_sro_parameter<"SroParameter", float>(m);
  bind_sro_parameter<"SroParameter", double>(m);

  bind_site<"Site", float>(m);
  bind_site<"Site", double>(m);

  bind_result<"SqsResult", float, SUBLATTICE_MODE_INTERACT>(m);
  bind_result<"SqsResult", float, SUBLATTICE_MODE_SPLIT>(m);
  bind_result<"SqsResult", double, SUBLATTICE_MODE_INTERACT>(m);
  bind_result<"SqsResult", double, SUBLATTICE_MODE_SPLIT>(m);

  m.def(
      "parse_config",
      [](py::dict const &config) { return unwrap(io::config::parse_config(py::handle(config))); },
      py::arg("config"));

  m.def(
      "parse_config",
      [](std::string const &json) {
        py::gil_scoped_release nogil{};
        nlohmann::json document = nlohmann::json::parse(json);
        return unwrap(io::config::parse_config(document));
      },
      py::arg("config_json"));

  m.def(
      "optimize",
      [](std::variant<core::configuration<float>, core::configuration<double>> &&config,
         spdlog::level::level_enum log_level, std::optional<sqs_callback_t> callback) {
        if (callback.has_value()) {
          py::gil_scoped_release nogil{};
          return sqsgen::run_optimization(std::forward<decltype(config)>(config), log_level,
                                          callback);
        } else {
          return sqsgen::run_optimization(std::forward<decltype(config)>(config), log_level,
                                          std::nullopt);
        }
      },
      py::arg("config"), py::arg("log_level") = spdlog::level::level_enum::warn,
      py::arg("callback") = std::nullopt);

  bind_configuration<"SqsConfiguration", float>(m);
  bind_configuration<"SqsConfiguration", double>(m);

  bind_result_pack<"SqsResultPack", float, SUBLATTICE_MODE_INTERACT>(m);
  bind_result_pack<"SqsResultPack", float, SUBLATTICE_MODE_SPLIT>(m);
  bind_result_pack<"SqsResultPack", double, SUBLATTICE_MODE_INTERACT>(m);
  bind_result_pack<"SqsResultPack", double, SUBLATTICE_MODE_SPLIT>(m);
}
