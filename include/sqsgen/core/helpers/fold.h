//
// Created by Dominik Gehringer on 22.10.24.
//

#ifndef SQSGEN_CORE_HELPERS_FOLD_H
#define SQSGEN_CORE_HELPERS_FOLD_H

namespace sqsgen::core::helpers {
  struct fold_left_fn {
    template <std::input_iterator I, std::sentinel_for<I> S, class T = std::iter_value_t<I>,
              class F>
    constexpr auto operator()(I first, S last, T init, F f) const {
      using U = std::decay_t<std::invoke_result_t<F&, T, std::iter_reference_t<I>>>;
      if (first == last) return U(std::move(init));
      U accum = std::invoke(f, std::move(init), *first);
      for (++first; first != last; ++first) accum = std::invoke(f, std::move(accum), *first);
      return std::move(accum);
    }

    template <std::ranges::input_range R, class T = std::ranges::range_value_t<R>, class F>
    constexpr auto operator()(R&& r, T init, F f) const {
      return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(init), std::ref(f));
    }
  };

  inline constexpr fold_left_fn fold_left;

  struct fold_left_first_fn {
    template <std::input_iterator I, std::sentinel_for<I> S, class F>
      requires std::constructible_from<std::iter_value_t<I>, std::iter_reference_t<I>>
    constexpr auto operator()(I first, S last, F f) const {
      using U = decltype(fold_left(std::move(first), last, std::iter_value_t<I>(*first), f));
      if (first == last) return std::optional<U>();
      std::optional<U> init(std::in_place, *first);
      for (++first; first != last; ++first) *init = std::invoke(f, std::move(*init), *first);
      return std::move(init);
    }

    template <std::ranges::input_range R, class F>
      requires std::constructible_from<std::ranges::range_value_t<R>,
                                       std::ranges::range_reference_t<R>>
    constexpr auto operator()(R&& r, F f) const {
      return (*this)(std::ranges::begin(r), std::ranges::end(r), std::ref(f));
    }
  };

  inline constexpr fold_left_first_fn fold_left_first;
};  // namespace sqsgen::core::helpers

#endif  // SQSGEN_CORE_HELPERS_FOLD_H
