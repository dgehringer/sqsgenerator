//
// Created by dominik on 29.05.21.
//

#ifndef SQSGENERATOR_UTILS_H
#define SQSGENERATOR_UTILS_H

#include <stdexcept>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/storage.hpp>

using namespace boost::numeric::ublas;

namespace boost{
    template<class MultiArray>
    void extentTo(MultiArray & ma, const MultiArray & other){ //this function is adapted from
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

    template <typename T>
    matrix<T> matrix_from_multi_array(const_multi_array_ref<T, 2> &ref) {

    }
}

namespace sqsgenerator::utils {

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
}

#endif //SQSGENERATOR_UTILS_H
