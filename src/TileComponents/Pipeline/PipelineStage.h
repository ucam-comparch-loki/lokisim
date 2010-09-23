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
#include "../../flag_signal.h"
#include "../../Datatype/DecodedInst.h"

class Cluster;

class PipelineStage : public Component {

//==============================//
// Ports
//==============================//

public:

  // Clock.
  sc_in<bool>  clock;

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

  // Do I want a parent() method, so the user has to know the module hierarchy,
  // but can access any position in it, or a cluster() method, which hides
  // any changes, but makes arbitrary access harder?
  Cluster* parent() const;

  // The main loop responsible for this pipeline stage. It first initialises
  // the stage, then calls newCycle() at each new clock cycle.
  virtual void execute();

  // To simulate pipelining, all pipeline stages execute a method every cycle.
  // This method usually involves checking for new data, and passing it to
  // subcomponents if appropriate.
  virtual void newCycle() = 0;

  // Some components require initialisation before execution begins.
  // Initialisation may involve operations such as setting flow control values.
  virtual void initialise();

};

#endif /* PIPELINESTAGE_H_ */
