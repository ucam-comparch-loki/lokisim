/*
 * InstructionHandler.h
 *
 * Unit responsible for handling one aspect of an instruction's execution.
 *
 * This unit is notified when the instruction begins a particular phase of
 * execution, and then waits until it can tell the instruction that the
 * process has finished.
 *
 *  Created on: Aug 14, 2019
 *      Author: db434
 */

#ifndef SRC_TILE_CORE_INSTRUCTIONHANDLER_H_
#define SRC_TILE_CORE_INSTRUCTIONHANDLER_H_

#include "../../ISA/InstructionDecode.h"
#include "../../LokiComponent.h"

namespace Compute {

class Core;
class PipelineStage;
using sc_core::sc_event;

class InstructionHandler: public LokiComponent {
protected:
  SC_HAS_PROCESS(InstructionHandler);
  InstructionHandler(sc_module_name name);

public:
  // Notify this InstructionHandler that an instruction has started this phase
  // of execution.
  void begin(DecodedInstruction inst);

protected:

  // Tell the instruction that the core has finished a request. Return a result
  // to the instruction if appropriate.
  void respondToInstruction(DecodedInstruction inst) const;

  // Event triggered when the core has finished a request.
  const sc_event& coreFinishedEvent() const;

  // The parent pipeline stage.
  PipelineStage& stage() const;

  // Reference to this core.
  Core& core() const;

private:
  void mainLoop();

  sc_event beginEvent;
  bool active;

  DecodedInstruction currentInstruction;
};


class ReadRegisterHandler: public InstructionHandler {
public:
  ReadRegisterHandler(sc_module_name name, RegisterPort port);
  void respondToInstruction(DecodedInstruction inst) const;
  const sc_event& coreFinishedEvent() const;
private:
  const RegisterPort port;
};

class WriteRegisterHandler: public InstructionHandler {
public:
  WriteRegisterHandler(sc_module_name name);
  void respondToInstruction(DecodedInstruction inst) const;
  const sc_event& coreFinishedEvent() const;
};

class ComputeHandler: public InstructionHandler {
public:
  ComputeHandler(sc_module_name name);
  void respondToInstruction(DecodedInstruction inst) const;
  const sc_event& coreFinishedEvent() const;
};

class ReadCMTHandler: public InstructionHandler {
public:
  ReadCMTHandler(sc_module_name name);
  void respondToInstruction(DecodedInstruction inst) const;
  const sc_event& coreFinishedEvent() const;
};

class WriteCMTHandler: public InstructionHandler {
public:
  WriteCMTHandler(sc_module_name name);
  void respondToInstruction(DecodedInstruction inst) const;
  const sc_event& coreFinishedEvent() const;
};

class ReadScratchpadHandler: public InstructionHandler {
public:
  ReadScratchpadHandler(sc_module_name name);
  void respondToInstruction(DecodedInstruction inst) const;
  const sc_event& coreFinishedEvent() const;
};

class WriteScratchpadHandler: public InstructionHandler {
public:
  WriteScratchpadHandler(sc_module_name name);
  void respondToInstruction(DecodedInstruction inst) const;
  const sc_event& coreFinishedEvent() const;
};

class ReadCRegsHandler: public InstructionHandler {
public:
  ReadCRegsHandler(sc_module_name name);
  void respondToInstruction(DecodedInstruction inst) const;
  const sc_event& coreFinishedEvent() const;
};

class WriteCRegsHandler: public InstructionHandler {
public:
  WriteCRegsHandler(sc_module_name name);
  void respondToInstruction(DecodedInstruction inst) const;
  const sc_event& coreFinishedEvent() const;
};

class ReadPredicateHandler: public InstructionHandler {
public:
  ReadPredicateHandler(sc_module_name name);
  void respondToInstruction(DecodedInstruction inst) const;
  const sc_event& coreFinishedEvent() const;
};

class WritePredicateHandler: public InstructionHandler {
public:
  WritePredicateHandler(sc_module_name name);
  void respondToInstruction(DecodedInstruction inst) const;
  const sc_event& coreFinishedEvent() const;
};

class NetworkSendHandler: public InstructionHandler {
public:
  NetworkSendHandler(sc_module_name name);
  void respondToInstruction(DecodedInstruction inst) const;
  const sc_event& coreFinishedEvent() const;
};

class WocheHandler: public InstructionHandler {
public:
  WocheHandler(sc_module_name name);
  void respondToInstruction(DecodedInstruction inst) const;
  const sc_event& coreFinishedEvent() const;
};

class SelchHandler: public InstructionHandler {
public:
  SelchHandler(sc_module_name name);
  void respondToInstruction(DecodedInstruction inst) const;
  const sc_event& coreFinishedEvent() const;
};

} // end namespace

#endif /* SRC_TILE_CORE_INSTRUCTIONHANDLER_H_ */
