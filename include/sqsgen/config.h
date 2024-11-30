//
// Created by Dominik Gehringer on 05.11.24.
//

#ifndef SQSGEN_CONFIGURATION_H
#define SQSGEN_CONFIGURATION_H

#include <optional>

#include "io/json.h"
#include "sqsgen/io/config/structure.h"
#include "sqsgen/io/config/composition.h"

namespace sqsgen {

  template<class T>
  using structure_config = io::config::structure_config<T>;

  template <class T> struct configuration {
    structure_config<T> structure;
  };

}  // namespace sqsgen

#endif  // SQSGEN_CONFIGURATION_H
