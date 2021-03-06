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

#include "../../../Datatype/Word.h"
#include "../../../LokiComponent.h"

class Scratchpad: public LokiComponent {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  Scratchpad(const sc_module_name& name,
             const scratchpad_parameters_t& params);

//============================================================================//
// Methods
//============================================================================//

public:

  int32_t read(RegisterIndex addr) const;
  void    write(RegisterIndex addr, int32_t value);

//============================================================================//
// Local state
//============================================================================//

private:

  vector<int32_t> data;

};

#endif /* SCRATCHPAD_H_ */
