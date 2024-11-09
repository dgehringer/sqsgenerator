//
// Created by Dominik Gehringer on 23.10.24.
//

#ifndef SQSGEN_CORE_HELPERS_TEMPLATES_H
#define SQSGEN_CORE_HELPERS_TEMPLATES_H

#include <ranges>
#include <variant>

namespace sqsgen::core::helpers {
  namespace ranges = std::ranges;
  namespace views = std::ranges::views;

  template <ranges::input_range R, class Fn, class T = ranges::range_value_t<R>,
            ranges::input_range U = std::invoke_result_t<Fn, T>>
  constexpr auto flat_map(R&& r, Fn&& fn) {
    return r | views::transform(std::forward<Fn>(fn)) | views::join;
  }

  template <class... Args, class Fn> constexpr auto map(std::tuple<Args...>&& tuple, Fn&& fn) {
    return [&]<std::size_t... Index>(std::index_sequence<Index...>, Fn&& f) {
      ;
      return std::make_tuple(f(std::forward<std::tuple_element_t<Index, std::tuple<Args...>>>(
          std::get<Index>(tuple)))...);
    }(std::make_index_sequence<sizeof...(Args)>{}, std::forward<Fn>(fn));
  }

  namespace detail {

    template <class, class> struct push_impl {};
    template <class T, class... Args> struct push_impl<T, std::tuple<Args...>> {
      using type = std::tuple<T, Args...>;
    };

    template <class, class> struct push_all_impl {};
    template <class T, class... Args> struct push_all_impl<T, std::tuple<Args...>> {
      using type = std::tuple<typename push_impl<T, std::tuple<Args>>::type...>;
    };
    template <class...> struct product_impl {};
    template <class... Args> struct product_impl<std::tuple<Args...>> {
      using type = std::tuple<Args...>;
    };

    template <class...> struct join_impl {};
    template <> struct join_impl<> {};
    template <class... T> struct join_impl<std::tuple<T...>> {
      using type = std::tuple<T...>;
    };
    template <class... T, class... U> struct join_impl<std::tuple<T...>, std::tuple<U...>> {
      using type = std::tuple<T..., U...>;
    };

    template <class T, class... U> struct join_impl<T, U...> {
      using type = typename join_impl<T, typename join_impl<U...>::type>::type;
    };
    template <class... Args, class... Args2>
    struct product_impl<std::tuple<Args...>, std::tuple<Args2...>> {
      static_assert(sizeof...(Args) == sizeof...(Args2));
      using type =
          typename join_impl<typename push_all_impl<Args, std::tuple<Args2...>>::type...>::type;
    };

    template <template <class...> class, template <class...> class, class> struct lift_impl {

    };
    template <template <class...> class Out, template <class...> class Lifter, class... Args> struct lift_impl<Out, Lifter, std::tuple<Args...>> {
      using type = Out<typename Lifter<Args>::type...>;
    };
  } // namespace detail

  template<class... Args>
  using product_t = typename detail::product_impl<Args...>::type;

  template<class... Args>
  using join_t = typename detail::join_impl<Args...>::type;

  template <template <class...> class Out, template <class...> class Lifter, class... Args>
  using lift_t = typename detail::lift_impl<Out, Lifter, Args...>::type;



}  // namespace sqsgen::core::helpers

#endif  // SQSGEN_CORE_HELPERS_TEMPLATES_H
