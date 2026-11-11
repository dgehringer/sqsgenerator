#ifndef SQSGEN_IMPL_CONFIGURATION_SHARED_IMPL_HPP
#define SQSGEN_IMPL_CONFIGURATION_SHARED_IMPL_HPP

#include <optional>
#include <string_view>
#include <variant>

#include "sqsgen/core/atom.h"
#include "sqsgen/core/helpers/variant.h"
#include "sqsgen/types.h"
namespace sqsgen::configuration {

  namespace helpers = sqsgen::core::helpers;

  namespace detail {

    inline result_t<specie_t> validate_species(std::string_view key,
                                               const std::variant<int, std::string>& input,
                                               std::optional<std::string_view> where
                                               = std::nullopt) {
      return std::visit(
          helpers::overloaded{
              [&](int z) -> result_t<specie_t> {
                const auto atom = core::atom::from_z(z);
                if (!atom)
                  return std::unexpected(configuration_error{
                      std::string(key), error_code::bad_argument,
                      std::format("{}unknown ordinal {}", where.value_or(""), z)});
                else
                  return atom.value().Z;
              },
              [&](const std::string& symbol) -> result_t<specie_t> {
                const auto atom = core::atom::from_symbol(symbol);
                if (!atom)
                  return std::unexpected(configuration_error{
                      std::string(key), error_code::bad_argument,
                      std::format("{}unknown symbol {}", where.value_or(""), symbol)});
                else
                  return atom.value().Z;
              }},
          input);
    }
  }  // namespace detail
}  // namespace sqsgen::configuration

#endif
