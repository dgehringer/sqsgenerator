//
// Created by Dominik Gehringer on 04.03.25.
//

#ifndef SQSGEN_IMPL_IO_STRUCTURE_PDB_IMPL_HPP
#define SQSGEN_IMPL_IO_STRUCTURE_PDB_IMPL_HPP

#include <cstddef>
#include <format>
#include <string>

#include "sqsgen/core/atom.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::io {

  template <class T> std::string structure_adapter<T, io::structure_format::pdb>::format(
      const core::structure<T>& structure) {
    auto filtered = structure.without_vacancies();
    std::string result;
    result.reserve(filtered.size() * 144);

    const auto z_to_symbol = [](auto&& z) { return core::atom::from_z(z).symbol; };

    const auto println
        = [&result](std::string const& line) { result.append(std::format("{}\n", line)); };

    const auto [a, b, c] = detail::lengths<T>(filtered.lattice);
    const auto [alpha, beta, gamma] = detail::angles<T>(filtered.lattice);
    println(std::format("CRYST1{:9.3f}{:9.3f}{:9.3f}{:7.2f}{:7.2f}{:7.2f} P 1", a, b, c, alpha,
                        beta, gamma));

    coords_t<T> cart_coords = filtered.frac_coords * filtered.lattice.transpose();

    std::size_t maxnum = 100000;
    println("MODEL     0");
    for (std::size_t index = 0; index < filtered.size(); ++index) {
      auto row = cart_coords.row(index);
      auto symbol = z_to_symbol(filtered.species[index]);
      println(
          std::format("ATOM  {:5} {:>4} MOL     1    {:8.3f}{:8.3f}{:8.3f}  1.00  0.00    "
                      "      {:>2}  ",
                      index % maxnum, symbol, row(0), row(1), row(2), detail::rjust(symbol, 2)));
    }

    println("ENDMDL");
    result.shrink_to_fit();
    return result;
  }

}  // namespace sqsgen::io

#endif  // SQSGEN_IMPL_IO_STRUCTURE_PDB_IMPL_HPP
