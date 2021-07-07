//
// Created by dominik on 02.06.21.
//

#ifndef SQSGENERATOR_ATOMISTICS_HPP
#define SQSGENERATOR_ATOMISTICS_HPP
#include "utils.hpp"
#include "types.hpp"
#include <map>
#include <vector>
#include <string>
#include <boost/multi_array.hpp>

using namespace boost;

namespace sqsgenerator::utils::atomistics {

    class Atom {
    public:
        const Species Z;
        const std::string name;
        const std::string symbol;
        const std::string electronic_configuration;
        const double atomic_radius;
        const double mass;
        const double en;
    };

    class Atoms {

    private:
        const static std::vector<Atom> m_elements;


        static std::map<std::string , Species> make_symbol_map() {
            static std::map<std::string, Species> symbol_map;
            for( const auto &atom : Atoms::m_elements)  symbol_map.emplace(atom.symbol, atom.Z);
            return symbol_map;
        }

        static std::map<std::string , Species> m_symbolMap;

    public:
        //TODO: Ask how to make m_elements private while be able to initialize m_symbolMap;

        static Atom from_z(Species Z);
        static Atom from_symbol(const std::string &symbol);
        static std::vector<Atom> from_z(const std::vector<Species> &numbers);
        static std::vector<Atom> from_symbol(const std::vector<std::string> &symbols);
    };

    class Structure {
    private:
        uint8_t m_prec;
        array_2d_t m_lattice;
        array_2d_t m_frac_coords;
        array_2d_t m_distance_matrix;
        array_3d_t m_pbc_vecs;
        std::array<bool, 3> m_pbc;
        std::vector<Atom> m_species;
        pair_shell_matrix m_shell_matrix;

    public:
        Structure(array_2d_t lattice, array_2d_t frac_coords, std::vector<Atom> species, std::array<bool, 3> pbc);
        Structure(array_2d_t lattice, array_2d_t frac_coords, std::vector<std::string> species, std::array<bool, 3> pbc);
        Structure(array_2d_t lattice, array_2d_t frac_coords, std::vector<Species> species, std::array<bool, 3> pbc);

        const_array_2d_ref_t lattice() const;
        const_array_2d_ref_t frac_coords() const;
        std::array<bool, 3> pbc() const;
        const std::vector<Atom>& species() const;
        const_array_3d_ref_t distance_vecs() const;
        const_array_2d_ref_t distance_matrix() const;
        const_pair_shell_matrix_ref shell_matrix(uint8_t prec);
    };
}


#endif //SQSGENERATOR_ATOMISTICS_HPP
