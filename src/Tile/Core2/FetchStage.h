/*
 * FetchStage.h
 *
 *  Created on: Aug 14, 2019
 *      Author: db434
 */

#ifndef SRC_TILE_CORE_FETCHSTAGE_H_
#define SRC_TILE_CORE_FETCHSTAGE_H_

#include "PipelineStage.h"

namespace Compute {

class FetchStage: public FirstPipelineStage {

//============================================================================//
// Constructors and destructors
//============================================================================//
public:
  FetchStage(sc_module_name name);

//============================================================================//
// Methods
//============================================================================//

public:

  virtual void execute();

};

} // end namespace

#endif /* SRC_TILE_CORE_FETCHSTAGE_H_ */
