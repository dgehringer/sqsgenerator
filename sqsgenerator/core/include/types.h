//
// Created by dominik on 21.05.21.
//

#ifndef SQSGENERATOR_TYPES_H
#define SQSGENERATOR_TYPES_H

#include <vector>
#include <stdint.h>

namespace sqsgenerator {
    typedef uint8_t Species;
    typedef std::vector<Species> Configuration;
    typedef std::vector<double> SROParameters;
    typedef std::tuple<double, Configuration, SROParameters> SQSResult;
}
#endif //SQSGENERATOR_TYPES_H
