//
// Created by Dominik Gehringer on 11.11.24.
//

#ifndef ENUMERATE_H
#define ENUMERATE_H
namespace sqsgen::core::helpers {
  namespace ranges = std::ranges;
  namespace views = ranges::views;
  template <class U, ranges::range R>
    requires std::is_integral_v<U>
  auto enumerate(R&& r) {
    U index{0};
    return r | views::transform([&](auto const& e) { return std::make_pair(index++, e); });
  }
}  // namespace sqsgen::core::helpers
#endif  // ENUMERATE_H
