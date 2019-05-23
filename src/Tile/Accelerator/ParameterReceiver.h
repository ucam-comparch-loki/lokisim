/*
 * ParameterReceiver.h
 *
 * Component responsible for receiving a convolution configuration and notifying
 * when all parameters have arrived.
 *
 *  Created on: 23 Jul 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_PARAMETERRECEIVER_H_
#define SRC_TILE_ACCELERATOR_PARAMETERRECEIVER_H_

#include "../../LokiComponent.h"
#include "../../Network/FIFOs/NetworkFIFO.h"
#include "AcceleratorTypes.h"

class ParameterReceiver: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  // Receive parameters for the next computation.
  sc_port<network_sink_ifc<Word>> iParameter;


//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(ParameterReceiver);

  ParameterReceiver(sc_module_name name);


//============================================================================//
// Methods
//============================================================================//

public:

  // Does this unit have a complete set of parameters?
  bool hasAllParameters() const;

  // Get a copy of all parameters received. Doing this frees up the component
  // to receive the next batch of parameters.
  conv_parameters_t getParameters();

  // Event which is triggered when the final parameter arrives.
  const sc_event& allParametersArrived() const;

private:

  // Receive a parameter and store it locally.
  void receiveParameter();


//============================================================================//
// Local state
//============================================================================//

private:

  conv_parameters_t parameters;
  int parametersReceived;

  NetworkFIFO<Word> inputBuffer;

  sc_event allParametersArrivedEvent;

};

#endif /* SRC_TILE_ACCELERATOR_PARAMETERRECEIVER_H_ */
