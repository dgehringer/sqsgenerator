//
// Created by dominik on 08.07.21.
//

#ifndef SQSGENERATOR_SETTINGS_HPP
#define SQSGENERATOR_SETTINGS_HPP

#include "types.hpp"
#include "atomistics.hpp"

using namespace sqsgenerator::utils::atomistics;

namespace sqsgenerator::utils {

    class IterationSettings {

    private:
        size_t m_nspecies;
        size_t m_iterations;
        Structure m_structure;

        std::tuple<configuration_t, configuration_t> remap_configuration() {

        }
    public:
        const Structure& structure() const;


}

#endif //SQSGENERATOR_SETTINGS_HPP
