/*
 * PipelineStage.h
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
#include "../../Datatype/Instruction.h"
#include "../../Datatype/AddressedWord.h"

class PipelineStage : public Component {

public:
/* Ports */
  sc_in<bool> clock;

/* Constructors and destructors */
  SC_HAS_PROCESS(PipelineStage);
  PipelineStage(sc_module_name name);
  ~PipelineStage();

protected:
/* Methods */
  virtual void newCycle() = 0;

};

#endif /* PIPELINESTAGE_H_ */
