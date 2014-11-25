/*
 * ReadOnlyException.h
 *
 * Some sections of an ELF executable are marked as read-only. This exception
 * is used when memory attempts to modify one of these sections.
 *
 *  Created on: 3 Oct 2014
 *      Author: db434
 */

#ifndef READONLYEXCEPTION_H_
#define READONLYEXCEPTION_H_

#include "../Typedefs.h"

class ReadOnlyException : public std::exception {

public:

  ReadOnlyException(MemoryAddr address) {
    address_ = address;
  }

  virtual ~ReadOnlyException() throw() {}

  virtual const char* what() const throw() {
    std::stringstream ss;

    ss << "Attempting to write to read-only memory address: " << address_
       << " (0x" << std::hex << address_ << std::dec << ")";

    return ss.str().c_str();
  }

private:

  MemoryAddr address_;

};


#endif /* READONLYEXCEPTION_H_ */
