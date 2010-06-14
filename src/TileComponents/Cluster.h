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
#include "Pipeline/PredicateRegister.h"
#include "Pipeline/Fetch/FetchStage.h"
#include "Pipeline/Decode/DecodeStage.h"
#include "Pipeline/Execute/ExecuteStage.h"
#include "Pipeline/Write/WriteStage.h"

class Cluster : public TileComponent {

//==============================//
// Ports
//==============================//

// Inherited from TileComponent:
//   clock
//   in
//   out
//   flowControlIn
//   flowControlOut

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Cluster);
  Cluster(sc_module_name name, int ID);
  virtual ~Cluster();

//==============================//
// Methods
//==============================//

public:

  virtual double area() const;
  virtual double energy() const;

  // Initialise the instructions a Cluster will execute.
  virtual void storeData(std::vector<Word>& data);

  // Returns the channel ID of the specified cluster's instruction packet FIFO.
  static int   IPKFIFOInput(int ID);

  // Returns the channel ID of the specified cluster's instruction packet cache.
  static int   IPKCacheInput(int ID);

  // Returns the channel ID of the specified cluster's input channel.
  static int   RCETInput(int ID, int channel);

private:

  void         stallPipeline();
  void         updateCurrentPacket();

//==============================//
// Components
//==============================//

private:

  IndirectRegisterFile     regs;
  PredicateRegister        pred;
  FetchStage               fetch;
  DecodeStage              decode;
  ExecuteStage             execute;
  WriteStage               write;

//==============================//
// Signals (wires)
//==============================//

private:

  sc_signal<bool>          stallSig;

  // To/from fetch stage
  sc_buffer<Address>       FLtoIPKC, currentIPKSig;
  sc_buffer<short>         jumpOffsetSig;
  flag_signal<Instruction> nextInst;
  sc_signal<bool>          fetchStallSig;

  // To/from decode stage
  sc_buffer<bool>          cacheHitSig, indirectReadSig;
  sc_signal<bool>          roomToFetch, refetchSig, persistent;
  sc_buffer<Word>          regData1, regData2;
  flag_signal<short>       regRead1, regRead2, decWriteAddr, decIndWrite;
  sc_buffer<short>         indChannelSig;
  sc_buffer<Data>          RCETtoALU1, RCETtoALU2, regToALU1, regToALU2, SEtoALU;
  flag_signal<short>       operation, op1Select, op2Select;
  sc_signal<bool>          setPredSig, readPredSig, decStallSig, stallFetchSig;

  // To/from execute stage
  flag_signal<Instruction> decToExInst,  exToWriteInst;
  flag_signal<short>       decToExRChan, exToWriteRChan;
  flag_signal<short>       decToExMemOp, exToWriteMemOp;
  flag_signal<short>       decToExWOCHE, exToWriteWOCHE;
  flag_signal<Data>        ALUOutput;
  sc_signal<bool>          writePredSig;

  // To/from write stage
  flag_signal<short>       writeAddr, indWriteAddr;
  sc_buffer<short>         writeRegAddr, indirectWrite;
  sc_buffer<Word>          regWriteData;
  sc_signal<bool>          writeStallSig;

};

#endif /* CLUSTER_H_ */
