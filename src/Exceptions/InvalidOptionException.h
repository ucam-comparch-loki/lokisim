/*
 * InvalidOptionException.h
 *
 * An invalid or unsupported option was selected.
 *
 *  Created on: 3 Oct 2014
 *      Author: db434
 */

#ifndef INVALIDOPTIONEXCEPTION_H_
#define INVALIDOPTIONEXCEPTION_H_

class InvalidOptionException : public std::exception {

public:

  InvalidOptionException(std::string description, unsigned int option) {
    description_ = description;
    option_ = option;
  }

  virtual ~InvalidOptionException() throw() {}

  virtual const char* what() const throw() {
    std::stringstream ss;

    ss << "Invalid option selected for " << description_ << ": " << option_;

    return ss.str().c_str();
  }

private:

  std::string  description_;
  unsigned int option_;

};


#endif /* INVALIDOPTIONEXCEPTION_H_ */
