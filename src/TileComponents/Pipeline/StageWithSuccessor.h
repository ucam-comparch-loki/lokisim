/*
 * StageWithSuccessor.h
 *
 * A pipeline stage which is known to have a successor stage. This allows us to
 * standardise the outputs.
 *
 *  Created on: 27 Oct 2010
 *      Author: db434
 */

#ifndef STAGEWITHSUCCESSOR_H_
#define STAGEWITHSUCCESSOR_H_

#include "PipelineStage.h"

class StageWithSuccessor : public virtual PipelineStage {

//==============================//
// Ports
//==============================//

public:

// Inherited from PipelineStage:
//   sc_in<bool>  clock
//   sc_out<bool> idle

  // The decoded instruction after passing through this pipeline stage.
  // DecodedInst holds all necessary fields for data at all stages throughout
  // the pipeline.
  sc_out<DecodedInst> dataOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  StageWithSuccessor(sc_module_name name, ComponentID ID);

};

#endif /* STAGEWITHSUCCESSOR_H_ */
