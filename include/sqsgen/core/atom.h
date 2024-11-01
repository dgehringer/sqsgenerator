//
// Created by Dominik Gehringer on 22.10.24.
//

#ifndef ATOM_H
#define ATOM_H
#include <string>

#include "sqsgen/types.h"

namespace sqsgen::core {

  namespace detail {
    using atom_record
        = std::tuple<specie_t, std::string, std::string, std::string, double, double, double>;
  }

  static const auto KNOWN_ELEMENTS = std::array{
      detail::atom_record{0, "Vacancy", "0", "", 0.0, 0.0, 0.0},
      detail::atom_record{1, "Hydrogen", "H", "1s1", 53.0000, 1.0080, 2.2000},
      detail::atom_record{2, "Helium", "He", "1s2", 31.0000, 4.0026, 0.0000},
      detail::atom_record{3, "Lithium", "Li", "[He] 2s1", 167.0000, 6.9400, 0.9800},
      detail::atom_record{4, "Beryllium", "Be", "[He] 2s2", 112.0000, 9.0122, 1.5700},
      detail::atom_record{5, "Boron", "B", "[He] 2s2 2p1", 87.0000, 10.8100, 2.0400},
      detail::atom_record{6, "Carbon", "C", "[He] 2s2 2p2", 67.0000, 12.0110, 2.5500},
      detail::atom_record{7, "Nitrogen", "N", "[He] 2s2 2p3", 56.0000, 14.0070, 3.0400},
      detail::atom_record{8, "Oxygen", "O", "[He] 2s2 2p4", 48.0000, 15.9990, 3.4400},
      detail::atom_record{9, "Fluorine", "F", "[He] 2s2 2p5", 42.0000, 18.9984, 3.9800},
      detail::atom_record{10, "Neon", "Ne", "[He] 2s2 2p6", 38.0000, 20.1797, 0.0000},
      detail::atom_record{11, "Sodium", "Na", "[Ne] 3s1", 190.0000, 22.9898, 0.9300},
      detail::atom_record{12, "Magnesium", "Mg", "[Ne] 3s2", 145.0000, 24.3050, 1.3100},
      detail::atom_record{13, "Aluminium", "Al", "[Ne] 3s2 3p1", 118.0000, 26.9815, 1.6100},
      detail::atom_record{14, "Silicon", "Si", "[Ne] 3s2 3p2", 111.0000, 28.0850, 1.9000},
      detail::atom_record{15, "Phosphorus", "P", "[Ne] 3s2 3p3", 98.0000, 30.9738, 2.1900},
      detail::atom_record{16, "Sulfur", "S", "[Ne] 3s2 3p4", 88.0000, 32.0600, 2.5800},
      detail::atom_record{17, "Chlorine", "Cl", "[Ne] 3s2 3p5", 79.0000, 35.4500, 3.1600},
      detail::atom_record{18, "Argon", "Ar", "[Ne] 3s2 3p6", 71.0000, 39.9480, 0.0000},
      detail::atom_record{19, "Potassium", "K", "[Ar] 4s1", 243.0000, 39.0983, 0.8200},
      detail::atom_record{20, "Calcium", "Ca", "[Ar] 4s2", 194.0000, 40.0780, 1.0000},
      detail::atom_record{21, "Scandium", "Sc", "[Ar] 3d1 4s2", 184.0000, 44.9559, 1.3600},
      detail::atom_record{22, "Titanium", "Ti", "[Ar] 3d2 4s2", 176.0000, 47.8670, 1.5400},
      detail::atom_record{23, "Vanadium", "V", "[Ar] 3d3 4s2", 171.0000, 50.9415, 1.6300},
      detail::atom_record{24, "Chromium", "Cr", "[Ar] 3d5 4s1", 166.0000, 51.9961, 1.6600},
      detail::atom_record{25, "Manganese", "Mn", "[Ar] 3d5 4s2", 161.0000, 54.9380, 1.5500},
      detail::atom_record{26, "Iron", "Fe", "[Ar] 3d6 4s2", 156.0000, 55.8450, 1.8300},
      detail::atom_record{27, "Cobalt", "Co", "[Ar] 3d7 4s2", 152.0000, 58.9332, 1.8800},
      detail::atom_record{28, "Nickel", "Ni", "[Ar] 3d8 4s2", 149.0000, 58.6934, 1.9100},
      detail::atom_record{29, "Copper", "Cu", "[Ar] 3d10 4s1", 145.0000, 63.5460, 1.9000},
      detail::atom_record{30, "Zinc", "Zn", "[Ar] 3d10 4s2", 142.0000, 65.3800, 1.6500},
      detail::atom_record{31, "Gallium", "Ga", "[Ar] 3d10 4s2 4p1", 136.0000, 69.7230, 1.8100},
      detail::atom_record{32, "Germanium", "Ge", "[Ar] 3d10 4s2 4p2", 125.0000, 72.6300, 2.0100},
      detail::atom_record{33, "Arsenic", "As", "[Ar] 3d10 4s2 4p3", 114.0000, 74.9216, 2.1800},
      detail::atom_record{34, "Selenium", "Se", "[Ar] 3d10 4s2 4p4", 103.0000, 78.9600, 2.5500},
      detail::atom_record{35, "Bromine", "Br", "[Ar] 3d10 4s2 4p5", 94.0000, 79.9040, 2.9600},
      detail::atom_record{36, "Krypton", "Kr", "[Ar] 3d10 4s2 4p6", 88.0000, 83.7980, 3.0000},
      detail::atom_record{37, "Rubidium", "Rb", "[Kr] 5s1", 265.0000, 85.4678, 0.8200},
      detail::atom_record{38, "Strontium", "Sr", "[Kr] 5s2", 219.0000, 87.6200, 0.9500},
      detail::atom_record{39, "Yttrium", "Y", "[Kr] 4d1 5s2", 212.0000, 88.9059, 1.2200},
      detail::atom_record{40, "Zirconium", "Zr", "[Kr] 4d2 5s2", 206.0000, 91.2240, 1.3300},
      detail::atom_record{41, "Niobium", "Nb", "[Kr] 4d4 5s1", 198.0000, 92.9064, 1.6000},
      detail::atom_record{42, "Molybdenum", "Mo", "[Kr] 4d5 5s1", 190.0000, 95.9600, 2.1600},
      detail::atom_record{43, "Technetium", "Tc", "[Kr] 4d5 5s2", 183.0000, 98.0000, 1.9000},
      detail::atom_record{44, "Ruthenium", "Ru", "[Kr] 4d7 5s1", 178.0000, 101.0700, 2.2000},
      detail::atom_record{45, "Rhodium", "Rh", "[Kr] 4d8 5s1", 173.0000, 102.9055, 2.2800},
      detail::atom_record{46, "Palladium", "Pd", "[Kr] 4d10", 169.0000, 106.4200, 2.2000},
      detail::atom_record{47, "Silver", "Ag", "[Kr] 4d10 5s1", 165.0000, 107.8682, 1.9300},
      detail::atom_record{48, "Cadmium", "Cd", "[Kr] 4d10 5s2", 161.0000, 112.4110, 1.6900},
      detail::atom_record{49, "Indium", "In", "[Kr] 4d10 5s2 5p1", 156.0000, 114.8180, 1.7800},
      detail::atom_record{50, "Tin", "Sn", "[Kr] 4d10 5s2 5p2", 145.0000, 118.7100, 1.9600},
      detail::atom_record{51, "Antimony", "Sb", "[Kr] 4d10 5s2 5p3", 133.0000, 121.7600, 2.0500},
      detail::atom_record{52, "Tellurium", "Te", "[Kr] 4d10 5s2 5p4", 123.0000, 127.6000, 2.1000},
      detail::atom_record{53, "Iodine", "I", "[Kr] 4d10 5s2 5p5", 115.0000, 126.9045, 2.6600},
      detail::atom_record{54, "Xenon", "Xe", "[Kr] 4d10 5s2 5p6", 108.0000, 131.2930, 2.6000},
      detail::atom_record{55, "Caesium", "Cs", "[Xe] 6s1", 298.0000, 132.9055, 0.7900},
      detail::atom_record{56, "Barium", "Ba", "[Xe] 6s2", 253.0000, 137.3270, 0.8900},
      detail::atom_record{57, "Lanthanum", "La", "[Xe] 5d1 6s2", 169.0000, 138.9055, 1.1000},
      detail::atom_record{58, "Cerium", "Ce", "[Xe] 4f1 5d1 6s2", 131.0000, 140.9076, 1.1200},
      detail::atom_record{59, "Praseodymium", "Pr", "[Xe] 4f3 6s2", 247.0000, 140.9076, 1.1300},
      detail::atom_record{60, "Neodymium", "Nd", "[Xe] 4f4 6s2", 206.0000, 144.2420, 1.1400},
      detail::atom_record{61, "Promethium", "Pm", "[Xe] 4f5 6s2", 205.0000, 145.0000, 0.0000},
      detail::atom_record{62, "Samarium", "Sm", "[Xe] 4f6 6s2", 238.0000, 150.3600, 1.1700},
      detail::atom_record{63, "Europium", "Eu", "[Xe] 4f7 6s2", 231.0000, 151.9640, 0.0000},
      detail::atom_record{64, "Gadolinium", "Gd", "[Xe] 4f7 5d1 6s2", 233.0000, 157.2500, 1.2000},
      detail::atom_record{65, "Terbium", "Tb", "[Xe] 4f9 6s2", 225.0000, 158.9254, 0.0000},
      detail::atom_record{66, "Dysprosium", "Dy", "[Xe] 4f10 6s2", 228.0000, 162.5000, 1.2200},
      detail::atom_record{67, "Holmium", "Ho", "[Xe] 4f11 6s2", 0.0000, 164.9303, 1.2300},
      detail::atom_record{68, "Erbium", "Er", "[Xe] 4f12 6s2", 226.0000, 167.2590, 1.2400},
      detail::atom_record{69, "Thulium", "Tm", "[Xe] 4f13 6s2", 222.0000, 168.9342, 1.2500},
      detail::atom_record{70, "Ytterbium", "Yb", "[Xe] 4f14 6s2", 222.0000, 173.0540, 0.0000},
      detail::atom_record{71, "Lutetium", "Lu", "[Xe] 4f14 5d1 6s2", 217.0000, 174.9668, 1.2700},
      detail::atom_record{72, "Hafnium", "Hf", "[Xe] 4f14 5d2 6s2", 208.0000, 178.4900, 1.3000},
      detail::atom_record{73, "Tantalum", "Ta", "[Xe] 4f14 5d3 6s2", 200.0000, 180.9479, 1.5000},
      detail::atom_record{74, "Tungsten", "W", "[Xe] 4f14 5d4 6s2", 193.0000, 183.8400, 2.3600},
      detail::atom_record{75, "Rhenium", "Re", "[Xe] 4f14 5d5 6s2", 188.0000, 186.2070, 1.9000},
      detail::atom_record{76, "Osmium", "Os", "[Xe] 4f14 5d6 6s2", 185.0000, 190.2300, 2.2000},
      detail::atom_record{77, "Iridium", "Ir", "[Xe] 4f14 5d7 6s2", 180.0000, 192.2170, 2.2000},
      detail::atom_record{78, "Platinum", "Pt", "[Xe] 4f14 5d9 6s1", 177.0000, 195.0840, 2.2800},
      detail::atom_record{79, "Gold", "Au", "[Xe] 4f14 5d10 6s1", 174.0000, 196.9666, 2.5400},
      detail::atom_record{80, "Mercury", "Hg", "[Xe] 4f14 5d10 6s2", 171.0000, 200.5920, 2.0000},
      detail::atom_record{81, "Thallium", "Tl", "[Xe] 4f14 5d10 6s2 6p1", 156.0000, 204.3800,
                          1.6200},
      detail::atom_record{82, "Lead", "Pb", "[Xe] 4f14 5d10 6s2 6p2", 154.0000, 207.2000, 2.3300},
      detail::atom_record{83, "Bismuth", "Bi", "[Xe] 4f14 5d10 6s2 6p3", 143.0000, 208.9804,
                          2.0200},
      detail::atom_record{84, "Polonium", "Po", "[Xe] 4f14 5d10 6s2 6p4", 135.0000, 209.0000,
                          2.0000},
      detail::atom_record{85, "Astatine", "At", "[Xe] 4f14 5d10 6s2 6p5", 0.0000, 210.0000, 2.2000},
      detail::atom_record{86, "Radon", "Rn", "[Xe] 4f14 5d10 6s2 6p6", 120.0000, 222.0000, 0.0000},
      detail::atom_record{87, "Francium", "Fr", "[Rn] 7s1", 0.0000, 223.0000, 0.7000},
      detail::atom_record{88, "Radium", "Ra", "[Rn] 7s2", 0.0000, 226.0000, 0.9000},
      detail::atom_record{89, "Actinium", "Ac", "[Rn] 6d1 7s2", 0.0000, 227.0000, 1.1000},
      detail::atom_record{90, "Thorium", "Th", "[Rn] 6d2 7s2", 0.0000, 232.0381, 1.3000},
      detail::atom_record{91, "Protactinium", "Pa", "[Rn] 5f2 6d1 7s2", 0.0000, 231.0359, 1.5000},
      detail::atom_record{92, "Uranium", "U", "[Rn] 5f3 6d1 7s2", 0.0000, 238.0289, 1.3800},
      detail::atom_record{93, "Neptunium", "Np", "[Rn] 5f4 6d1 7s2", 0.0000, 237.0000, 1.3600},
      detail::atom_record{94, "Plutonium", "Pu", "[Rn] 5f6 7s2", 0.0000, 244.0000, 1.2800},
      detail::atom_record{95, "Americium", "Am", "[Rn] 5f7 7s2", 0.0000, 243.0000, 1.3000},
      detail::atom_record{96, "Curium", "Cm", "[Rn] 5f7 6d1 7s2", 0.0000, 247.0000, 1.3000},
      detail::atom_record{97, "Berkelium", "Bk", "[Rn] 5f9 7s2", 0.0000, 247.0000, 1.3000},
      detail::atom_record{98, "Californium", "Cf", "[Rn] 5f10 7s2", 0.0000, 251.0000, 1.3000},
      detail::atom_record{99, "Einsteinium", "Es", "[Rn] 5f11 7s2", 0.0000, 252.0000, 1.3000},
      detail::atom_record{100, "Fermium", "Fm", "[Rn] 5f12 7s2", 0.0000, 257.0000, 1.3000},
      detail::atom_record{101, "Mendelevium", "Md", "[Rn] 5f13 7s2", 0.0000, 258.0000, 1.3000},
      detail::atom_record{102, "Nobelium", "No", "[Rn] 5f14 7s2", 0.0000, 259.0000, 1.3000},
      detail::atom_record{103, "Lawrencium", "Lr", "[Rn] 5f14 7s2 7p1", 0.0000, 262.0000, 1.3000},
      detail::atom_record{104, "Rutherfordium", "Rf", "[Rn] 5f14 6d2 7s2", 0.0000, 267.0000,
                          1.3000},
      detail::atom_record{105, "Dubnium", "Db", "[Rn] 5f14 6d3 7s2", 0.0000, 268.0000, 1.3000},
      detail::atom_record{106, "Seaborgium", "Sg", "[Rn] 5f14 6d4 7s2", 0.0000, 269.0000, 0.0000},
      detail::atom_record{107, "Bohrium", "Bh", "[Rn] 5f14 6d5 7s2", 0.0000, 270.0000, 0.0000},
      detail::atom_record{108, "Hassium", "Hs", "[Rn] 5f14 6d6 7s2", 0.0000, 269.0000, 0.0000},
      detail::atom_record{109, "Meitnerium", "Mt", "[Rn] 5f14 6d7 7s2", 0.0000, 278.0000, 0.0000},
      detail::atom_record{110, "Darmstadtium", "Ds", "[Rn] 5f14 6d8 7s2", 0.0000, 281.0000, 0.0000},
      detail::atom_record{111, "Roentgenium", "Rg", "[Rn] 5f14 6d9 7s2", 0.0000, 281.0000, 0.0000},
      detail::atom_record{112, "Copernicium", "Cn", "[Rn] 5f14 6d10 7s2", 0.0000, 285.0000, 0.0000},
      detail::atom_record{113, "Ununtrium", "Uut", "[Rn] 5f14 6d10 7s2 7p1", 0.0000, 286.0000,
                          0.0000},
      detail::atom_record{114, "Flerovium", "Fl", "[Rn] 5f14 6d10 7s2 7p2", 0.0000, 289.0000,
                          0.0000},
  };

  static auto _make_symbol_map() {
    std::unordered_map<std::string, specie_t> map(KNOWN_ELEMENTS.size());
    for (auto record : KNOWN_ELEMENTS) map.emplace(std::get<2>(record), std::get<0>(record));
    return map;
  }

  static std::unordered_map SYMBOL_MAP{_make_symbol_map()};
  class Atom {
  public:
    specie_t Z;         /**< The ordinal number of the element in the periodic table */
    std::string name;   /**< The full english name of the element. E. g. "Iron" */
    std::string symbol; /**< The two character symbol of an element. E.g "Fe" for iron */
    std::string electronic_configuration; /**< The electronic configuration as a string */
    double atomic_radius;                 /**< Atomic radius in pm */
    double mass;                          /**< The mass of the atoms in atomic units */
    double en;                            /**< Electronegativity */

    template <std::integral T> static Atom from_z(T ordinal) {
      if (ordinal > KNOWN_ELEMENTS.size() && ordinal < 0)
        throw std::out_of_range("Element is out of range");
      return Atom(KNOWN_ELEMENTS[ordinal]);
    }

    static Atom from_symbol(std::string const& symbol) {
      if (!SYMBOL_MAP.contains(symbol))
        throw std::out_of_range(std::format("Unknown element {}", symbol));
      return from_z(SYMBOL_MAP.at(symbol));
    }

  private:
    explicit Atom(detail::atom_record const& record)
        : Z(std::get<0>(record)),
          name(std::get<1>(record)),
          symbol(std::get<2>(record)),
          electronic_configuration(std::get<3>(record)),
          atomic_radius(std::get<4>(record)),
          mass(std::get<5>(record)),
          en(std::get<6>(record)) {}
  };

}  // namespace sqsgen::core
#endif  // ATOM_H
