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

//==============================//
// Methods
//==============================//

public:

  short getReturnID() const;
  short getNumFlits() const;
  bool  isReadRequest() const;

//==============================//
// Constructors and destructors
//==============================//

public:

  Request(int returnID, int numFlits = 1, bool readRequest = false);
  Request(const Word& other);
  virtual ~Request();

};

#endif /* REQUEST_H_ */
