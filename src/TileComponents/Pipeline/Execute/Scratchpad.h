/*
 * Scratchpad.h
 *
 * The evolution of the "extended" part of the extended register file.
 *
 * The scratchpad sits in the execute stage with a single read/write port and
 * is only accessible through indirect reads and writes.
 *
 *  Created on: 17 Jan 2013
 *      Author: db434
 */

#ifndef SCRATCHPAD_H_
#define SCRATCHPAD_H_

#include "../../../Component.h"
#include "../../../Datatype/Word.h"
#include "../../../Memory/AddressedStorage.h"

class Scratchpad: public Component {

//==============================//
// Constructors and destructors
//==============================//

public:

  Scratchpad(const sc_module_name& name, const ComponentID& ID);

//==============================//
// Methods
//==============================//

public:

  const int32_t read(const MemoryAddr addr) const;
  void          write(const MemoryAddr addr, const int32_t value);

//==============================//
// Local state
//==============================//

private:

  AddressedStorage<Word> data;

};

#endif /* SCRATCHPAD_H_ */
