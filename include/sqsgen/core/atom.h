//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Dominik Gehringer
//
// This file is part of sqsgenerator, distributed under the MIT License.
// See the LICENSE file in the project root for full license information.
//

#ifndef SQSGEN_CORE_ATOM_H
#define SQSGEN_CORE_ATOM_H

#include <concepts>
#include <string>
#include <tuple>

#include "sqsgen/types.h"

namespace sqsgen::core {

  namespace detail {
    using atom_record
        = std::tuple<specie_t, std::string, std::string, std::string, double, double, double>;
  }  // namespace detail

  class atom {
  public:
    specie_t Z;         /**< The ordinal number of the element in the periodic table */
    std::string name;   /**< The full english name of the element. E. g. "Iron" */
    std::string symbol; /**< The two character symbol of an element. E.g "Fe" for iron */
    std::string electronic_configuration; /**< The electronic configuration as a string */
    double atomic_radius;                 /**< Atomic radius in pm */
    double mass;                          /**< The mass of the atoms in atomic units */
    double en;                            /**< Electronegativity */

    template <std::integral T> static atom from_z(T ordinal);

    static atom from_symbol(std::string const& symbol);

  private:
    explicit atom(detail::atom_record const& record);
  };

}  // namespace sqsgen::core

#include "sqsgen/impl/core/atom_impl.hpp"

#endif  // SQSGEN_CORE_ATOM_H
