/*
 * OutOfBoundsException.h
 *
 *  Created on: 13 Sep 2010
 *      Author: db434
 */

#ifndef OUTOFBOUNDSEXCEPTION_H_
#define OUTOFBOUNDSEXCEPTION_H_

#include <sstream>

class OutOfBoundsException : public std::exception {

public:

  OutOfBoundsException(int32_t accessed, int32_t max=100000000) {
    _accessed = accessed;
    _max = max;
  }

  virtual const char* what() const throw() {
    std::stringstream ss;

    ss << "Attempting to access memory address " << _accessed;
    if(_accessed >= _max) ss << " (max = " << _max << ")";
    ss << ".";

    return ss.str().c_str();
  }

private:

  int32_t _accessed;
  int32_t _max;

};

#endif /* OUTOFBOUNDSEXCEPTION_H_ */
