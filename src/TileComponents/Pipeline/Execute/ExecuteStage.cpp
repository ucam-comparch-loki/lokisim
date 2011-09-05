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
  if(waitingToFetch) {
    DecodedInst instruction = dataIn.read();
    bool success = fetch(instruction);

    if(success) {
      adjustNetworkAddress(instruction);
      dataOut.write(instruction);
      executedInstruction.notify();
      waitingToFetch = false;
      // TODO: instrumentation

      next_trigger(dataIn.default_event());
    }
    else{
      // Try again next cycle.
      next_trigger(clock.posedge_event());
    }
  }
  else if(dataIn.event()) {
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
    bool success;

    // Special cases for any instructions which don't use the ALU.
    switch(operation.operation()) {
      case InstructionMap::SETCHMAP:
//      case InstructionMap::SETCHMAPI:
        setChannelMap(operation);
        success = true;
        break;

      case InstructionMap::FETCH:
      case InstructionMap::FETCHPST:
      case InstructionMap::FILL:
      case InstructionMap::PSELFETCH:
        success = fetch(operation);
        break;

      default:
        success = alu.execute(operation);
        break;
    }

    currentInst.result(operation.result());

    // Memory operations may be sent to different memory banks depending on the
    // address accessed.
    // In practice, this would be performed by a separate, small functional
    // unit in parallel with the main ALU, so that there is time left to request
    // a path to memory.
    if(operation.isMemoryOperation()) adjustNetworkAddress(operation);

    executedInstruction.notify();

    if(success) dataOut.write(operation);
  }
  else {
    // If the instruction will not be executed, invalidate it so we don't
    // try to forward data from it.
    currentInst.destination(0);
    currentInst.setsPredicate(false);
  }
}

bool ExecuteStage::fetch(DecodedInst& inst) {
  MemoryAddr fetchAddress;

  // Compute the address to fetch from, depending on which operation this is.
  switch(inst.operation()) {
    case InstructionMap::FETCH:
    case InstructionMap::FETCHPST:
    case InstructionMap::FILL:
      fetchAddress = inst.operand1() + inst.operand2();
      break;

    case InstructionMap::PSELFETCH:
      fetchAddress = readPredicate() ? inst.operand1() : inst.operand2();
      break;

    default:
      cerr << "Error: called ExecuteStage::fetch with non-fetch instruction." << endl;
      assert(false);
  }


  // Return whether the fetch request should be sent (it should be sent if the
  // tag is not in the cache).
  if(!waitingToFetch && parent()->inCache(fetchAddress, inst.operation())) {
    return false;
  }
  else if(parent()->readyToFetch()) {
    // Generate a fetch request.
    MemoryRequest request(MemoryRequest::IPK_READ, fetchAddress);
    inst.result(request.toULong());
    return true;
  }
  else {
    waitingToFetch = true;
    // TODO: instrumentation
    return false;
  }
}

void ExecuteStage::setChannelMap(DecodedInst& inst) const {
  MapIndex entry = inst.immediate();
  uint32_t value = inst.operand1();

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

  // Generate a message to claim the port we have just stored the address of.
  if(sendChannel.isCore()) {
    ChannelID returnChannel(id, entry);
    inst.result(returnChannel.toInt());
    inst.channelMapEntry(entry);
    inst.networkDestination(sendChannel);
    inst.portClaim(true);
    inst.usesCredits(parent()->channelMapTable.getEntry(entry).usesCredits());
  }
}

void ExecuteStage::adjustNetworkAddress(DecodedInst& inst) const {
  assert(inst.isMemoryOperation());
  MemoryRequest::MemoryOperation op = (MemoryRequest::MemoryOperation)inst.memoryOp();

  Word w = MemoryRequest(op, inst.result());
  bool addressFlit = op == MemoryRequest::LOAD_B ||
                     op == MemoryRequest::LOAD_HW ||
                     op == MemoryRequest::LOAD_W ||
                     op == MemoryRequest::STORE_B ||
                     op == MemoryRequest::STORE_HW ||
                     op == MemoryRequest::STORE_W ||
                     op == MemoryRequest::IPK_READ;

  // Adjust destination channel based on memory configuration if necessary
  uint32_t increment = 0;

  // We want to access lots of information from the channel map table, so get
  // the entire entry.
  ChannelMapEntry& channelMapEntry = parent()->channelMapTable.getEntry(inst.channelMapEntry());

  if (channelMapEntry.localMemory() && channelMapEntry.memoryGroupBits() > 0) {
    if (addressFlit) {
      increment = (uint32_t)inst.result();
      increment &= (1UL << (channelMapEntry.memoryGroupBits() + channelMapEntry.memoryLineBits())) - 1UL;
      increment >>= channelMapEntry.memoryLineBits();

      // Store the adjustment which must be made, so that any subsequent flits
      // can also access the same memory.
      channelMapEntry.setAddressIncrement(increment);
    } else {
      increment = channelMapEntry.getAddressIncrement();
    }
  }

  // Set the instruction's result to the data + memory operation
  inst.result(w.toULong());

  ChannelID adjusted = inst.networkDestination();
  adjusted = adjusted.addPosition(increment);
  inst.networkDestination(adjusted);
}

bool ExecuteStage::isStalled() const {
  return waitingToFetch; // alu.isBusy(); if/when we have multi-cycle operations
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
