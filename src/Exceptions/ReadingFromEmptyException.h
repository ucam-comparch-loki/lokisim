/*
 * ReadingFromEmptyException.h
 *
 * Shows that the system tried to read from an empty queue or stack.
 *
 *  Created on: 14 Sep 2010
 *      Author: db434
 */

#ifndef READINGFROMEMPTYEXCEPTION_H_
#define READINGFROMEMPTYEXCEPTION_H_

class ReadingFromEmptyException : public std::exception {

public:

  ReadingFromEmptyException(std::string name = std::string("storage location")) {
    name_ = name;
  }

  virtual ~ReadingFromEmptyException() throw() {

  }

  virtual const char* what() const throw() {
    std::stringstream ss;

    ss << "Attempting to read from empty " << name_ << ".";

    return ss.str().c_str();
  }

private:

  std::string name_;

};

#endif /* READINGFROMEMPTYEXCEPTION_H_ */
