/*
 * WriteStage.h
 *
 *  Created on: Aug 14, 2019
 *      Author: db434
 */

#ifndef SRC_TILE_CORE_WRITESTAGE_H_
#define SRC_TILE_CORE_WRITESTAGE_H_

#include "PipelineStage.h"

namespace Compute {

class WriteStage: public LastPipelineStage {

//============================================================================//
// Constructors and destructors
//============================================================================//
public:
  WriteStage(sc_module_name name);

//============================================================================//
// Methods
//============================================================================//

public:

  virtual void execute();

  // Part of the CoreInterface implemented here.
  virtual void writeRegister(RegisterIndex index, int32_t value);
  virtual void sendOnNetwork(NetworkData flit);

};

} // end namespace

#endif /* SRC_TILE_CORE_WRITESTAGE_H_ */
