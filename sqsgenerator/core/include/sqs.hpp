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

    std::tuple<std::vector<SQSResult>, timing_map_t> do_pair_iterations(const IterationSettings &settings);
    SQSResult do_pair_analysis(const IterationSettings &settings);
}
#endif //SQSGENERATOR_SQS_HPP
