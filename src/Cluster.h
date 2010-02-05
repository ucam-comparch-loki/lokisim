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

/* Components */
  IndirectRegisterFile regs;
  FetchStage fetch;
  DecodeStage decode;
  ExecuteStage execute;
  WriteStage write;

/* Signals (wires) */
  // Clock
//  sc_signal<bool> fetchClock, decodeClock, executeClock, writeClock;

  // Main inputs
  sc_signal<Word> in1toIPKQ, in2toIPKC, in3toRCET, in4toRCET;

  // To/from fetch stage
  sc_signal<Address> FLtoIPKC;
  sc_signal<Instruction> nextInst, sendInst;

  // To/from decode stage
  sc_signal<bool> cacheHitSig, indirectReadSig;
  sc_signal<Word> regData1, regData2;
  sc_signal<short> regRead1, regRead2, decWriteAddr, decIndWrite;
  sc_signal<Data> RCETtoALU1, RCETtoALU2, regToALU1, regToALU2, SEtoALU;
  sc_signal<short> operation, op1Select, op2Select;

  // To/from execute stage
  sc_signal<Instruction> decodeToExecute, executeToWrite;
  sc_signal<Data> ALUtoALU1, ALUtoALU2, ALUtoWrite;

  // To/from write stage
  sc_signal<short> writeAddr, indWriteAddr;
  sc_signal<short> writeRegAddr, indirectWrite;
  sc_signal<Word> regWriteData;

/* Methods */
  void combineOutputs();

public:
/* Constructors and destructors */
  SC_HAS_PROCESS(Cluster);    // Removes need for SC_CTOR, allowing more arguments
  Cluster(sc_core::sc_module_name name, int ID);

  virtual ~Cluster();
};

#endif /* CLUSTER_H_ */
