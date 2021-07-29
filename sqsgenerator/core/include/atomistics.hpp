/**
 * @file atomistics.hpp
 */

#ifndef SQSGENERATOR_ATOMISTICS_HPP
#define SQSGENERATOR_ATOMISTICS_HPP

#include "types.hpp"
#include "structure_utils.hpp"
#include <map>
#include <queue>
#include <vector>
#include <string>
#include <boost/multi_array.hpp>

using namespace boost;

namespace sqsgenerator::utils::atomistics {

    /**
     * Data class which holds some information of the periodic table. To construct instances, the static utility
     * functions of `Atoms`.
     */
    class Atom {
    public:
        const species_t Z; /**< The ordinal number of the element in the periodic table */
        const std::string name; /**< The full english name of the element. E. g. "Iron" */
        const std::string symbol; /**< The two character symbol of an element. E.g "Fe" for iron */
        const std::string electronic_configuration; /**< The electronic configuration as a string */
        const double atomic_radius; /**< Atomic radius in pm */
        const double mass; /**< The mass of the atoms in atomic units */
        const double en; /**< Electronegativity */
    };

    /**
     * Utility class, exposing public constructors for the `Atom` class
     * @see Atom
     */
    class Atoms {

    private:
        const static std::vector<Atom> m_elements; /** < the list of elements available */
        static std::map<std::string , species_t> make_symbol_map() {
            static std::map<std::string, species_t> symbol_map;
            for( const auto &atom : Atoms::m_elements)  symbol_map.emplace(atom.symbol, atom.Z);
            return symbol_map;
        }

        static std::map<std::string , species_t> m_symbol_map;

    public:
        /**
         * resolves the ordinal number of an atomic element and looks up the Atom object
         * @param Z the ordinal number in the periodic table
         * @return the atom object
         */
        static Atom from_z(species_t Z);

        /***
         * Resolves a vector of ordinal number into a list of Atoms objects
         * @param numbers vector of ordinal numbers
         * @return vector of Atom objects
         */
        static std::vector<Atom> from_z(const std::vector<species_t> &numbers);

        /**
         * resolves the symbol of an atomic element e. g. "Fe" and looks up the Atom object. The input argument is
         * case sensitive
         * @param symbol a string with the species symbol
         * @return the Atom object
         */
        static Atom from_symbol(const std::string &symbol);

        /**
         * Resolves a vector of species symbols into a list of Atoms objects
         * @param symbols vector of symbols
         * @return vector of Atom objects
         */
        static std::vector<Atom> from_symbol(const std::vector<std::string> &symbols);
    };

    class Structure {
    private:
        size_t m_natoms;
        array_2d_t m_lattice;
        array_2d_t m_frac_coords;
        array_2d_t m_distance_matrix;
        array_3d_t m_pbc_vecs;
        std::array<bool, 3> m_pbc;
        std::vector<Atom> m_species;

    public:
        Structure(array_2d_t lattice, array_2d_t frac_coords, std::vector<Atom> species, std::array<bool, 3> pbc);
        Structure(array_2d_t lattice, array_2d_t frac_coords, std::vector<std::string> species, std::array<bool, 3> pbc);
        Structure(array_2d_t lattice, array_2d_t frac_coords, std::vector<species_t> species, std::array<bool, 3> pbc);

        [[nodiscard]] size_t num_atoms() const;
        [[nodiscard]] const_array_2d_ref_t lattice() const;
        [[nodiscard]] const_array_2d_ref_t frac_coords() const;
        [[nodiscard]] std::array<bool, 3> pbc() const;
        [[nodiscard]] const std::vector<Atom>& species() const;
        [[nodiscard]] const_array_3d_ref_t distance_vecs() const;
        [[nodiscard]] const_array_2d_ref_t distance_matrix() const;
        [[nodiscard]] configuration_t configuration() const;
        [[nodiscard]] std::vector<std::string> symbols() const;
        [[nodiscard]] pair_shell_matrix_t shell_matrix(double atol=1.0e-5, double rtol=1.0e-8) const;
        [[nodiscard]] pair_shell_matrix_t shell_matrix(const std::vector<double> &shell_distances, double atol=1.0e-5, double rtol=1.0e-8) const;

        static std::vector<AtomPair> create_pair_list(pair_shell_matrix_t shell_matrix, const std::map<shell_t, double> &weights) {
            return sqsgenerator::utils::create_pair_list(shell_matrix, weights);
        }


    };
}

#endif //SQSGENERATOR_ATOMISTICS_HPP
