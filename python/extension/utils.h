//
// Created by Dominik Gehringer on 11.03.25.
//

#ifndef UTILS_H
#define UTILS_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace sqsgen::python::helpers {

   inline std::string read_file(const std::string& filePath) {
    std::ifstream fileStream(filePath);
    if (!fileStream) throw std::runtime_error(std::format("Could not open file '{}'", filePath));

    std::stringstream buffer;
    buffer << fileStream.rdbuf();
    return buffer.str();
  }

}  // namespace sqsgen::python::helpers

#endif  // UTILS_H
