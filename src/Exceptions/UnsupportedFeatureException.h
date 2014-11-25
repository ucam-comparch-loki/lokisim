/*
 * UnsupportedFeatureException.h
 *
 * A feature which is not yet implemented was used.
 *
 *  Created on: 3 Oct 2014
 *      Author: db434
 */

#ifndef UNSUPPORTEDFEATUREEXCEPTION_H_
#define UNSUPPORTEDFEATUREEXCEPTION_H_

class UnsupportedFeatureException : public std::exception {

public:

  UnsupportedFeatureException(std::string description) {
    description_ = description;
  }

  virtual ~UnsupportedFeatureException() throw() {}

  virtual const char* what() const throw() {
    std::stringstream ss;

    ss << "Attempted to use unsupported feature: " << description_;

    return ss.str().c_str();
  }

private:

  std::string  description_;

};

#endif /* UNSUPPORTEDFEATUREEXCEPTION_H_ */
