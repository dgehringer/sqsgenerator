

#include "sqsgen/core/result.h"

#include "sqsgen/types.h"

namespace sqsgen::core {
  template <class T> struct sqs_result<T, SUBLATTICE_MODE_INTERACT> {
    T objective;
    configuration_t species;
    cube_t<T> sro;

    template <typename H> friend H AbslHashValue(H h, const sqs_result& c) {
      return H::combine(std::move(h), c.species);
    }

    sqs_result() = default;

    // Explicit copy constructor
    sqs_result(const sqs_result& other)
        : objective(other.objective),
          species(other.species),
          sro(other.sro) {}  // Eigen::Tensor should handle this properly

    // Explicit move constructor
    sqs_result(sqs_result&& other) noexcept
        : objective(std::move(other.objective)),
          species(std::move(other.species)),
          sro(std::move(other.sro)) {}  // Eigen::Tensor should handle this properly

    // Assignment operators
    sqs_result& operator=(const sqs_result& other) {
      if (this != &other) {
        objective = other.objective;
        species = other.species;
        sro = other.sro;
      }
      return *this;
    }

    sqs_result& operator=(sqs_result&& other) noexcept {
      if (this != &other) {
        objective = std::move(other.objective);
        species = std::move(other.species);
        sro = std::move(other.sro);
      }
      return *this;
    }

    bool operator==(sqs_result const& other) const {
      return objective == other.objective && species == other.species;
    }
  };

  template <class T> struct sqs_result<T, SUBLATTICE_MODE_SPLIT> {
    T objective;
    std::vector<sqs_result<T, SUBLATTICE_MODE_INTERACT>> sublattices;

    template <typename H> friend H AbslHashValue(H h, const sqs_result& c) {
      H hash_state = std::move(h);
      for (auto const& sublattice : c.sublattices)
        hash_state = H::combine(std::move(hash_state), sublattice);
      return hash_state;
    }

    sqs_result() = default;

    sqs_result(T objective, std::vector<sqs_result<T, SUBLATTICE_MODE_INTERACT>> const& sublattices)
        : objective(objective), sublattices(sublattices) {}

    sqs_result(T objective, std::vector<T> const& objectives,
               const std::vector<configuration_t>& species, std::vector<cube_t<T>> const& sro)
        : objective(objective) {
      if (objectives.size() != species.size() || objectives.size() != sro.size())
        throw std::invalid_argument("invalid number entries");
      sublattices.reserve(objectives.size());
      for (auto i = 0; i < objectives.size(); ++i)
        sublattices.push_back({objectives[i], species[i], sro[i]});
    }

    bool operator==(sqs_result const& other) const {
      return objective == other.objective && sublattices == other.sublattices;
    }
  };

  template <> struct sqs_result<double, SUBLATTICE_MODE_SPLIT>;
  template <> struct sqs_result<float, SUBLATTICE_MODE_SPLIT>;
  template <> struct sqs_result<double, SUBLATTICE_MODE_INTERACT>;
  template <> struct sqs_result<float, SUBLATTICE_MODE_INTERACT>;
}  // namespace sqsgen::core
