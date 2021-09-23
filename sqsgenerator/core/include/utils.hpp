//
// Created by dominik on 29.05.21.
//

#ifndef SQSGENERATOR_UTILS_HPP
#define SQSGENERATOR_UTILS_HPP

#include "types.hpp"
#include <sstream>
#include <stdexcept>
#include <boost/multi_array.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/storage.hpp>

using namespace boost;
using namespace boost::numeric::ublas;

namespace boost{
    template<class MultiArray>
    void extent_to(MultiArray & ma, const MultiArray & other){ //this function is adapted from
        auto& otherShape = reinterpret_cast<boost::array<size_t, MultiArray::dimensionality> const&>(*other.shape());
        ma.resize(otherShape);
    }

    template <typename T, typename F=row_major>
    matrix<T, F> matrix_from_vector(size_t m, size_t n, const std::vector<T> & v)
    {
        if(m*n!=v.size()) {
            throw std::invalid_argument("Matrix size does not match storage size");
        }
        unbounded_array<T> storage(m*n);
        std::copy(v.begin(), v.end(), storage.begin());
        return matrix<T>(m, n, storage);
    }


    template<typename MultiArray>
    std::vector<typename MultiArray::size_type> shape_from_multi_array(const MultiArray &a) {
        return std::vector<typename MultiArray::size_type>(a.shape(), a.shape() + a.num_dimensions());
    }

    // TODO: Probably unused function -> Maybe we should remove it

    template<typename MultiArrayRef, typename MultiArray>
    [[maybe_unused]] MultiArrayRef make_array_ref(MultiArray &array) {
        return MultiArrayRef(array.data(), shape_from_multi_array<MultiArray>(array));
    }


    template <typename MultiArray>
    matrix<typename MultiArray::element> matrix_from_multi_array(MultiArray &ref) {
        typedef typename MultiArray::index index_t;
        auto shape = shape_from_multi_array(ref);
        auto [sx, sy] = std::make_tuple(static_cast<index_t>(shape[0]), static_cast<index_t>(shape[1]));
        matrix<typename MultiArray::element> result(sx, sy);
        for (index_t i = 0; i < sx; i++) {
            for (index_t j = 0; j < sy; j++) {
                result(i,j) = ref[i][j];
            }
        }
        return result;
    }

    // Helper class to determine the full number of elements in the
    // multi-dimensional array
    template <std::size_t... vs> struct ArraySize;
    template <std::size_t v, std::size_t... vs> struct ArraySize<v, vs...>
    { static constexpr std::size_t size = v * ArraySize<vs...>::size; };
    template <> struct ArraySize<>
    { static constexpr std::size_t size = 1; };

    // Creates your multi_array
    template <typename T, int... dims>
    boost::multi_array<T, sizeof...(dims)>
    make_multi_array(std::initializer_list<T> l)
    {
        constexpr std::size_t asize = ArraySize<dims...>::size;
        assert(l.size() == asize); // could be a static assert in C++14

        // Dump data into a vector (because it has the right kind of ctor)
        const std::vector<T> a(l);
        // This can be used in a multi_array_ref ctor.
        boost::const_multi_array_ref<T, sizeof...(dims)> mar(
                &a[0],
                std::array<int, sizeof...(dims)>{dims...});
        // Finally, deep-copy it into the structure we can return.
        return boost::multi_array<T, sizeof...(dims)>(mar);
    }

    template<typename T1, typename T2, typename T1Cast = T1, typename T2Cast = T2>
    std::string format_dict(std::vector<T1> keys, std::vector<T2> values) {
        assert(keys.size() == values.size());
        std::stringstream message;
        message << "{";
        for (size_t i = 0; i < keys.size()-1 ; i++) message << static_cast<T1Cast>(keys[i]) << ": " << static_cast<T2Cast>(values[i]) << ", ";
        message << static_cast<T1Cast>(keys.back()) << ": " << static_cast<T2Cast>(values.back()) << "}";
        return message.str();
    }

    template<typename T1, typename T1Cast = T1>
    std::string format_vector(std::vector<T1> values) {
        std::stringstream message;
        message << "{";
        for (auto it = values.begin(); it != values.end() - 1; ++it) message << static_cast<T1Cast>(*it) << ", ";
        message << static_cast<T1Cast>(values.back()) << "}";
        return message.str();
    }

    template<typename MultiArray>
    std::vector<typename MultiArray::element> to_flat_vector(const MultiArray &array) {
        return std::vector<typename MultiArray::element>(array.data(), array.data() + array.num_elements());
    }

    template<typename MultiArray, typename T1 = typename MultiArray::element>
    std::string format_matrix(MultiArray array) {
        typedef typename MultiArray::index index_t;
        auto shape_ = shape_from_multi_array(array);
        std::vector<index_t> shape {static_cast<index_t>(shape_[0]), static_cast<index_t>(shape_[1]) };
        std::stringstream message;
        message << "[";
        for (index_t i = 0; i < shape[0]; i++) {
            message << "[";
            for (index_t j = 0; j < shape[1]; j++) {
                message << static_cast<T1>(array[i][j]);
                if (j != shape[1] - 1) message << ", ";
            }
            message << "]";
            if (i != shape[0] -1) message << std::endl;
        }
        return message.str();
    }



}

namespace sqsgenerator::utils {

        template<typename T>
        inline
        bool is_close(T a, T b, T atol=1.0e-5, T rtol=1.0e-8) {
            return std::abs(a - b) <= (atol + rtol * std::abs(b));
        }

        template<typename R, typename P>
        inline R factorial(P p) {
            R r {1}, c {1};
            for (; c <= p; c += 1 ) r *= c;
            return r;
        }

        template<typename T>
        T identity(T &t) {
            return t;
        }

        template<typename T>
        T round_nplaces(T value, uint8_t to)
        {
            T places = std::pow(10.0, to);
            return std::round(value * places) / places;
        }

        template<typename T>
        int get_index(std::vector<T> v, T el)
        {
            auto it = std::find(v.begin(), v.end(), el);
            return  it != v.end() ? it - v.begin() : -1;
        }

        template<typename TOut, typename TIn>
        std::vector<TOut> apply(const std::vector<TIn> &input, std::function<TOut(TIn)> f) {
            std::vector<TOut> result;
            std::transform(input.begin(),  input.end(), std::back_inserter(result), f);
            return result;
        }

        template<typename T>
        std::vector<size_t> argsort(const std::vector<T> &array) {
            std::vector<size_t> indices(array.size());
            std::iota(indices.begin(), indices.end(), 0);
            std::sort(indices.begin(), indices.end(),
                      [&array](int left, int right) -> bool {
                          // sort indices according to corresponding array element
                          return array[left] < array[right];
                      });

            return indices;
        }

    template<typename Iterable>
    class enumerate_object
    {
    private:
        Iterable _iter;
        std::size_t _size;
        decltype(std::begin(_iter)) _begin;
        const decltype(std::end(_iter)) _end;

        public:
            enumerate_object(Iterable iter):
                    _iter(iter),
                    _size(0),
                    _begin(std::begin(iter)),
                    _end(std::end(iter))
            {}

            const enumerate_object& begin() const { return *this; }
            const enumerate_object& end()   const { return *this; }

            bool operator!=(const enumerate_object&) const
            {
                return _begin != _end;
            }

            void operator++()
            {
                ++_begin;
                ++_size;
            }

            auto operator*() const-> std::pair<std::size_t, decltype(*_begin)>
            {
                return { _size, *_begin };
            }
        };

        template<typename Iterable>
        auto enumerate(Iterable&& iter)-> enumerate_object<Iterable>
        {
            return { std::forward<Iterable>(iter) };
        }

        configuration_t unique_species(configuration_t conf);
        std::vector<size_t> configuration_histogram(const configuration_t &conf);
        std::tuple<configuration_t, configuration_t> pack_configuration(const configuration_t &configuration);
        configuration_t unpack_configuration(const configuration_t &indices, const configuration_t &packed);

        // random number generator
        uint32_t random_bounded(uint32_t range, uint64_t *seed);
        void shuffle_configuration(configuration_t &configuration, uint64_t *seed, const shuffling_bounds_t &bounds);
};


#endif //SQSGENERATOR_UTILS_HPP
