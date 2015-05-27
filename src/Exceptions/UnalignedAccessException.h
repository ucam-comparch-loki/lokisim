/*
 * UnalignedAccessException.h
 *
 *  Created on: 3 Feb 2015
 *      Author: db434
 */

#ifndef SRC_EXCEPTIONS_UNALIGNEDACCESSEXCEPTION_H_
#define SRC_EXCEPTIONS_UNALIGNEDACCESSEXCEPTION_H_

class UnalignedAccessException : public std::exception {

public:

  UnalignedAccessException(uint address, uint alignment) {
    address_ = address;
    alignment_ = alignment;
  }

  virtual ~UnalignedAccessException() throw() {

  }

  virtual const char* what() const throw() {
    std::stringstream ss;

    ss << "Attempting to access address 0x" << std::hex << address_
       << std::dec << " with alignment " << alignment_ << ".";

    return ss.str().c_str();
  }

private:

  uint address_;
  uint alignment_;

};

#endif /* SRC_EXCEPTIONS_UNALIGNEDACCESSEXCEPTION_H_ */
