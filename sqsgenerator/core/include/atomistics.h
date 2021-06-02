//
// Created by dominik on 02.06.21.
//

#ifndef SQSGENERATOR_ATOMISTICS_H
#define SQSGENERATOR_ATOMISTICS_H

#include "types.h"
#include <map>
#include <vector>
#include <string>

namespace sqsgenerator::utils::atomistics {

    class Atom {
    public:
        const Species Z;
        const std::string name;
        const std::string symbol;
        const std::string electronicConfiguration;
        const double atomicRadius;
        const double mass;
        const double en;

    };

    class Atoms {


    private:
        const static std::vector<Atom> m_elements;


        static std::map<std::string , Species> makeSymbolMap() {
            static std::map<std::string, Species> symbolMap;
            for( const auto &atom : Atoms::m_elements)  symbolMap.emplace(atom.symbol, atom.Z);
            return symbolMap;
        }

        static std::map<std::string , Species> m_symbolMap;

    public:
        //TODO: Ask how to make m_elements private while be able to initialize m_symbolMap;

        static Atom fromZ(Species Z);
        static Atom fromSymbol(const std::string &symbol);
        static std::vector<Atom> fromZ(const std::vector<Species> &numbers);
        static std::vector<Atom> fromSymbol(const std::vector<std::string> &symbols);
    };
}


#endif //SQSGENERATOR_ATOMISTICS_H
