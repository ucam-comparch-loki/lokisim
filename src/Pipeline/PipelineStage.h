/*
 * PipelineStage.h
 *
 *  Created on: 7 Jan 2010
 *      Author: db434
 */

#ifndef PIPELINESTAGE_H_
#define PIPELINESTAGE_H_

#include "../Component.h"
#include "../Datatype/Word.h"
#include "../Datatype/Address.h"
#include "../Datatype/Data.h"
#include "../Datatype/Instruction.h"
#include "../Datatype/AddressedWord.h"

class PipelineStage : public Component {

protected:
  // Is there anything all the stages have in common?
  // Main input? Main output? A doOp method?

/* Methods */
  virtual void newCycle() = 0;

public:
/* Ports */
  sc_in<bool> clock;

/* Constructors and destructors */
  SC_HAS_PROCESS(PipelineStage);
  PipelineStage(sc_core::sc_module_name, int ID);
  ~PipelineStage();

};

#endif /* PIPELINESTAGE_H_ */
