//
// Created by dominik on 02.06.21.
//

#include "types.hpp"
#include "atomistics.hpp"
#include <gtest/gtest.h>

using namespace sqsgenerator::utils::atomistics;

namespace sqsgenerator::test {

    class AtomisticsTestFixture : public ::testing::Test {
    protected:
        size_t m_numSpecies;
        std::vector<Species> m_zNumbers;

        AtomisticsTestFixture() : m_numSpecies(114), m_zNumbers(m_numSpecies) {
            std::iota(m_zNumbers.begin(), m_zNumbers.end(), 1);
        }



    };

    void vectorCompare(const std::vector<Atom> &lhs, const std::vector<Atom> &rhs) {
        ASSERT_EQ(lhs.size(), rhs.size()) << "Vectors x and y are of unequal length";

        for (int i = 0; i < lhs.size(); ++i) {
            ASSERT_EQ(lhs[i].Z, rhs[i].Z) << "Vectors x and y differ at index " << i;
        }
    }

    TEST_F(AtomisticsTestFixture, TestAtomFromZComplete) {
        ASSERT_EQ(Atoms::from_z(1).Z, 1);
        ASSERT_EQ(Atoms::from_z(1).symbol, "H");
        ASSERT_EQ(Atoms::from_z(1).name, "Hydrogen");
        ASSERT_EQ(Atoms::from_z(m_numSpecies).Z, m_numSpecies);
        ASSERT_EQ(Atoms::from_z(m_numSpecies).symbol, "Fl");
        ASSERT_EQ(Atoms::from_z(m_numSpecies).name, "Flerovium");
        for (const auto& spec: m_zNumbers) {
            ASSERT_EQ(Atoms::from_z(spec).Z, spec);
        }
    }

    TEST_F(AtomisticsTestFixture, TestAtomfrom_symbolComplete) {
        ASSERT_EQ(Atoms::from_symbol("H").Z, 1);
        ASSERT_EQ(Atoms::from_symbol("H").symbol, "H");
        ASSERT_EQ(Atoms::from_symbol("H").name, "Hydrogen");
        ASSERT_EQ(Atoms::from_symbol("Fl").Z, m_numSpecies);
        ASSERT_EQ(Atoms::from_symbol("Fl").symbol, "Fl");
        ASSERT_EQ(Atoms::from_symbol("Fl").name, "Flerovium");
        auto symbols = Atoms::from_z(m_zNumbers);
        for (const auto& sym: symbols) {
            ASSERT_EQ(sym.Z, Atoms::from_symbol(sym.symbol).Z);
        }
    }

    TEST_F(AtomisticsTestFixture, TestFromZAndfrom_symbols) {
        auto atoms = Atoms::from_z(m_zNumbers);
        std::vector<std::string> symbols;
        for (const auto& atom : atoms) symbols.push_back(atom.symbol);
        std::vector<std::string> empty;
        ASSERT_EQ(empty, symbols);
        //vectorCompare(atoms, Atoms::from_symbol(symbols));
    }




}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}