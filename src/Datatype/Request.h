/*
 * Request.h
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#ifndef REQUEST_H_
#define REQUEST_H_

#include "Word.h"

class Request: public Word {

public:
/* Constructors and destructors */
  Request(short returnID, short numFlits, bool readRequest);
  Request(const Word& other);
  virtual ~Request();

/* Methods */
  short getReturnID() const;
  short getNumFlits() const;
  bool isReadRequest() const;

};

#endif /* REQUEST_H_ */
