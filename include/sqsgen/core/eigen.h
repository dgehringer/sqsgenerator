
#ifndef SQSGEN_CORE_EIGEN_H
#define SQSGEN_CORE_EIGEN_H

#include "sqsgen/types.h"

namespace sqsgen::core {

  struct tensor_shape_error {
    enum class kind { empty_dimension, ragged, size_mismatch, negative_extent };
    kind which;
    std::string message;
  };

  namespace eigen::detail {
    template <class Tensor, std::size_t N>
    decltype(auto) at(Tensor&& t, const std::array<Eigen::Index, N>& idx) {
      return std::apply([&](auto... i) -> decltype(auto) { return t(i...); }, idx);
    }

    template <class T, int N> std::array<Eigen::Index, N> extents(const Eigen::Tensor<T, N>& t) {
      std::array<Eigen::Index, N> e{};
      for (int d = 0; d < N; ++d) e[d] = t.dimension(d);
      return e;
    }

    // advance a row-major multi-index in place; false once it wraps past the end.
    template <std::size_t N>
    bool next_index(std::array<Eigen::Index, N>& idx, const std::array<Eigen::Index, N>& shape) {
      for (int d = static_cast<int>(N) - 1; d >= 0; --d) {  // last axis fastest
        if (++idx[d] < shape[d]) return true;
        idx[d] = 0;
      }
      return false;
    }

    template <class T, int N, int M>
    void measure_nested(const nested_tensor<T, M>& v, std::array<Eigen::Index, N>& shape) {
      if constexpr (M > 0) {
        shape[N - M] = static_cast<Eigen::Index>(v.size());
        if (!v.empty()) measure_nested<T, N, M - 1>(v.front(), shape);
      }
    }

    // every sibling at each level must match the measured extent for that level.
    template <class T, int N, int M>
    bool is_rectangular(const nested_tensor<T, M>& v, const std::array<Eigen::Index, N>& shape) {
      if constexpr (M == 0) {
        return true;
      } else {
        if (static_cast<Eigen::Index>(v.size()) != shape[N - M]) return false;
        for (const auto& child : v)
          if (!is_rectangular<T, N, M - 1>(child, shape)) return false;
        return true;
      }
    }

    // copy nested values into the tensor at the running multi-index.
    template <class T, int N, int M> void fill_from_nested(const nested_tensor<T, M>& v,
                                                           Eigen::Tensor<T, N>& out,
                                                           std::array<Eigen::Index, N>& idx) {
      if constexpr (M == 0) {
        at(out, idx) = v;  // v is a scalar T
      } else {
        for (std::size_t i = 0; i < v.size(); ++i) {
          idx[N - M] = static_cast<Eigen::Index>(i);
          fill_from_nested<T, N, M - 1>(v[i], out, idx);
        }
      }
    }

    // build the nested array for the sub-block fixed by the leading coords of idx.
    template <class T, int N, int Level>
    nested_tensor<T, N - Level> build_nested(const Eigen::Tensor<T, N>& t,
                                             std::array<Eigen::Index, N>& idx) {
      if constexpr (Level == N) {
        return at(t, idx);  // scalar leaf
      } else {
        const Eigen::Index dim = t.dimension(Level);
        nested_tensor<T, N - Level> node(static_cast<std::size_t>(dim));
        for (Eigen::Index i = 0; i < dim; ++i) {
          idx[Level] = i;
          node[static_cast<std::size_t>(i)] = build_nested<T, N, Level + 1>(t, idx);
        }
        return node;
      }
    }

    template <class Format> struct tensor_converter {};

    template <class T, int N> struct tensor_converter<packed_tensor<T, N>> {
      packed_tensor<T, N> to(const Eigen::Tensor<T, N>& t) {
        packed_tensor<T, N> out;
        const auto shape = extents(t);
        for (int d = 0; d < N; ++d) out.shape[d] = static_cast<std::int64_t>(shape[d]);

        out.data.resize(static_cast<std::size_t>(t.size()));
        if (t.size() == 0) return out;

        std::array<Eigen::Index, N> idx{};
        std::size_t flat = 0;
        do {
          out.data[flat++] = at(t, idx);
        } while (next_index(idx, shape));
        return out;
      }

      std::expected<Eigen::Tensor<T, N>, tensor_shape_error> from(const packed_cube<T, N>& p) {
        std::array<Eigen::Index, N> shape{};
        std::int64_t count = 1;
        for (int d = 0; d < N; ++d) {
          if (p.shape[d] < 0)
            return std::unexpected(
                tensor_shape_error{tensor_shape_error::kind::negative_extent,
                                   "negative extent in dimension " + std::to_string(d)});
          shape[d] = static_cast<Eigen::Index>(p.shape[d]);
          count *= p.shape[d];
        }
        if (static_cast<std::int64_t>(p.data.size()) != count)
          return std::unexpected(tensor_shape_error{tensor_shape_error::kind::size_mismatch,
                                                    "data length " + std::to_string(p.data.size())
                                                        + " != shape product "
                                                        + std::to_string(count)});

        Eigen::Tensor<T, N> t;
        std::apply([&](auto... e) { t.resize(e...); }, shape);
        if (t.size() == 0) return t;

        std::array<Eigen::Index, N> idx{};
        std::size_t flat = 0;
        do {
          at(t, idx) = p.data[flat++];
        } while (next_index(idx, shape));
        return t;
      }

    }

    template <class T, int N>
    struct tensor_converter<nested_tensor<T, N>> {
      nested_tensor<T, N> to(const Eigen::Tensor<T, N>& t) {
        std::array<Eigen::Index, N> idx{};
        return build_nested<T, N, 0>(t, idx);
      }

      std::expected<Eigen::Tensor<T, N>, tensor_shape_error> from(
          const nested_tensor<T, N>& nested) {
        std::array<Eigen::Index, N> shape{};
        measure_nested<T, N, N>(nested, shape);

        for (int d = 0; d < N; ++d)
          if (shape[d] == 0)
            return std::unexpected(
                tensor_shape_error{tensor_shape_error::kind::empty_dimension,
                                   "dimension " + std::to_string(d) + " has length zero"});

        if (!is_rectangular<T, N, N>(nested, shape))
          return std::unexpected(tensor_shape_error{tensor_shape_error::kind::ragged,
                                                    "nested array is not rectangular"});

        Eigen::Tensor<T, N> t;
        std::apply([&](auto... e) { t.resize(e...); }, shape);

        std::array<Eigen::Index, N> idx{};
        fill_from_nested<T, N, N>(nested, t, idx);
        return t;
      }
    }
  }  // namespace eigen::detail

  template <class Target, class T, int N> Target tensor_as(const Eigen::Tensor<T, N>& t) {
    return tensor_converter<Target>::to(t);
  }

  template <class T, int N>
  std::expected<Eigen::Tensor<T, N>, tensor_shape_error> tensor_from(const packed_tensor<T, N>& p) {
    return tensor_converter<packed_tensor<T, N>>::from(p);
  }

  // nested -> Eigen::Tensor
  template <class T, int N>
  std::expected<Eigen::Tensor<T, N>, tensor_shape_error> tensor_from(const nested_tensor<T, N>& v) {
    return tensor_converter<nested_tensor<T, N>>::from(v);
  }

}  // namespace sqsgen::core

#endif  // SQSGEN_CORE_EIGEN_H
