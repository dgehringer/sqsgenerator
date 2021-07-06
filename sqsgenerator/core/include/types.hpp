//
// Created by dominik on 21.05.21.
//

#ifndef SQSGENERATOR_TYPES_H
#define SQSGENERATOR_TYPES_H
#include <vector>
#include <map>
#include <array>
#include <cstdint>
#include <boost/multi_array.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/storage.hpp>

namespace sqsgenerator {
    typedef uint8_t Species;
    typedef uint8_t Shell;
    typedef std::vector<Species> Configuration;
    typedef boost::multi_array<double, 3> PairSROParameters;
    typedef boost::multi_array<double, 6> TripletSROParameters;
    typedef std::vector<double> ParameterStorage;
    template<size_t NDims>
    using Shape = std::array<size_t, NDims>;
    typedef boost::multi_array<Shell, 2> PairShellMatrix;
    // The array consists of {size_t i, size_t j, size_t shell, size_t shell_index}
    typedef std::array<PairShellMatrix::index, 4> AtomPair;

}
#endif //SQSGENERATOR_TYPES_H
