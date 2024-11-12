//
// Created by Dominik Gehringer on 11.11.24.
//

#ifndef SQSGEN_CORE_HELPERS_ZIP_H
#define SQSGEN_CORE_HELPERS_ZIP_H

#include <iostream>
#include <list>
#include <ranges>

namespace sqsgen::core::helpers {
  namespace ranges = std::ranges;
  namespace views = std::ranges::views;

  namespace detail {
    template <typename... Args, std::size_t... Index>
    auto any_match_impl(std::tuple<Args...> const& lhs, std::tuple<Args...> const& rhs,
                        std::index_sequence<Index...>) -> bool {
      auto result = false;
      result = (... | (std::get<Index>(lhs) == std::get<Index>(rhs)));
      return result;
    }

    template <typename... Args>
    auto any_match(std::tuple<Args...> const& lhs, std::tuple<Args...> const& rhs) -> bool {
      return any_match_impl(lhs, rhs, std::index_sequence_for<Args...>{});
    }

    template <ranges::viewable_range... Ranges> class zip_iterator {
    public:
      using value_t = std::tuple<ranges::range_reference_t<Ranges>...>;
      zip_iterator() = delete;
      explicit zip_iterator(ranges::iterator_t<Ranges>&&... iterators)
          : _iterators(std::forward<std::ranges::iterator_t<Ranges>>(iterators)...) {}

      auto operator++() -> zip_iterator& {
        std::apply([](auto&&... args) { ((++args), ...); }, _iterators);
        return *this;
      }

      auto operator++(int) -> zip_iterator {
        auto tmp = *this;
        std::apply([](auto&&... args) { ((++args), ...); }, _iterators);
        return tmp;
      }

      auto operator!=(zip_iterator const& other) const { return !(*this == other); }

      auto operator==(zip_iterator const& other) const {
        return any_match(_iterators, other._iterators);
      }

      auto operator*() -> value_t {
        return std::apply([](auto&&... args) { return value_t(*args...); }, _iterators);
      }

    private:
      std::tuple<ranges::iterator_t<Ranges>...> _iterators;
    };

    template <std::ranges::viewable_range... Ranges> class zipper {
    public:
      using zip_t = zip_iterator<Ranges...>;

      template <class... Args> explicit zipper(Args&&... args)
          : _ranges(std::forward<Args>(args)...) {}


      zip_t begin() {
        return std::apply([](auto&&... args) { return zip_t(ranges::begin(args)...); },
                          _ranges);
      }
      zip_t end()  {
        return std::apply([](auto&&... args) { return zip_t(ranges::end(args)...); },
                          _ranges);
      }

    private:
      std::tuple<Ranges...> _ranges;
    };
  }  // namespace detail

  template <std::ranges::viewable_range... R> auto zip(R&&... r) {
    return detail::zipper<R...>{std::forward<R>(r)...};
  }

}  // namespace sqsgen::core::helpers

#endif  // SQSGEN_CORE_HELPERS_ZIP_H
