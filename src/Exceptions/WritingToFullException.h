/*
 * WritingToFullException.h
 *
 *  Created on: 14 Sep 2010
 *      Author: db434
 */

#ifndef WRITINGTOFULLEXCEPTION_H_
#define WRITINGTOFULLEXCEPTION_H_

class WritingToFullException : public std::exception {

public:

  WritingToFullException(std::string name = std::string("storage location")) {
    _name = name;
  }

  virtual ~WritingToFullException() throw() {

  }

  virtual const char* what() const throw() {
    std::stringstream ss;

    ss << "Attempting to write to full " << _name << ".";

    return ss.str().c_str();
  }

private:

  std::string _name;

};

#endif /* WRITINGTOFULLEXCEPTION_H_ */
