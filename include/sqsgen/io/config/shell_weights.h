//
// Created by Dominik Gehringer on 02.12.24.
//

#ifndef SQSGEN_IO_CONFIG_SHELL_WEIGHTS_H
#define SQSGEN_IO_CONFIG_SHELL_WEIGHTS_H

#ifdef WITH_PYTHON
#  include <pybind11/pybind11.h>
#endif
#define NLOHMANN_JSON_ABI_TAG v3_11_3
#include <nlohmann/json.hpp>

#include "sqsgen/core/helpers.h"
#include "sqsgen/io/config/shared.h"
#include "sqsgen/io/parsing.h"

namespace sqsgen::io::config {

  using namespace sqsgen::core::helpers;

  template <class T> using weights_t = parse_result<std::vector<shell_weights_t<T>>>;

  namespace detail {
    template <string_literal key, class Document>
    parse_result<usize_t> parse_shell_index(auto value) {
      if constexpr (std::is_same_v<std::decay_t<Document>, nlohmann::json>) {
        try {
          return static_cast<usize_t>(std::stoul(value));
        } catch (std::invalid_argument const& e) {
          return parse_error::from_msg<key, CODE_BAD_VALUE>(
              fmt::format("Could not parse shell index: {}", e.what()));
        } catch (std::out_of_range const& e) {
          return parse_error::from_msg<key, CODE_BAD_VALUE>(
              fmt::format("Could not parse shell index: {}", e.what()));
        }
      }
#ifdef WITH_PYTHON
      if constexpr (std::is_same_v<std::decay_t<Document>, pybind11::handle>
                    || std::is_same_v<std::decay_t<Document>, pybind11::object>)
        return accessor<Document>::template get_as<KEY_NONE, usize_t>(value);
#endif

      return parse_error::from_msg<key, CODE_TYPE_ERROR>("Unknown document type");
    }

    template <string_literal key, class T, class Document>
    parse_result<shell_weights_t<T>> parse_weights(Document const& doc, auto num_shells) {
      if (!accessor<Document>::is_document(doc))
        return parse_error::from_msg<key, CODE_BAD_VALUE>("\"shell_weights\" must be an object");
      shell_weights_t<T> weights;
      for (auto&& [si, w] : accessor<Document>::items(doc)) {
        auto r = parse_shell_index<key, Document>(si)
                     .combine(accessor<Document>::template get_as<KEY_NONE, T>(w))
                     .and_then([&](auto&& i_and_w) -> parse_result<std::tuple<usize_t, T>> {
                       auto shell_index = std::get<0>(i_and_w);
                       if (shell_index == 0)
                         return parse_error::from_msg<key, CODE_BAD_VALUE>(
                             "There is no point in including self-interactions");
                       if (shell_index >= num_shells)
                         return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(fmt::format(
                             "There are {} shells but you specified a shell index of {}",
                             num_shells, shell_index));
                       return i_and_w;
                     });
        if (r.failed()) return r.error().with_key(key.data);
        auto [shell_index, weight] = r.result();
        weights[shell_index] = weight;
      }
      if (weights.empty())
        return parse_error::from_msg<key, CODE_OUT_OF_RANGE>("shell weights cannot be empty");
      return weights;
    }

    template <class T>
    std::vector<shell_weights_t<T>> default_weights(std::vector<usize_t> const& num_shells) {
      return as<std::vector>{}(num_shells | views::transform([](auto nshells) {
                                 return as<std::map>{}(core::helpers::range<usize_t>({1, nshells})
                                                       | views::transform([&](usize_t&& i) {
                                                           return std::make_tuple(
                                                               i, T(1.0) / static_cast<T>(i));
                                                         }));
                               }));
    }
  }  // namespace detail

  template <string_literal key, class T, class Document>
  weights_t<T> parse_shell_weights(Document const& document, SublatticeMode mode,
                                   stl_matrix_t<T> const& shell_radii) {
    const auto num_shells = as<std::vector>{}(
        shell_radii | views::transform([](auto&& r) { return static_cast<usize_t>(r.size()); }));
    if (!accessor<Document>::contains(document, key.data))
      return detail::default_weights<T>(num_shells);

    auto doc = accessor<Document>::get(document, key.data);
    return parse_for_mode<key>(
        [&] {
          return detail::parse_weights<key, T>(doc, num_shells[0])
              .and_then([](auto&& w) -> weights_t<T> { return std::vector{w}; });
        },
        [&]() -> weights_t<T> {
          using accessor_t = accessor<std::decay_t<decltype(doc)>>;
          if (accessor_t::is_document(doc)) {
            return lift<key>(
                [&](auto&& nshells) { return detail::parse_weights<key, T>(doc, nshells); },
                num_shells);
          }
          if (accessor_t::is_list(doc)) {
            return lift<key>(
                [](auto&& subdoc, auto&& nshells) {
                  return detail::parse_weights<key, T>(subdoc, nshells);
                },
                accessor_t::range(doc), num_shells);
          }
          return parse_error::from_msg<key, CODE_BAD_VALUE>("Cannot interpret value");
        },
        mode);
  }

}  // namespace sqsgen::io::config

#endif  // SQSGEN_IO_CONFIG_SHELL_WEIGHTS_H
