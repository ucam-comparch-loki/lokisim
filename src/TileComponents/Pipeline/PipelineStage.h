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

//==============================//
// Methods
//==============================//

protected:

  // Do I want a parent() method, so the user has to know the module hierarchy,
  // but can access any position in it, or a cluster() method, which hides
  // any changes, but makes arbitrary access harder?
  Cluster* parent() const;

  // The main loop responsible for this pipeline stage. It should execute
  // appropriate methods at appropriate points in each clock cycle.
  virtual void execute() = 0;

  // Recompute whether this pipeline stage is ready for new input.
  virtual void updateReady() = 0;

};

#endif /* PIPELINESTAGE_H_ */
