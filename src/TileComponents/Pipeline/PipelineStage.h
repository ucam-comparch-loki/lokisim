/*
 * PipelineStage.h
 *
 * The base class for all pipeline stages.
 *
 *  Created on: 7 Jan 2010
 *      Author: db434
 */

#ifndef PIPELINESTAGE_H_
#define PIPELINESTAGE_H_

// Copy a value from an input to an output, only if the value is new
#define COPY_IF_NEW(input, output) if(input.event()) output.write(input.read())

#include "../../Component.h"
#include "../../Datatype/Word.h"
#include "../../Datatype/Address.h"
#include "../../Datatype/Data.h"
#include "../../Datatype/DecodedInst.h"
#include "../../Datatype/Instruction.h"
#include "../../Datatype/AddressedWord.h"

class PipelineStage : public Component {

//==============================//
// Ports
//==============================//

public:

  // Clock.
  sc_in<bool>  clock;

  // When true, the pipeline stage should do nothing. When false, execution
  // can continue as normal.
  sc_in<bool>  stall;

  // Signal that this pipeline stage is currently idle.
  sc_out<bool> idle;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(PipelineStage);
  PipelineStage(sc_module_name name);
  ~PipelineStage();

//==============================//
// Methods
//==============================//

protected:

  // To simulate pipelining, all pipeline stages execute a method every cycle.
  // This method usually involves copying all of the inputs to the
  // subcomponents that use them.
  virtual void newCycle() = 0;

};

#endif /* PIPELINESTAGE_H_ */
