/*
 * PipelineRegister.h
 *
 * Two registers to go between each pair of pipeline stages, removing the need
 * for instantaneous propagation of pipeline flow control signals.
 *
 * Loosely based (much higher-level) on the implementation in
 * http://www.cl.cam.ac.uk/~rdm34/Memos/localstall.pdf
 *
 * NOTE: I have switched back to a one-register implementation, as having two
 * registers makes forwarding awkward.
 *
 *  Created on: 15 Mar 2012
 *      Author: db434
 */

#ifndef PIPELINEREGISTER_H_
#define PIPELINEREGISTER_H_

#include "../../Component.h"
#include "../../Datatype/DecodedInst.h"
#include "../../Memory/BufferStorage.h"

class PipelineRegister: public Component {

//============================================================================//
// Types
//============================================================================//

public:

  // Since we are using DecodedInst as a uniform datatype to pass down the
  // pipeline, we need a way of distinguishing between the registers in
  // different positions - they are all of different widths.
  enum PipelinePosition {
    FETCH_DECODE,
    DECODE_EXECUTE,
    EXECUTE_WRITE
  };

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  PipelineRegister(const sc_module_name& name,
                   const ComponentID ID,
                   const PipelinePosition pos);

//============================================================================//
// Methods
//============================================================================//

public:

  // Is this register ready to receive the next instruction?
  bool ready() const;
  void write(const DecodedInst& inst);

  // Is this register ready to provide the next instruction?
  bool hasData() const;
  const DecodedInst& read();

  const sc_event& dataAdded() const;
  const sc_event& dataRemoved() const;

  // Discard a single instruction from the register, if there is one.
  // Returns whether an instruction was discarded.
  bool discard();

//============================================================================//
// Local state
//============================================================================//

private:

  // My implementation of the two registers.
//  BufferStorage<DecodedInst> buffer;

  DecodedInst data;
  bool valid;

  sc_event readEvent, writeEvent;

  const PipelinePosition position;

};

#endif /* PIPELINEREGISTER_H_ */
