//
// Created by dominik on 21.05.21.
//
#include <iostream>
#include <gmp.h>
#include <algorithm>
#include <unordered_map>
#include <rank.h>
#include <types.h>

using namespace sqsgenerator;

const Configuration unique_species(Configuration conf){
    // it is intended that we do not pass in by-reference since we want to have a copy of the configuration
    Configuration::iterator it;
    it = std::unique(conf.begin(), conf.end());
    conf.resize(std::distance(conf.begin(), it));
    return conf;
}

std::vector<size_t> configuration_histogram(const Configuration &conf) {
    auto uspcies = unique_species(conf);
    std::vector<size_t> hist(uspcies.size());
    for (int i = 0; i < uspcies.size(); i++) {
        hist[i] = std::count(conf.begin(), conf.end(), uspcies[i]);
    }
    return hist;
}

mpz_class total_permutations(const Configuration &conf){
    mpz_class permutations = mpz_class::factorial(conf.size());
    for (size_t &entry: configuration_histogram(conf)) {
        permutations /= mpz_class::factorial(entry);
    }
    return permutations;
}

mpz_class rank_permutation_gmp(const Configuration &conf, size_t nspecies) {
    Species x;
    mpz_class temp;
    mpz_class suffix_permutations {1};
    mpz_class rank {1};
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
        suffix_permutations = temp/hist[x];
    }
    return rank;
}

uint64_t rank_permutation_std(const Configuration &conf, size_t nspecies){
    Species x;
    uint64_t temp;
    uint64_t suffix_permutations {1};
    uint64_t rank {1};
    size_t atoms = conf.size();
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
        suffix_permutations = temp/hist[x];
    }
    return rank;
}

void unrank_permutation_gmp(Configuration &conf, std::vector<size_t> hist, mpz_class total_permutations, mpz_class rank) {

    if (rank > total_permutations) {
        throw std::out_of_range("The rank is larger than the total number of permutations");
    }

    size_t k {0};
    size_t atoms = { conf.size() };
    size_t nspecies { hist.size() };
    mpz_class suffix_count;

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

void unrank_permutation_std(Configuration &conf, std::vector<size_t> hist, uint64_t total_permutations, uint64_t rank) {
    if (rank > total_permutations) {
        throw std::out_of_range("The rank is larger than the total number of permutations");
    }

    size_t k {0};
    size_t atoms {conf.size()};
    size_t nspecies = hist.size();
    uint64_t suffix_count;

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


