//
// Created by dominik on 30.06.21.
//

#ifndef SQSGENERATOR_SQS_HPP
#define SQSGENERATOR_SQS_HPP


#include "types.hpp"
#include "settings.hpp"
#include "rank.hpp"
#include "result.hpp"
#include <map>
#include <omp.h>
#include <limits>
#include <chrono>
#include <vector>
#include <sstream>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <boost/circular_buffer.hpp>



using namespace sqsgenerator::utils;
using namespace sqsgenerator::utils::atomistics;


namespace sqsgenerator {

    std::vector<SQSResult> do_pair_iterations(const IterationSettings &settings);
}
#endif //SQSGENERATOR_SQS_HPP
