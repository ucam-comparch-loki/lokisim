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
    accessed_ = accessed;
    max_ = max;
  }

  virtual const char* what() const throw() {
    std::stringstream ss;

    ss << "Attempting to access memory address " << accessed_;
    if(accessed_ >= max_) ss << " (max = " << max_ << ")";
    ss << ".";

    return ss.str().c_str();
  }

private:

  int32_t accessed_;
  int32_t max_;

};

#endif /* OUTOFBOUNDSEXCEPTION_H_ */
