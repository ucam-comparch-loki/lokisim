/*
 * BlockedException.h
 *
 *  Created on: 13 Sep 2010
 *      Author: db434
 */

#ifndef BLOCKEDEXCEPTION_H_
#define BLOCKEDEXCEPTION_H_

class BlockedException : public std::exception {

public:

  BlockedException(std::string location, int clusterID) {
    location_ = location;
    id_       = clusterID;
  }

  virtual ~BlockedException() throw() {

  }

  virtual const char* what() const throw() {
    std::stringstream ss;

    ss << "Pipeline " << id_ << " blocked at " << location_ << ".";

    return ss.str().c_str();
  }

private:

  std::string location_;
  int         id_;

};


#endif /* BLOCKEDEXCEPTION_H_ */
