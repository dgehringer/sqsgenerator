

#include <string>

#include "sqsgen/core/helpers/inst.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/structure.h"
#include "sqsgen/types.h"

namespace sqsgen::io {

  namespace core = sqsgen::core;
  namespace helpers = sqsgen::core::helpers;

  template <class T> std::string structure_adapter<T, io::structure_format::pdb>::format(
      const core::structure<T>& structure) {
    auto filtered = structure.without_vacancies();
    std::string result;
    result.reserve(filtered.size() * 144);

    const auto z_to_symbol = [](auto&& z) { return core::atom::from_z(z).symbol; };

    const auto println = [&result](std::string const& line) {
      result.append(helpers::format_string("%s\n", line));
    };

    const auto [a, b, c] = detail::lengths<T>(filtered.lattice);
    const auto [alpha, beta, gamma] = detail::angles<T>(filtered.lattice);
    println(helpers::format_string("CRYST1%9.3f%9.3f%9.3f%7.2f%7.2f%7.2f P 1", a, b, c, alpha, beta,
                                   gamma));

    coords_t<T> cart_coords = filtered.frac_coords * filtered.lattice.transpose();

    std::size_t maxnum = 100000;
    println("MODEL     0");
    for (std::size_t index = 0; index < filtered.size(); ++index) {
      auto row = cart_coords.row(index);
      auto symbol = z_to_symbol(filtered.species[index]);
      println(helpers::format_string(
          "ATOM  %5d %4s MOL     1    %8.3f%8.3f%8.3f  1.00  0.00          %2s  ", index % maxnum,
          symbol, row(0), row(1), row(2), detail::rjust(symbol, 2)));
    }

    println("ENDMDL");
    result.shrink_to_fit();
    return result;
  }

#define INST_STRUCTURE_ADAPTER_PDB(PREC) \
  template struct structure_adapter<PREC, structure_format::pdb>;
  SQSGEN_INSTANTIATE_PREC(INST_STRUCTURE_ADAPTER_PDB)
#undef INST_STRUCTURE_ADAPTER_PDB
}  // namespace sqsgen::io
