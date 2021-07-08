//
// Created by dominik on 21.05.21.
//
#include "rank.hpp"
#include "utils.hpp"
#include <algorithm>


namespace sqsgenerator::utils {

        rank_t total_permutations(const configuration_t &conf) {
            auto permutations = factorial<rank_t, size_t>(conf.size());
            for (size_t &entry: configuration_histogram(conf)) {
                permutations /= factorial<rank_t, size_t>(entry);
            }
            return permutations;
        }

        rank_t rank_permutation(const configuration_t &conf, size_t nspecies) {
            species_t x;
            rank_t temp;
            rank_t suffix_permutations{1};
            rank_t rank{1};
            auto atoms = conf.size();
            std::vector<size_t> hist(nspecies, 0);

            for (size_t i = 0; i < atoms; i++) {
                x = conf[(atoms - 1) - i];
                hist[x]++;
                for (int j = 0; j < nspecies; j++) {
                    if (j < x) {
                        temp = suffix_permutations * hist[j];
                        temp /= hist[x];
                        rank += temp;
                    }
                }
                temp = suffix_permutations * (i + 1);
                suffix_permutations = temp / hist[x];
            }
            return rank;
        }

        void
        unrank_permutation(configuration_t &conf, std::vector<size_t> hist, rank_t total_permutations, rank_t rank) {
            if (rank > total_permutations) {
                throw std::out_of_range("The rank is larger than the total number of permutations");
            }
            size_t k{0};
            size_t atoms = {conf.size()};
            size_t nspecies{hist.size()};
            rank_t suffix_count;

            for (size_t i = 0; i < atoms; i++) {
                for (size_t j = 0; j < nspecies; j++) {
                    suffix_count = (total_permutations * hist[j]) / (atoms - i);
                    if (rank <= suffix_count) {
                        conf[k] = j;
                        total_permutations = suffix_count;
                        hist[j]--;
                        k++;
                        if (hist[j] == 0) {
                            j++;
                        }
                        break;
                    }
                    rank -= suffix_count;
                }
            }
        }

    }
