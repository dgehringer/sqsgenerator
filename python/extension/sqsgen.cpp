//
// Created by Dominik Gehringer on 08.03.25.
//

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "sqsgen/types.h"

namespace py = pybind11;

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
}