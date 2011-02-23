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

#include "../../Component.h"

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

protected:

  SC_HAS_PROCESS(PipelineStage);
  PipelineStage(sc_module_name name, ComponentID ID);

public:

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
  virtual void newCycle();

  // For some pipeline stages, we may need to be sure that some information has
  // arrived (e.g. a "ready" signal from the next stage). To do this, we
  // execute this method later in the cycle.
  virtual void cycleSecondHalf();

  // If this pipeline stage has any output connections to the network, send
  // any data on them now.
  virtual void sendOutputs();

  // Recompute whether this pipeline stage is stalled.
  virtual void updateStall() = 0;

  // Some components require initialisation before execution begins.
  // Initialisation may involve operations such as setting flow control values.
  virtual void initialise();

};

#endif /* PIPELINESTAGE_H_ */
