/*
 * InvalidInstructionException.h
 *
 *  Created on: 16 Sep 2010
 *      Author: db434
 */

#ifndef INVALIDINSTRUCTIONEXCEPTION_H_
#define INVALIDINSTRUCTIONEXCEPTION_H_

class InvalidInstructionException : public std::exception {

public:

  InvalidInstructionException(std::string text = std::string("")) {
    _name = text;
  }

  virtual ~InvalidInstructionException() throw() {

  }

  virtual const char* what() const throw() {
    return _name.c_str();
  }

private:

  std::string _name;

};

#endif /* INVALIDINSTRUCTIONEXCEPTION_H_ */
