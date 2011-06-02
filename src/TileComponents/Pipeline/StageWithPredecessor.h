/*
 * StageWithPredecessor.h
 *
 * A pipeline stage which is known to have a predecessor stage. This allows us
 * to standardise the inputs.
 *
 *  Created on: 27 Oct 2010
 *      Author: db434
 */

#ifndef STAGEWITHPREDECESSOR_H_
#define STAGEWITHPREDECESSOR_H_

#include "PipelineStage.h"

class StageWithPredecessor : public virtual PipelineStage {

//==============================//
// Ports
//==============================//

public:

// Inherited from PipelineStage:
//   sc_in<bool>  clock
//   sc_out<bool> idle

  // The input instruction to be working on. DecodedInst holds all information
  // required for any pipeline stage to do its work.
  sc_in<DecodedInst> dataIn;

  // Tell whether this stage is ready for input (ignoring effects of any other stages).
  sc_out<bool>       readyOut;

//==============================//
// Methods
//==============================//

protected:  // See if we can get away with execute and updateStall being private

  // The main loop controlling this stage. Involves waiting for new input,
  // doing work on it, and sending it to the next stage.
  virtual void execute();

  // Determine whether this stage is stalled or not, and write the appropriate
  // output.
  virtual void updateReady();

  // Returns whether this stage is currently stalled (ignoring effects of any
  // other stages).
  virtual bool isStalled() const = 0;

  // Deal with a new input DecodedInst. Must be implemented in each stage.
  virtual void newInput(DecodedInst& dec) = 0;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(StageWithPredecessor);
  StageWithPredecessor(sc_module_name name, const ComponentID& ID);
  virtual ~StageWithPredecessor();

};

#endif /* STAGEWITHPREDECESSOR_H_ */
