//
// Created by dominik on 30.06.21.
//

#ifndef SQSGENERATOR_SQS_HPP
#define SQSGENERATOR_SQS_HPP


#include "types.hpp"
#include "settings.hpp"
#include "rank.hpp"
#include "result.hpp"



using namespace sqsgenerator::utils;
using namespace sqsgenerator::utils::atomistics;


namespace sqsgenerator {

    std::vector<SQSResult> do_pair_iterations(const IterationSettings &settings);
}
#endif //SQSGENERATOR_SQS_HPP
