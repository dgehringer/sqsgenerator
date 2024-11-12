//
// Created by Dominik Gehringer on 05.11.24.
//

#ifndef SQSGEN_CONFIGURATION_H
#define SQSGEN_CONFIGURATION_H



namespace sqsgen {

  enum Prec : std::uint8_t{
    PREC_SINGLE = 0,
    PREC_DOUBLE = 1,
  };

  template<class T>
  class configuration {
  };

}

#endif //SQSGEN_CONFIGURATION_H
