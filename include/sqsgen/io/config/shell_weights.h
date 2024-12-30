//
// Created by Dominik Gehringer on 02.12.24.
//

#ifndef SQSGEN_IO_CONFIG_SHELL_WEIGHTS_H
#define SQSGEN_IO_CONFIG_SHELL_WEIGHTS_H

#include <pybind11/pybind11.h>

#include <nlohmann/json.hpp>

#include "sqsgen/config.h"
#include "sqsgen/core/helpers.h"
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
              std::format("Could not parse shell index: {}", e.what()));
        } catch (std::out_of_range const& e) {
          return parse_error::from_msg<key, CODE_BAD_VALUE>(
              std::format("Could not parse shell index: {}", e.what()));
        }
      } else if constexpr (std::is_same_v<std::decay_t<Document>, pybind11::handle>
                           || std::is_same_v<std::decay_t<Document>, pybind11::object>) {
        return accessor<Document>::template get_as<KEY_NONE, usize_t>(value);
      } else
        return parse_error::from_msg<key, CODE_TYPE_ERROR>("Unknown document type");
    }

    template <string_literal key, class T, class Document>
    parse_result<shell_weights_t<T>> parse_weights(Document const& doc, auto num_shells) {
      if (!accessor<Document>::is_document(doc))
        parse_error::from_msg<key, CODE_BAD_VALUE>("\"shell_weights\" must be a JSON object");
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
                         return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(std::format(
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

    template <class T> weights_t<T> default_weights(std::vector<usize_t> const& num_shells) {
      return as<std::vector>{}(num_shells | views::transform([](auto nshells) {
                                 return as<std::map>{}(
                                     core::helpers::range<usize_t>({1, nshells - 1})
                                     | views::transform([&](usize_t&& i) {
                                         return std::make_tuple(i, 1.0 / static_cast<T>(i));
                                       }));
                               }));
    }
  }  // namespace detail

  template <string_literal key, class T, class Document>
  weights_t<T> parse_shell_weights(Document const& document,
                                   std::vector<usize_t> const& num_shells) {
    return get_optional<"sublattice_mode", SublatticeMode>(document)
        .value_or(parse_result<SublatticeMode>{SUBLATTIC_MODE_INTERACT})
        .and_then([&](auto&& mode) -> weights_t<T> {
          // in case the "key" is not present we return the default value
          if (!accessor<Document>::contains(document, key.data))
            return detail::default_weights<T>(num_shells);
          auto doc = accessor<Document>::get(document, key.data);
          if (mode == SUBLATTIC_MODE_INTERACT)
            return detail::parse_weights<key, T>(doc, num_shells[0])
                .and_then([](auto&& w) -> weights_t<T> { return std::vector{w}; });
          if (mode == SUBLATTIC_MODE_SPLIT) {
            std::vector<shell_weights_t<T>> w;
            if (accessor<std::decay_t<decltype(doc)>>::is_document(doc)) {
              for (auto&& nshells : num_shells) {
                auto r = detail::parse_weights<key, T>(doc, nshells);
                if (r.failed()) return r.error();
                w.push_back(r.result());
              }
              return w;
            }
            if (accessor<std::decay_t<decltype(doc)>>::is_list(doc)) {
              auto index = 0;
              for (auto&& d : doc) {
                if (index >= num_shells.size())
                  return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
                      "You have specified more values than sublattices availabe");
                auto r = detail::parse_weights<key, T>(doc, num_shells[index]);
                if (r.failed()) return r.error();
                w.push_back(r.result());
                index++;
              }
              return w;
            }
          }
          return parse_error::from_msg<key, CODE_BAD_VALUE>(
              R"(Invalid sublattice mode. Must be either "interact" or "split")");
        });
  }

}  // namespace sqsgen::io::config

#endif  // SQSGEN_IO_CONFIG_SHELL_WEIGHTS_H
