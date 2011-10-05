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
  switch(state) {
    case NO_INSTRUCTION: {
      assert(dataIn.event());

      idle.write(false);
      DecodedInst instruction = dataIn.read();
      state = EXECUTING;
      newInput(instruction);

      break;
    }

    case EXECUTING:
      if(clock.negedge()) {
        // FIXME
        // Quick hack: send arbitration requests slightly after the clock edge.
        // This is because the memory receives data on the clock edge, and may
        // discover that its buffers are now full and it can't take any more.
        // Would prefer to have the memory put data in its buffer as soon as it
        // arrives, rather than on the clock edge.
        next_trigger(0.1, sc_core::SC_NS);
        return;
      }

      assert(currentInst.sendsOnNetwork());

      // Memory operations may be sent to different memory banks depending on the
      // address accessed.
      // In practice, this would be performed by a separate, small functional
      // unit in parallel with the main ALU, so that there is time left to request
      // a path to memory.
      if(currentInst.isMemoryOperation()) adjustNetworkAddress(currentInst);
      requestArbitration(currentInst.networkDestination(), true);

      break;

    case ARBITRATING: {

      // Have just received a grant from the arbiter (if one was needed).
      dataOut.write(currentInst);
      executedInstruction.notify();

      state = FINISHED;
      next_trigger(clock.posedge_event());

      break;
    }

    case WAITING_TO_FETCH: {
      DecodedInst instruction = dataIn.read();
      bool sendFetch = fetch(instruction);

      if(sendFetch) {
        currentInst.result(instruction.result());
        adjustNetworkAddress(currentInst);
        executedInstruction.notify();

        // Apply to send the fetch onto the network if the cache is now ready.
        requestArbitration(currentInst.networkDestination(), true);

        // TODO: tell instrumentation that we are no longer stalled
      }
      // Wait until negative edge because that's when arbitration requests must
      // be sent.
      else next_trigger(clock.negedge_event());

      break;
    }

    case FINISHED: {
      // Have now finished executing, so this pipeline stage is idle.
      idle.write(true);

      // Take down the request if this is the last flit of the packet.
      if(currentInst.sendsOnNetwork() && currentInst.endOfNetworkPacket()) {
        requestArbitration(currentInst.networkDestination(), false);
      }

      // Invalidate the current instruction so its data isn't forwarded anymore.
      currentInst.preventForwarding();

      state = NO_INSTRUCTION;
      next_trigger(dataIn.default_event());
      break;
    }
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
  // See if the instruction should execute.
  bool willExecute = checkPredicate(operation);

  if(operation.isALUOperation())
    Instrumentation::operation(id, operation, willExecute);

  if(willExecute) {
    bool success = true;

    if(DEBUG) cout << this->name() << ": executing " << operation.name()
        << " on " << operation.operand1() << " and " << operation.operand2() << endl;

    // Special cases for any instructions which don't use the ALU.
    switch(operation.opcode()) {
      case InstructionMap::OP_SETCHMAP:
      case InstructionMap::OP_SETCHMAPI:
        setChannelMap(operation);
        break;

      case InstructionMap::OP_FETCH:
      case InstructionMap::OP_FETCHR:
      case InstructionMap::OP_FETCHPST:
      case InstructionMap::OP_FETCHPSTR:
      case InstructionMap::OP_FILL:
      case InstructionMap::OP_FILLR:
      case InstructionMap::OP_PSEL_FETCH:
        success = fetch(operation);
        break;

      case InstructionMap::OP_LLI:
        // FIXME: should LLI happen in execute stage or during register write?
        operation.result((operation.operand1() & 0xFFFF0000) | operation.operand2());
        break;

      case InstructionMap::OP_LUI:
        operation.result(operation.operand2() << 16);
        break;

      case InstructionMap::OP_SYSCALL:
        alu.systemCall(operation.immediate());
        break;

//      case InstructionMap::OP_WOCHE ?

      default:
        if(InstructionMap::isALUOperation(operation.opcode()))
          alu.execute(operation);
        break;
    }

    // Store the instruction now that it has a result which can be forwarded.
    // This may need to change if ALU operations can ever take more than 1 cycle.
    currentInst = operation;

    executedInstruction.notify();

    if(success) {
      if(currentInst.sendsOnNetwork()) {
        // Will need to request network resources - wait some time to
        // simulate the computation of which resources are needed.
        next_trigger(clock.negedge_event());
      }
      else {
        state = ARBITRATING;
        next_trigger(sc_core::SC_ZERO_TIME);
      }
    }
    // success can only be false if we can't send a fetch - wait a cycle
    else next_trigger(clock.negedge_event());
  } // if will execute
  else {
    // If the instruction will not be executed, invalidate it so we don't
    // try to forward data from it.
    currentInst.preventForwarding();

    state = FINISHED;
    next_trigger(clock.posedge_event());
  }
}

bool ExecuteStage::fetch(DecodedInst& inst) {
  MemoryAddr fetchAddress;

  // Compute the address to fetch from, depending on which operation this is.
  switch(inst.opcode()) {
    case InstructionMap::OP_FETCH:
    case InstructionMap::OP_FETCHR:
    case InstructionMap::OP_FETCHPST:
    case InstructionMap::OP_FETCHPSTR:
    case InstructionMap::OP_FILL:
    case InstructionMap::OP_FILLR:
      fetchAddress = inst.operand1() + inst.operand2();
      break;

    case InstructionMap::OP_PSEL_FETCH:
      fetchAddress = readPredicate() ? inst.operand1() : inst.operand2();
      break;

    default:
      cerr << "Error: called ExecuteStage::fetch with non-fetch instruction." << endl;
      assert(false);
  }

  // Return whether the fetch request should be sent (it should be sent if the
  // tag is not in the cache).
  if(state != WAITING_TO_FETCH && parent()->inCache(fetchAddress, inst.opcode())) {
    // Packet is already in the cache: don't fetch.
    state = FINISHED;
    return false;
  }
  else if(parent()->readyToFetch()) {
    // Packet is not in the cache, and there is space for the new packet: fetch.
    MemoryRequest request(MemoryRequest::IPK_READ, fetchAddress);
    inst.result(request.toULong());
    return true;
  }
  else {
    // Packet is not in the cache, but there is not space for the new packet:
    // don't fetch yet.
    state = WAITING_TO_FETCH;
    // TODO: instrumentation
    return false;
  }
}

void ExecuteStage::requestArbitration(ChannelID destination, bool request) {
  if(request) {
    state = ARBITRATING;

    // Call execute() again when the request is granted.
    next_trigger(parent()->requestArbitration(destination, request));
  }
  else {
    // We are not sending a request which will be granted, so don't use
    // next_trigger this time.
    parent()->requestArbitration(destination, request);
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
  return state == WAITING_TO_FETCH || state == EXECUTING || state == ARBITRATING;
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

  // Initial state is FINISHED, so that when execute() is first called, it
  // tidies/initialises everything correctly and waits for the first instruction.
  state = FINISHED;

  SC_METHOD(execute);
  // do initialise

}
