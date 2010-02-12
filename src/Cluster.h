/*
 * Cluster.h
 *
 * A class representing an individual processing core. All work is done in the
 * subcomponents, and this class just serves to hold them all in one place and
 * connect them together correctly.
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef CLUSTER_H_
#define CLUSTER_H_

#include "TileComponent.h"

#include "Memory/IndirectRegisterFile.h"
#include "Pipeline/Fetch/FetchStage.h"
#include "Pipeline/Decode/DecodeStage.h"
#include "Pipeline/Execute/ExecuteStage.h"
#include "Pipeline/Write/WriteStage.h"

class Cluster : public TileComponent {

public:
/* Constructors and destructors */
  SC_HAS_PROCESS(Cluster);    // Removes need for SC_CTOR, allowing more arguments
  Cluster(sc_core::sc_module_name name, int ID);

  virtual ~Cluster();

private:
/* Methods */
  void splitInputs();
  void combineOutputs();

/* Components */
  IndirectRegisterFile regs;
  FetchStage fetch;
  DecodeStage decode;
  ExecuteStage execute;
  WriteStage write;

/* Signals (wires) */
  // Main inputs/outputs
  sc_signal<Word> in1toIPKQ, in2toIPKC, in3toRCET, in4toRCET;
  sc_signal<AddressedWord> decodeOutput;
  sc_signal<Array<AddressedWord> > writeOut;

  // To/from fetch stage
  sc_signal<Address> FLtoIPKC;
  sc_signal<Instruction> nextInst, sendInst;

  // To/from decode stage
  sc_signal<bool> cacheHitSig, roomToFetch, indirectReadSig;
  sc_buffer<bool> decFlowControl;
  sc_signal<Word> regData1, regData2;
  sc_signal<short> regRead1, regRead2, decWriteAddr, decIndWrite, predicate;
  sc_signal<Data> RCETtoALU1, RCETtoALU2, regToALU1, regToALU2, SEtoALU;
  sc_buffer<short> operation, op1Select, op2Select;

  // To/from execute stage
  sc_buffer<Instruction> decToExInst, exToWriteInst;
  sc_signal<short> decToExRChan, exToWriteRChan;
  sc_signal<bool> d2eNewRChan, e2wNewRChan;
  sc_buffer<Data> ALUOutput;

  // To/from write stage
  sc_signal<short> writeAddr, indWriteAddr;
  sc_signal<short> writeRegAddr, indirectWrite;
  sc_signal<Word> regWriteData;
  sc_buffer<Array<bool> > writeFlowControl;

};

#endif /* CLUSTER_H_ */
