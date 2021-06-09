//
// Created by dominik on 21.05.21.
//

#ifndef SQSGENERATOR_TYPES_H
#define SQSGENERATOR_TYPES_H
#include <vector>
#include <stdint.h>
#include <boost/multi_array.hpp>

namespace sqsgenerator {
    typedef uint8_t Species;
    typedef std::vector<Species> Configuration;
    typedef boost::multi_array<double, 3> PairSROParameters;
    typedef boost::multi_array<double, 6> TripletSROParameters;
    typedef std::vector<double> ParameterStorage;

    template<size_t NDims>
    using Shape = std::array<size_t, NDims>;

}
#endif //SQSGENERATOR_TYPES_H
