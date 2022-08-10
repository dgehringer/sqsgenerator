//
// Created by dominik on 02.06.21.
//
#include "test_helpers.hpp"
#include "types.hpp"
#include "atomistics.hpp"
#include <map>
#include <time.h>
#include <gtest/gtest.h>

using namespace sqsgenerator::utils::atomistics;

namespace sqsgenerator::test {

    Structure from_test_case_data(const TestCaseData& test_case) {
        return Structure(test_case.lattice, test_case.fcoords, Atoms::from_z(test_case.configuration), {true, true, true});
    }

    class AtomisticsTestFixture : public ::testing::Test {
    protected:
        size_t m_num_species;
        std::vector<species_t> m_z;
        
        // primitive fcc-Al structure (3 x 3 x 3) super cell
        std::vector<TestCaseData> m_test_cases;

        AtomisticsTestFixture() :
            m_num_species(114),
            m_z(m_num_species) {
            std::iota(m_z.begin(), m_z.end(), 1);
            m_test_cases = load_test_cases("resources");
            srand ( time(NULL) );
        }

    };

    TEST_F(AtomisticsTestFixture, TestAtomFromZComplete) {
        ASSERT_EQ(Atoms::from_z(1).Z, 1);
        ASSERT_EQ(Atoms::from_z(1).symbol, "H");
        ASSERT_EQ(Atoms::from_z(1).name, "Hydrogen");
        ASSERT_EQ(Atoms::from_z(m_num_species).Z, m_num_species);
        ASSERT_EQ(Atoms::from_z(m_num_species).symbol, "Fl");
        ASSERT_EQ(Atoms::from_z(m_num_species).name, "Flerovium");
        for (const auto& spec: m_z) {
            ASSERT_EQ(Atoms::from_z(spec).Z, spec);
        }
    }

    TEST_F(AtomisticsTestFixture, TestAtomfrom_symbolComplete) {
        ASSERT_EQ(Atoms::from_symbol("H").Z, 1);
        ASSERT_EQ(Atoms::from_symbol("H").symbol, "H");
        ASSERT_EQ(Atoms::from_symbol("H").name, "Hydrogen");
        ASSERT_EQ(Atoms::from_symbol("Fl").Z, m_num_species);
        ASSERT_EQ(Atoms::from_symbol("Fl").symbol, "Fl");
        ASSERT_EQ(Atoms::from_symbol("Fl").name, "Flerovium");
        auto symbols = Atoms::from_z(m_z);
        for (const auto& sym: symbols) {
            ASSERT_EQ(sym.Z, Atoms::from_symbol(sym.symbol).Z);
        }
    }

    TEST_F(AtomisticsTestFixture, TestFromZAndFromSymbols) {
        // batch convert all ordinal numbers
        auto atoms = Atoms::from_z(m_z);
        std::vector<std::string> symbols;
        for (const auto& atom : atoms) symbols.push_back(atom.symbol);

        // batch convert all symbols to atoms
        auto atoms1 = Atoms::from_symbol(symbols);
        ASSERT_EQ(atoms.size(), atoms1.size());

        for (auto i = 0; i < atoms.size(); i++){
            ASSERT_EQ(atoms[i].Z, atoms1[i].Z);
            ASSERT_EQ(atoms[i].symbol, atoms1[i].symbol);
            ASSERT_EQ(atoms[i].electronic_configuration, atoms1[i].electronic_configuration);
            ASSERT_EQ(atoms[i].mass, atoms1[i].mass);
            ASSERT_EQ(atoms[i].atomic_radius, atoms1[i].atomic_radius);
        }
    }

    TEST_F(AtomisticsTestFixture, TestInvalidConstructionStructure){

        auto frac_coords = create_multi_array<double, 1, 3>({ 0.0, 0.0, 0.0});

        // wrong lattice matrix shape
        auto wrong_lattice = create_multi_array<double, 4, 3>({
                 2.473329, 6.995632, 4.283932,
                 2.473329, 6.995632, -4.283932,
                 -4.946659, 6.995632, 0.000000,
                 -4.946659, 6.995632, 0.000000
        });
        ASSERT_THROW(Structure(wrong_lattice, frac_coords, {"Al"}, {true, true, true}), std::invalid_argument);

        // length mismatch in symbols and fractional coords
        auto valid_lattice = create_multi_array<double, 3, 3>({
                2.473329, 6.995632, 4.283932,
                2.473329, 6.995632, -4.283932,
                -4.946659, 6.995632, 0.000000,
        });
        ASSERT_THROW(Structure(valid_lattice, frac_coords, std::vector<std::string>({"Al", "Al"}), {true, true, true}), std::invalid_argument);

        // wrong dimensions of fractional coords
        auto wrong_frac_coords = create_multi_array<double, 1, 2>({ 0.0, 0.0});
        ASSERT_THROW(Structure(valid_lattice, wrong_frac_coords , {"Al"}, {true, true, true}), std::invalid_argument);
    }

    TEST_F(AtomisticsTestFixture, TestConstructionStructure){
        for (const auto& test_case: m_test_cases) {
            auto structure = Structure(test_case.lattice, test_case.fcoords, Atoms::z_to_symbol(test_case.configuration), {true, true, true});
            auto structure1 = Structure(test_case.lattice, test_case.fcoords, Atoms::from_z(test_case.configuration), {true, true, true});
            assert_vector_equals(structure.symbols(), structure1.symbols());
            assert_vector_equals(structure.configuration(), structure1.configuration());
            ASSERT_EQ(structure.num_atoms(), structure1.num_atoms());
        }
    }

    TEST_F(AtomisticsTestFixture, TestStructureLattice){
        for (const auto& test_case: m_test_cases) {
            auto structure = from_test_case_data(test_case);
            assert_multi_array_near(structure.lattice(), test_case.lattice);
        }
    }

    TEST_F(AtomisticsTestFixture, TestStructureFracCoords){
        for (const auto& test_case: m_test_cases) {
            auto structure = from_test_case_data(test_case);
            assert_multi_array_near(structure.frac_coords(), test_case.fcoords);
        }
    }

    TEST_F(AtomisticsTestFixture, TestStructureSpeciesConfigurationSymbols){
        typedef std::vector<std::string> StringVector;
        for (const auto& test_case: m_test_cases) {
            auto structure = from_test_case_data(test_case);
            assert_vector_equals(Atoms::z_to_symbol(test_case.configuration), structure.symbols());
            assert_vector_equals(test_case.configuration, structure.configuration());
        }
    }

    TEST_F(AtomisticsTestFixture, TestStructureDistanceVecsAndMatrixAndShellMatrix) {
        for (const auto &c: m_test_cases) {
            auto structure = from_test_case_data(c);
            auto computed_vecs = structure.distance_vecs();
            auto computed_dists = structure.distance_matrix();
            auto computed_shells = structure.shell_matrix();
            assert_multi_array_near(computed_vecs, c.vecs, std::abs<double>, std::abs<double>);
            assert_multi_array_near(computed_dists, c.distances);
            assert_multi_array_near(computed_shells, c.shells);
        }
    }

    TEST_F(AtomisticsTestFixture, TestStructureWithSpeciesAndRearranged) {

        // we randomly generate a sequence of new occupations for each test case
        configuration_t choice_species = {13, 17, 25, 26};
        // we sort them by their ordinal number
        std::sort(choice_species.begin(), choice_species.end());

        for (const auto &c: m_test_cases) {
            auto structure = from_test_case_data(c);
            configuration_t new_species(structure.num_atoms());
            std::map<species_t, size_t> histogram;
            // create a random equence of occupations containing the atoms defined in choice_species
            // create a histogram in parallel
            for (auto i = 0; i < structure.num_atoms(); i++) {
                species_t species = choice_species[rand() % choice_species.size()];
                new_species[i] = species;
                if (! histogram.count(species)) {
                    histogram[species] = 1;
                } else {
                    histogram[species]++;
                }
            }

            auto new_structure = structure.with_species(new_species);
            ASSERT_EQ(new_structure.num_atoms(), structure.num_atoms());
            assert_vector_equals(new_structure.configuration(), new_species);


            auto indices = sqsgenerator::utils::argsort(new_species);

            auto rearranged_structure = new_structure.rearranged(indices);
            auto rearranged_configuration= configuration_t(rearranged_structure.num_atoms());

            auto cnt = 0;
            for (const auto& sp: choice_species) {
                for (auto i = 0; i < histogram[sp]; i++) {
                    rearranged_configuration[cnt] = sp;
                    cnt++;
                }
            }
            ASSERT_EQ(rearranged_structure.num_atoms(), structure.num_atoms());
            assert_vector_equals(rearranged_configuration, rearranged_structure.configuration());
        }
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}