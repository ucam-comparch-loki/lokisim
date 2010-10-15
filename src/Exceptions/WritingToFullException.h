/*
 * WritingToFullException.h
 *
 * Shows that the system tried to write to a full queue or stack.
 *
 *  Created on: 14 Sep 2010
 *      Author: db434
 */

#ifndef WRITINGTOFULLEXCEPTION_H_
#define WRITINGTOFULLEXCEPTION_H_

class WritingToFullException : public std::exception {

public:

  WritingToFullException(std::string name = std::string("storage location")) {
    name_ = name;
  }

  virtual ~WritingToFullException() throw() {

  }

  virtual const char* what() const throw() {
    std::stringstream ss;

    ss << "Attempting to write to full " << name_ << ".";

    return ss.str().c_str();
  }

private:

  std::string name_;

};

#endif /* WRITINGTOFULLEXCEPTION_H_ */
