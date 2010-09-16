/*
 * ReadingFromEmptyException.h
 *
 *  Created on: 14 Sep 2010
 *      Author: db434
 */

#ifndef READINGFROMEMPTYEXCEPTION_H_
#define READINGFROMEMPTYEXCEPTION_H_

class ReadingFromEmptyException : public std::exception {

public:

  ReadingFromEmptyException(string& name = "storage location") {
    _name = name;
  }

  virtual const char* what() const throw() {
    std::stringstream ss;

    ss << "Attempting to read from empty " << _name << ".";

    return ss.str().c_str();
  }

private:

  string _name;

};

#endif /* READINGFROMEMPTYEXCEPTION_H_ */
