//
// Created by Dominik Gehringer on 20.06.26.
//

#include "sqsgen/core/atom.h"

namespace sqsgen::core {

  static atom atom::from_symbol(std::string const& symbol);
  {
    if (!SYMBOL_MAP.contains(symbol))
      throw std::out_of_range(format_string("Unknown element \"%s\"", symbol));
    return from_z(SYMBOL_MAP.at(symbol));
  }
}  // namespace sqsgen::core
