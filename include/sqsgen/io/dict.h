//
// Created by Dominik Gehringer on 07.12.24.
//

#ifndef SQSGEN_IO_DICT_H
#define SQSGEN_IO_DICT_H

#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "sqsgen/core/structure.h"
#include "sqsgen/io/config/structure.h"

namespace sqsgen::io {
  namespace py = pybind11;

  template <> struct accessor<py::handle> {
    static bool contains(py::handle const& d, std::string&& key) {
      if (py::isinstance<py::dict>(d)) return d.contains(std::forward<std::string>(key));
      return false;
    }

    static auto get(py::handle const& d, std::string&& key) {
      if (contains(d, std::forward<std::string>(key))) {
        return py::handle(d[key.c_str()]);
      }
      throw std::out_of_range(std::forward<std::string>(key));
    }

    static bool is_document(py::handle const& o) { return py::isinstance<py::dict>(o); }

    static bool is_list(py::handle const& o) { return py::isinstance<py::list>(o); }

    static auto range(py::handle const& o) {
      std::vector<py::handle> vec;
      auto it = py::iter(o);
      while (it != py::iterator::sentinel()) {
        vec.push_back(*it);
        ++it;
      }
      return vec;
    }

    static auto items(py::handle const& d) {
      if (is_document(d)) {
        std::vector<std::pair<py::handle, py::handle>> items;
        auto dict = d.cast<py::dict>();
        items.reserve(py::len(dict));
        for (auto const& item : dict) {
          // if (!py::isinstance<py::str>(item.first)) throw std::invalid_argument("dictionary is
          // only allowed to have strings as keys");
          items.emplace_back(item.first, item.second);
        };
        return items;
      };
      throw std::invalid_argument("object is not a dictionary");
    }

    template <string_literal key = "", class Option>
    static parse_result<Option> get_as(py::handle const& d) {
      try {
        if constexpr (key == KEY_NONE) {
          return d.cast<Option>();
        } else {
          if (d.contains(key.data)) return {get(d, key.data).template cast<Option>()};
          return parse_error::from_msg<key, CODE_NOT_FOUND>(
              format("could not find key %s", key.data));
        }
      } catch (py::cast_error const& e) {
        return parse_error::from_msg<key, CODE_TYPE_ERROR>(
            format("type error - cannot parse %s - %s", typeid(Option).name(), e.what()));
      } catch (std::out_of_range const& e) {
        return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(e.what());
      }
    }
  };

  template <> struct accessor<py::object> {
    static bool is_document(py::object const& o) { return accessor<py::handle>::is_document(o); }

    static bool contains(py::object const& d, std::string&& key) {
      return accessor<py::handle>::contains(d, std::forward<std::string>(key));
    }

    static auto items(py::object const& d) { return accessor<py::handle>::items(d); }

    static auto get(py::object const& d, std::string&& key) {
      return accessor<py::handle>::get(d, std::forward<std::string>(key));
    }

    static auto range(py::object const& d) { return accessor<py::handle>::range(d); }

    static auto is_list(py::object const& o) { return accessor<py::handle>::is_list(o); }

    template <string_literal key = "", class Option>
    static parse_result<Option> get_as(py::object const& d) {
      return accessor<py::handle>::get_as<key, Option>(d);
    }
  };

}  // namespace sqsgen::io

#endif  // SQSGEN_IO_DICT_H
