//
// Created by Dominik Gehringer on 28.02.25.
//

#ifndef SQSGEN_IO_MPI_CONFIG_H
#define SQSGEN_IO_MPI_CONFIG_H

#ifdef WITH_MPI
#  include <mpl/mpl.hpp>
#endif

namespace sqsgen::io::mpi {
#ifdef WITH_MPI
  static constexpr bool HAVE_MPI = true;
#else
  static constexpr bool HAVE_MPI = false;
#endif

  static constexpr int RANK_HEAD = 0;
}  // namespace sqsgen::io::mpi

#endif  // SQSGEN_IO_MPI_CONFIG_H
