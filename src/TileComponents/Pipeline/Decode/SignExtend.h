/*
 * SignExtend.h
 *
 * Component which pads out immediate values to 32 bits, whilst ensuring
 * that the sign of the value is unchanged.
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#ifndef SIGNEXTEND_H_
#define SIGNEXTEND_H_

#include "../../../Component.h"

class SignExtend: public Component {

//==============================//
// Constructors and destructors
//==============================//

public:
  SignExtend(sc_module_name name);
  virtual ~SignExtend();

//==============================//
// Methods
//==============================//

  // Pad the received value out to 32 bits, preserving the sign.
  // This will probably not be needed in the simulator because all values
  // are automatically extended.
  int32_t extend(int32_t val);

};

#endif /* SIGNEXTEND_H_ */
