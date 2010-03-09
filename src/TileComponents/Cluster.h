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

#include "Pipeline/IndirectRegisterFile.h"
#include "Pipeline/Fetch/FetchStage.h"
#include "Pipeline/Decode/DecodeStage.h"
#include "Pipeline/Execute/ExecuteStage.h"
#include "Pipeline/Write/WriteStage.h"

class Cluster : public TileComponent {

public:
/* Constructors and destructors */
  Cluster(sc_module_name name, int ID);
  virtual ~Cluster();

/* Methods */
  virtual void storeData(std::vector<Word>& data);
  static int   IPKFIFOInput(int ID);
  static int   IPKCacheInput(int ID);
  static int   RCETInput(int ID, int channel);

private:

/* Components */
  IndirectRegisterFile     regs;
  FetchStage               fetch;
  DecodeStage              decode;
  ExecuteStage             execute;
  WriteStage               write;

/* Signals (wires) */
  // To/from fetch stage
  sc_buffer<Address>       FLtoIPKC;
  flag_signal<Instruction> nextInst;

  // To/from decode stage
  sc_buffer<bool>          cacheHitSig, roomToFetch, indirectReadSig;
  sc_buffer<Word>          regData1, regData2;
  flag_signal<short>       regRead1, regRead2, decWriteAddr, decIndWrite, predicate;
  sc_buffer<Data>          RCETtoALU1, RCETtoALU2, regToALU1, regToALU2, SEtoALU;
  flag_signal<short>       operation, op1Select, op2Select;
  sc_signal<bool>          setPredSig;

  // To/from execute stage
  flag_signal<Instruction> decToExInst, exToWriteInst;
  flag_signal<short>       decToExRChan, exToWriteRChan;
  flag_signal<Data>        ALUOutput;

  // To/from write stage
  flag_signal<short>       writeAddr, indWriteAddr;
  sc_buffer<short>         writeRegAddr, indirectWrite;
  sc_buffer<Word>          regWriteData;

};

#endif /* CLUSTER_H_ */
