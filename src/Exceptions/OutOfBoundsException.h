/*
 * OutOfBoundsException.h
 *
 * Shows that an invalid memory address was accessed.
 *
 *  Created on: 13 Sep 2010
 *      Author: db434
 */

#ifndef OUTOFBOUNDSEXCEPTION_H_
#define OUTOFBOUNDSEXCEPTION_H_

#include <sstream>
#include "../Typedefs.h"

class OutOfBoundsException : public std::exception {

public:

  OutOfBoundsException(MemoryAddr accessed, MemoryAddr max) {
    accessed_ = accessed;
    max_ = max;
  }

  virtual const char* what() const throw() {
    std::stringstream ss;

    ss << "Attempting to access memory address " << accessed_;
    if (accessed_ >= max_)
      ss << " (max = " << max_ << ")";
    ss << ".";

    return ss.str().c_str();
  }

private:

  MemoryAddr accessed_;
  MemoryAddr max_;

};

#endif /* OUTOFBOUNDSEXCEPTION_H_ */
