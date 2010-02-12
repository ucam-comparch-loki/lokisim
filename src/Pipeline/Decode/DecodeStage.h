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

public:
/* Ports */
  sc_in<Word> in1, in2;
  sc_out<AddressedWord> out1;

  sc_in<Instruction> inst;
  sc_in<Word> regIn1, regIn2;
  sc_in<bool> cacheHit, roomToFetch, flowControl;
  sc_out<Address> address;
  sc_out<Instruction> remoteInst;
  sc_out<short> regReadAddr1, regReadAddr2, writeAddr, indWriteAddr;
  sc_out<short> remoteChannel, operation, op1Select, op2Select;
  sc_out<bool> isIndirect, newRChannel;
  sc_out<Data> chEnd1, chEnd2, regOut1, regOut2, sExtend;

/* Constructors and destructors */
  SC_HAS_PROCESS(DecodeStage);
  DecodeStage(sc_core::sc_module_name name, int ID);
  virtual ~DecodeStage();

private:
/* Methods */
  // Methods which act when an input is received
  void receivedFromRegs1();
  void receivedFromRegs2();

  virtual void newCycle();

/* Components */
  FetchLogic fl;
  ReceiveChannelEndTable rcet;
  Decoder decoder;
  SignExtend extend;

/* Signals (wires) */
  sc_signal<Instruction> instruction;
  sc_signal<Word> fromNetwork1, fromNetwork2;
  sc_signal<Address> decodeToFetch;
  sc_buffer<short> decodeToRCET1, decodeToRCET2;
  sc_signal<Data> decodeToExtend, baseAddress;

};

#endif /* DECODESTAGE_H_ */
