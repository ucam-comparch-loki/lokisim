/*
 * DecodeStage.h
 *
 * Decode stage of the pipeline. Responsible for extracting useful information
 * from instructions and preparing it for use by the ALU, collecting data
 * inputs from the network, and sending memory requests to the network.
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#ifndef DECODESTAGE_H_
#define DECODESTAGE_H_

#include "../PipelineStage.h"
#include "FetchLogic.h"
#include "ReceiveChannelEndTable.h"
#include "Decoder.h"
#include "SignExtend.h"

class DecodeStage: public PipelineStage {

//==============================//
// Ports
//==============================//

public:

  // The NUM_RECEIVE_CHANNELS inputs to the receive channel-end table.
  sc_in<Word>          *in;

  // A flow control signal for each input (NUM_RECEIVE_CHANNELS).
  sc_out<bool>         *flowControlOut;

  // An output used to send FETCH requests.
  sc_out<AddressedWord> out1;

  // A flow control signal for the output.
  sc_in<bool>           flowControlIn;

  // The instruction to decode.
  sc_in<Instruction>    instructionIn;

  // Status signals from the instruction packet cache.
  sc_in<bool>           cacheHit, roomToFetch;

  // The address to lookup in the instruction packet cache.
  sc_out<Address>       address;

  // The offset to jump by in the instruction packet cache.
  sc_out<short>         jumpOffset;

  // The instruction to send to a remote cluster.
  sc_out<Instruction>   remoteInst;

  // The two registers to read data from.
  sc_out<short>         regReadAddr1, regReadAddr2;

  // Signal whether or not the second read address is indirect.
  sc_out<bool>          isIndirect;

  // The channel that the indirect register is pointing to.
  sc_in<short>          indirectChannel;

  // Data read from the register file.
  sc_in<Word>           regIn1, regIn2;

  // The register to write the result of the instruction to.
  sc_out<short>         writeAddr;

  // The indirect register to write the result of the instruction to.
  sc_out<short>         indWriteAddr;

  // The memory operation being performed.
  sc_out<short>         memoryOp;

  // The remote channel to send the data or instruction to.
  sc_out<short>         remoteChannel;

  // Stall the pipeline until this output channel is empty.
  sc_out<short>         waitOnChannel;

  // Two data values from the receive channel-end table.
  sc_out<Data>          chEnd1, chEnd2;

  // Two data values from the register file.
  sc_out<Data>          regOut1, regOut2;

  // A data value from the sign extender.
  sc_out<Data>          sExtend;

  // The operation the ALU should carry out.
  sc_out<short>         operation;

  // Choose where the ALU's input operands should come from.
  sc_out<short>         op1Select, op2Select;

  // The value of the predicate register.
  sc_in<bool>           predicate;

  // Tell the ALU whether it should use the result of this computation to
  // set the predicate register.
  sc_out<bool>          setPredicate;

  // Signal that the pipeline should stall.
  sc_out<bool>          stallOut;

  // A multi-cycle instruction requires that most of the pipeline keeps
  // working, but that no new instructions are supplied.
  sc_out<bool>          stallFetch;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(DecodeStage);
  DecodeStage(sc_module_name name, int ID);
  virtual ~DecodeStage();

//==============================//
// Methods
//==============================//

private:

  // Methods which act when an input is received
  void receivedFromRegs1();
  void receivedFromRegs2();

  void updateStall();
  void updateToRCET1();
  void updateOp1Select();

  virtual void newCycle();

//==============================//
// Components
//==============================//

private:

  FetchLogic              fl;
  ReceiveChannelEndTable  rcet;
  Decoder                 decoder;
  SignExtend              extend;

//==============================//
// Signals (wires)
//==============================//

private:

  sc_buffer<Instruction>  instructionSig;
  sc_buffer<Word>        *fromNetwork;
  sc_buffer<Address>      decodeToFetch;
  sc_buffer<short>        decodeToRCET1, decodeToRCET2, RCETread1;
  flag_signal<short>      RCETOperation;
  sc_buffer<short>        op1SelectSig;
  sc_buffer<Data>         decodeToExtend, baseAddress;
  sc_signal<bool>         rcetStall, flStall;

};

#endif /* DECODESTAGE_H_ */
