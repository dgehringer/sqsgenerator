//
// Created by dominik on 08.07.21.
//

#include "utils.hpp"

namespace sqsgenerator::utils {

    configuration_t unique_species(configuration_t conf) {
        // it is intended that we do not pass in by-reference since we want to have a copy of the configuration
        configuration_t::iterator it;
        it = std::unique(conf.begin(), conf.end());
        conf.resize(std::distance(conf.begin(), it));
        return conf;
    }

    std::vector<size_t> configuration_histogram(const configuration_t &conf) {
        auto uspcies = unique_species(conf);
        std::vector<size_t> hist(uspcies.size());
        for (int i = 0; i < uspcies.size(); i++) {
            hist[i] = std::count(conf.begin(), conf.end(), uspcies[i]);
        }
        return hist;
    }

    std::tuple<configuration_t, configuration_t> pack_configuration(const configuration_t &configuration) {
        auto unique_spec{unique_species(configuration)};
        std::sort(unique_spec.begin(), unique_spec.end());
        configuration_t remapped(configuration);

        for (size_t i = 0; i < configuration.size(); i++) {
            int index{get_index(unique_spec, configuration[i])};
            if (index < 0) throw std::runtime_error("A species was detected which I am not aware of");
            remapped[i] = index;
        }
        return std::make_tuple(unique_spec, remapped);
    }

    configuration_t unpack_configuration(const configuration_t &indices, const configuration_t &packed) {
        configuration_t unpacked;
        for (auto &index: packed) unpacked.push_back(indices[index]);
        return unpacked;
    }
}