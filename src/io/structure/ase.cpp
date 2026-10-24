
#include <glaze/glaze.hpp>
#include <glaze/json/generic_fwd.hpp>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "sqsgen/core/helpers/inst.h"
#include "sqsgen/core/structure.h"
#include "sqsgen/io/glaze.h"
#include "sqsgen/io/structure.h"

namespace sqsgen::io {

  namespace detail {

    template <class T> using ndarray_payload
        = std::tuple<std::vector<std::size_t>, std::string, std::vector<T>>;

    template <class T> struct ndarray {
      ndarray_payload<T> __ndarray__;
    };

    template <class T> struct ase_array {
      ndarray<T> array;
      std::optional<std::string> __ase_objtype__{};
    };

    template <class T> struct ase_structure {
      ase_array<T> cell;
      std::string user{"sqsgen"};
      double mtime{};
      double ctime{};
      ase_array<T> pbc;
      ase_array<T> positions;
    };

#define INST_ASE_STRUCTURE(PREC) template struct ase_structure<PREC>;
    SQSGEN_INSTANTIATE_PREC(INST_ASE_STRUCTURE)
#undef INST_ASE_STRUCTURE
  }  // namespace detail

  template <class T, class Array> detail::ndarray<T> make_ndarray(std::string dtype,
                                                                  std::vector<std::size_t> shape,
                                                                  const Array& array) {
    return detail::ndarray<T>{detail::ndarray_payload<T>{
        std::move(shape),
        std::move(dtype),
        std::vector<T>(array.data(), array.data() + array.size()),
    }};
  }

  template <class T, class Pbc> detail::ndarray<T> make_pbc_ndarray(const Pbc& pbc) {
    std::vector<T> data;
    data.reserve(std::size(pbc));
    for (auto b : pbc) data.push_back(static_cast<T>(b));
    return detail::ndarray<T>{
        detail::ndarray_payload<T>{{std::size(pbc)}, "bool", std::move(data)}};
  }

  double years_since_2000() {
    // January 1, 2000 00:00:00 UTC
    std::tm timeinfo = {};
    timeinfo.tm_year = 100;  // Year since 1900
    timeinfo.tm_mon = 0;     // Month [0, 11]
    timeinfo.tm_mday = 1;    // Day of the month [1, 31]

    // Convert std::tm to time_t
    std::time_t time_t_epoch = std::mktime(&timeinfo);

    // Convert time_t to std::chrono::system_clock::time_point
    auto time_point_2000 = std::chrono::system_clock::from_time_t(time_t_epoch);
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - time_point_2000);
    double seconds_per_year = 31557600.0;  // Average number of seconds per year
    return static_cast<double>(duration.count()) / seconds_per_year;
  }

  template <class T> detail::ase_structure<T> to_ase(core::structure<T> const& structure) {
    auto filtered = structure.without_vacancies();
    const auto n = static_cast<std::size_t>(filtered.size());

    // positions = frac_coords * lattice, per row (your existing math, unchanged)
    matrix_t<T> positions(filtered.size(), 3);
    for (Eigen::Index i = 0; i < filtered.size(); ++i)
      positions.row(i) = filtered.frac_coords.row(i) * filtered.lattice;

    detail::ase_structure<T> out;
    out.cell = detail::ase_array<T>{
        .array = make_ndarray<T>("float64", {3, 3}, filtered.lattice),
        .__ase_objtype__ = "cell",
    };
    out.pbc = detail::ase_array<T>{
        .array = make_pbc_ndarray<T>(filtered.pbc),
    };
    out.positions = detail::ase_array<T>{
        .array = make_ndarray<T>("float64", {n, std::size_t{3}}, positions),
    };
    out.mtime = years_since_2000();
    out.ctime = out.mtime;  // was computed twice; same value
    return out;
  }

  template <class T> std::string structure_adapter<T, structure_format::ase>::format(
      const core::structure<T>& structure, wire_format format) {
    auto data = write_any(to_ase<T>(structure), format);
    if (!data) throw std::runtime_error{"failed to serialize strucure"};
    return std::move(*data);
  }

#define INST_STRUCTURE_ADAPTER_ASE(PREC) \
  template struct structure_adapter<PREC, structure_format::ase>;
  SQSGEN_INSTANTIATE_PREC(INST_STRUCTURE_ADAPTER_ASE)
#undef INST_STRUCTURE_ADAPTER_ASE

}  // namespace sqsgen::io

namespace glz {
  template <class T> struct meta<sqsgen::io::detail::ndarray<T>> {
    using N = sqsgen::io::detail::ndarray<T>;
    static constexpr auto value = glz::object("__ndarray__", &N::__ndarray__);
  };

#define INST_DETAIL_NDARRAY(PREC) template struct meta<sqsgen::io::detail::ndarray<PREC>>;
  SQSGEN_INSTANTIATE_PREC(INST_DETAIL_NDARRAY)
#undef INST_DETAIL_NDARRAY

}  // namespace glz
