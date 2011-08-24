/*
 * ExecuteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "ExecuteStage.h"
#include "../../Cluster.h"

bool ExecuteStage::readPredicate() const {return parent()->readPredReg();}
int32_t ExecuteStage::readReg(RegisterIndex reg) const {return parent()->readReg(reg);}
int32_t ExecuteStage::readWord(MemoryAddr addr) const {return parent()->readWord(addr).toInt();}
int32_t ExecuteStage::readByte(MemoryAddr addr) const {return parent()->readByte(addr).toInt();}

void ExecuteStage::writePredicate(bool val) const {parent()->writePredReg(val);}
void ExecuteStage::writeReg(RegisterIndex reg, Word data) const {parent()->writeReg(reg, data.toInt());}
void ExecuteStage::writeWord(MemoryAddr addr, Word data) const {parent()->writeWord(addr, data);}
void ExecuteStage::writeByte(MemoryAddr addr, Word data) const {parent()->writeByte(addr, data);}

void ExecuteStage::execute() {
  if(dataIn.event()) {
    // Deal with the new input. We are currently not idle.
    idle.write(false);
    DecodedInst instruction = dataIn.read();
    newInput(instruction);

    next_trigger(clock.posedge_event());
  }
  else {
    // There is no instruction to execute - wait for one.
    idle.write(true);

    // Invalidate the current instruction so its data isn't forwarded anymore.
    currentInst.destination(0);
    currentInst.setsPredicate(false);

    next_trigger(dataIn.default_event());
  }
}

void ExecuteStage::updateReady() {
  bool ready = !isStalled();

  // Write our current stall status.
  if(ready != readyOut.read()) readyOut.write(ready);

  if(DEBUG && isStalled() && readyOut.read()) {
    cout << this->name() << " stalled." << endl;
  }

  // Wait until some point late in the cycle, so we know that any operations
  // will have completed.
  next_trigger(executedInstruction);
}

void ExecuteStage::newInput(DecodedInst& operation) {
  currentInst = operation;

  // Block forwarding of data if this is an indirect write - the result won't
  // actually get into the stated destination register.
  if(currentInst.operation() == InstructionMap::IWTR) currentInst.destination(0);

  // See if the instruction should execute.
  bool willExecute = checkPredicate(operation);

  if(operation.isALUOperation())
    Instrumentation::operation(id, operation, willExecute);

  if(willExecute) {
    if(operation.operation() == InstructionMap::SETCHMAP) {
      // TODO: SETCHMAPI
      setChannelMap(operation.immediate(), operation.operand1());
//      executedInstruction.notify();
//      return;
    }

    // Execute the instruction.
    bool success = alu.execute(operation);
    currentInst.result(operation.result());
    executedInstruction.notify();

    MapIndex channel = operation.channelMapEntry();
    if(channel != Instruction::NO_CHANNEL) {
      ChannelID destination = parent()->channelMapTable.read(channel);

      if(!destination.isNullMapping())
        operation.networkDestination(destination);
    }

    if(success) dataOut.write(operation);
  }
  else {
    // If the instruction will not be executed, invalidate it so we don't
    // try to forward data from it.
    currentInst.destination(0);
    currentInst.setsPredicate(false);
  }
}

void ExecuteStage::setChannelMap(MapIndex entry, uint32_t value) {
  // Extract all of the relevant sections from the encoded destination, and
  // create a ChannelID out of them.
  bool multicast = (value >> 28) & 0x1UL;
  // Hack: tile shouldn't be necessary for multicast addresses
  uint tile      = multicast ? id.getTile() : ((value >> 20) & 0xFFUL);
  uint position  = (value >> 12) & 0xFFUL;
  uint channel   = (value >> 8) & 0xFUL;
  uint groupBits = (value >> 4) & 0xFUL;
  uint lineBits  = value & 0xFUL;

  ChannelID sendChannel(tile, position, channel, multicast);

  // Write to the channel map table.
  parent()->channelMapTable.write(entry, sendChannel, groupBits, lineBits);
}

bool ExecuteStage::isStalled() const {
  return false; // alu.isBusy(); if/when we have multi-cycle operations
}

bool ExecuteStage::checkPredicate(DecodedInst& inst) {
  bool pred = readPredicate();
  short predBits = inst.predicate();

  bool result = (predBits == Instruction::ALWAYS) ||
                (predBits == Instruction::END_OF_PACKET) ||
                (predBits == Instruction::P     &&  pred) ||
                (predBits == Instruction::NOT_P && !pred);

  return result;
}

const DecodedInst& ExecuteStage::currentInstruction() const {
  return currentInst;
}

const sc_event& ExecuteStage::executedEvent() const {
  return executedInstruction;
}

ExecuteStage::ExecuteStage(sc_module_name name, const ComponentID& ID) :
    PipelineStage(name, ID),
    alu("alu", ID) {

  SC_METHOD(execute);
  // do initialise

}
