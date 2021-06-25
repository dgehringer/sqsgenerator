//
// Created by dominik on 21.05.21.
//

#ifndef SQSGENERATOR_TYPES_H
#define SQSGENERATOR_TYPES_H
#include <vector>
#include <stdint.h>
#include <boost/multi_array.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/storage.hpp>

namespace sqsgenerator {
    typedef uint8_t Species;
    typedef std::vector<Species> Configuration;
    typedef boost::multi_array<double, 3> PairSROParameters;
    typedef boost::multi_array<double, 6> TripletSROParameters;
    typedef std::vector<double> ParameterStorage;
    template<typename T>
    using Matrix = boost::numeric::ublas::matrix<T, boost::numeric::ublas::row_major, std::vector<T>>;
    template<size_t NDims>
    using Shape = std::array<size_t, NDims>;

}
#endif //SQSGENERATOR_TYPES_H
