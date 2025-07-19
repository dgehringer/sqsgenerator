//
// Created by Dominik Gehringer on 11.03.25.
//

#ifndef EXTENSION_UTILS_H
#define EXTENSION_UTILS_H

#include <pybind11/cast.h>
#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>

#include <fstream>
#include <ranges>
#include <sstream>
#include <string>

namespace sqsgen::python::helpers {

  inline std::string read_file(const std::string &filePath) {
    std::ifstream fileStream(filePath);
    if (!fileStream) throw std::runtime_error(fmt::format("Could not open file '{}'", filePath));

    std::stringstream buffer;
    buffer << fileStream.rdbuf();
    return buffer.str();
  }

  auto format_ordinal(auto z) -> std::string {
    std::string_view subscript = "0123456789";
    auto r{z};
    std::string out;
    out.reserve(3);
    while (r > 9) {
      out.push_back(subscript[r % 10]);
      r /= 10;
    }
    out.push_back(subscript[r]);
    std::reverse(out.begin(), out.end());
    return out;
  }

}  // namespace sqsgen::python::helpers

namespace pybind11 {
  PYBIND11_NAMESPACE_BEGIN(detail)
  template <class Access, return_value_policy Policy, std::ranges::range R, class... Extra>
  struct range_iterator_wrapper {
    using value_type = std::ranges::range_value_t<R>;
    std::unique_ptr<R> _range;
    decltype(std::ranges::begin(std::declval<R &>())) it;
    decltype(std::ranges::end(std::declval<R &>())) end;
    bool first_or_done;

    explicit range_iterator_wrapper(R &&r)
        : _range(std::make_unique<R>(std::move(r))),
          it(std::ranges::begin(*_range)),
          end(std::ranges::end(*_range)),
          first_or_done(true) {}
  };

  template <typename Access, return_value_policy Policy, std::ranges::range Range,
            typename... Extra>
  iterator make_range_iterator_impl(Range &&range, Extra &&...extra) {
    using state = detail::range_iterator_wrapper<Access, Policy, Range, Extra...>;
    using value_type = typename state::value_type;
    // TODO: state captures only the types of Extra, not the values

    if (!detail::get_type_info(typeid(state), false)) {
      class_<state>(handle(), "range_iterator", pybind11::module_local())
          .def("__iter__", [](state &s) -> state & { return s; })
          .def(
              "__next__",
              [](state &s) -> value_type {
                if (!s.first_or_done) {
                  ++s.it;
                } else {
                  s.first_or_done = false;
                }
                if (s.it == s.end) {
                  s.first_or_done = true;
                  throw stop_iteration();
                }
                return Access()(s.it);
                // NOLINTNEXTLINE(readability-const-return-type) // PR #3263
              },
              std::forward<Extra>(extra)..., Policy);
    }

    return cast(state{std::forward<Range>(range)});
  }
  PYBIND11_NAMESPACE_END(detail)

  template <return_value_policy Policy = return_value_policy::reference_internal,
            std::ranges::range Range, typename... Extra>
  typing::Iterator<std::ranges::range_value_t<Range>> make_range_iterator(Range &&range,
                                                                          Extra &&...extra) {
    using Iterator = decltype(std::ranges::begin(std::declval<Range &>()));
    return detail::make_range_iterator_impl<detail::iterator_access<Iterator>, Policy, Range,
                                            Extra...>(std::forward<Range>(range),
                                                      std::forward<Extra>(extra)...);
  }

}  // namespace pybind11

#endif  // EXTENSION_UTILS_H
