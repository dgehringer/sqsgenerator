//
// Created by Dominik Gehringer on 23.11.24.
//

#ifndef SQSGEN_CONFIGURATION_COMPOSITION_H
#define SQSGEN_CONFIGURATION_COMPOSITION_H

#include <variant>
#include <vector>

#include "sqsgen/core/sorted_vector.h"
#include "sqsgen/types.h"

namespace sqsgen::configuration {

  namespace ranges = std::ranges;
  namespace core = sqsgen::core;

  using indices_t = core::sorted_vector<std::size_t>;

  using site_spec = std::variant<std::vector<int>, std::vector<std::string>, std::string, int>;
  using composition_spec = std::map<std::string, site_spec>;
  using composition_input = std::variant<std::vector<composition_spec>, composition_spec>;

  template <sublattice_mode, class R>
  result_t<R> validate_composition_input(const composition_input&, const configuration_t& conf);

  /*
    namespace detail {


      template <string_literal key, class Document>
      parse_result<std::string> parse_symbol_string(auto value) {
        if constexpr (std::is_same_v<std::decay_t<Document>, nlohmann::json>) return value;


        return parse_error::from_msg<key, CODE_TYPE_ERROR>("Unknown document type");
      }

      template <string_literal key, class Document>
      parse_result<composition_t> parse_distribution(Document const& document, index_set_t&& sites)
    { composition_t composition; using pair_t = std::pair<std::string, int>; for (auto& [k, v] :
    accessor<Document>::items(document)) { auto rkey = parse_symbol_string<key, Document>(k); if
    (rkey.ok() && !core::SYMBOL_MAP.contains(rkey.result())) continue; auto r =
    rkey.combine(accessor<Document>::template get_as<KEY_NONE, int>(v)) .and_then([&](auto s_and_a)
    -> parse_result<pair_t> { auto [specie, amount] = s_and_a; if (amount > sites.size() || amount <
    0) return parse_error::from_msg<key, CODE_BAD_VALUE>(format_string( "You want to distribute %i
    \"%s\" atoms on a sublattice with %i sites", amount, specie, sites.size())); return
    pair_t{specie, amount};
                       });
          if (r.failed()) return r.error().with_key(key.data);
          auto [specie, amount] = r.result();
          composition[core::atom::from_symbol(specie).Z] = amount;
        }
        if (composition.empty())
          return parse_error::from_msg<key, CODE_OUT_OF_RANGE>(
              "You have not distributed any valid species on the (sub)lattice");
        return composition;
      }

      template <string_literal key, string_literal sitesKey = "sites", class Document>
      parse_result<sublattice> parse_sublattice(Document const& j, configuration_t const& conf,
                                                std::vector<sublattice>&& sublattices) {
        if (!accessor<Document>::is_document(j))
          return parse_error::from_msg<key, CODE_BAD_VALUE>("the JSON value is not an object");
        index_set_t occupied_sites = fold_left(sublattices, index_set_t{}, [](auto&& occ, auto&& sl)
    { occ.merge(sl.sites); return occ;
        });
        index_set_t remaining_indices(
            range(conf.size()) | views::filter([&](auto i) { return !occupied_sites.contains(i);
    })); if (remaining_indices.empty()) return parse_error::from_msg<sitesKey, CODE_OUT_OF_RANGE>(
              "There are no remaining species left for the sublattice");

        return parse_sites<sitesKey>(j, conf, std::move(remaining_indices))
            .and_then([&](auto&& sites) -> parse_result<index_set_t> {
              for (auto&& site : sites)
                if (occupied_sites.contains(site))
                  return parse_error::from_msg<"sites", CODE_BAD_VALUE>(format_string(
                      "The site with index %i is contained in more than one sublattice. "
                      "Make sure that the "
                      "\"sites\" argument does not contain overlapping index ranges",
                      site));
              return sites;
            })
            .and_then([&](auto&& sites) -> parse_result<sublattice> {
              // parse composition
              auto composition = parse_distribution<key>(j, std::forward<decltype(sites)>(sites));
              if (composition.failed()) return composition.error();
              auto num_sites
                  = fold_left(composition.result() | views::elements<1>, 0UL, std::plus<>{});
              if (num_sites != sites.size())
                return parse_error::from_msg<sitesKey, CODE_OUT_OF_RANGE>(
                    format_string("The total number of distributed atoms is %i but the sublattice "
                                  "contains %i sites",
                                  num_sites, sites.size()));
              return sublattice{sites, composition.result()};
            });
      }
    }  // namespace detail

    template <string_literal key, string_literal sitesKey = "sites", class Document>
    parse_result<std::vector<sublattice>> parse_composition(Document const& document,
                                                            configuration_t const& conf,
                                                            SublatticeMode sl_mode) {
      using namespace core::helpers;
      if (!accessor<Document>::contains(document, key.data))
        return parse_error::from_msg<key, CODE_NOT_FOUND>("You need to specify a composition");
      const auto doc = accessor<Document>::get(document, key.data);
      auto split_error = parse_error::from_msg<key, CODE_BAD_VALUE>(
          "You have specified \"split\" mode but only specified one sublattice");
      if (accessor<std::decay_t<decltype(doc)>>::is_list(doc)) {
        std::vector<sublattice> sublattices;
        for (auto const& d : doc) {
          auto sl = detail::parse_sublattice<key, sitesKey>(
              d, conf, std::forward<std::vector<sublattice>>(sublattices));
          if (sl.failed()) return sl.error();
          sublattices.push_back(sl.result());
        }
        if (sublattices.empty())
          return parse_error::from_msg<key, CODE_OUT_OF_RANGE>("Could not parse a valid
    sublattice");
        // if (sublattices.size() < 2 && sl_mode == SUBLATTICE_MODE_SPLIT) return split_error;
        return {sublattices};
      }
      if (sl_mode == SUBLATTICE_MODE_SPLIT) return split_error;
      return detail::parse_sublattice<key, sitesKey>(doc, conf, {})
          .and_then(
              [&](auto&& sl) -> parse_result<std::vector<sublattice>> { return std::vector{sl}; });
              }*/

}  // namespace sqsgen::configuration
#endif  // SQSGEN_CONFIGURATION_COMPOSITION_H
