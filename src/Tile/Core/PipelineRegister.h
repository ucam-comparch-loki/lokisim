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

#include "../../Datatype/DecodedInst.h"
#include "../../LokiComponent.h"
#include "../../Network/FIFOs/FIFO.h"

class core_pipeline_consumer_ifc : virtual public sc_interface {
public:
  // Can an instruction be read?
  virtual bool canRead() const = 0;

  // Read an instruction.
  virtual const DecodedInst& read() = 0;

  // Event triggered when `canRead()` becomes `true`.
  virtual const sc_event& canReadEvent() const = 0;

  // Read and discard the next instruction. Returns whether there was an
  // instruction to be discarded.
  virtual bool discard() = 0;
};

class core_pipeline_producer_ifc : virtual public sc_interface {
public:
  // Can an instruction be written?
  virtual bool canWrite() const = 0;

  // Write an instruction.
  virtual void write(const DecodedInst& inst) = 0;

  // Event triggered when `canWrite()` becomes `true`.
  virtual const sc_event& canWriteEvent() const = 0;
};

class core_pipeline_inout_ifc : virtual public core_pipeline_producer_ifc,
                                virtual public core_pipeline_consumer_ifc {
  // Nothing extra.
};


typedef sc_port<core_pipeline_consumer_ifc> InstructionInput;
typedef sc_port<core_pipeline_producer_ifc> InstructionOutput;


class PipelineRegister: public LokiComponent, public core_pipeline_inout_ifc {

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
                   const PipelinePosition pos);

//============================================================================//
// Methods
//============================================================================//

public:

  // Is this register ready to receive the next instruction?
  virtual bool canWrite() const;
  virtual void write(const DecodedInst& inst);

  // Is this register ready to provide the next instruction?
  virtual bool canRead() const;
  virtual const DecodedInst& read();

  virtual const sc_event& canReadEvent() const;
  virtual const sc_event& canWriteEvent() const;

  // Discard a single instruction from the register, if there is one.
  // Returns whether an instruction was discarded.
  virtual bool discard();

//============================================================================//
// Local state
//============================================================================//

private:

  DecodedInst data;
  bool valid;

  sc_event readEvent, writeEvent;

  const PipelinePosition position;

};

#endif /* PIPELINEREGISTER_H_ */
