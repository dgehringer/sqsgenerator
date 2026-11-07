
#include "sqsgen/types.h"

namespace sqsgen::optimization {

  // everything the kernel needs for pairs
  template <class T, sublattice_mode S> struct pair_data {
    sublattice_mode_nesting<core::pair_list_t, S> pairs;  // precomputed (site, site, shell)
    sublattice_mode_nesting<pair_tensor_t<T>, S> prefactors;
    sublattice_mode_nesting<pair_tensor_t<T>, S> weights;  // w^i * p_{xi eta}
    sublattice_mode_nesting<pair_tensor_t<T>, S> target;
  };

  // everything the kernel needs for triplets
  template <class T, sublattice_mode S> struct triplet_data {
    sublattice_mode_nesting<core::triplet_list_t, S>
        triplets;  // precomputed (site, site, site, class)
    sublattice_mode_nesting<core::triplet_class_mapping_t, S> classes;
    sublattice_mode_nesting<triplet_tensor_t<T>, S> prefactors;
    sublattice_mode_nesting<triplet_tensor_t<T>, S> weights;  // w^c * q_{xi eta zeta}
    sublattice_mode_nesting<triplet_tensor_t<T>, S> target;
  };

  // what every run needs, plus the optional orders
  template <class T, sublattice_mode S> struct kernel_data {
    sublattice_mode_nesting<shuffler_t, S> shuffler;
    sublattice_mode_nesting<configuration_t, S> species_packed;
    std::optional<pair_data<T, S>> pairs;
    std::optional<triplet_data<T, S>> triplets;
    T lambda{0};  // weight between pairs and triplets
  };
}  // namespace sqsgen::optimization
