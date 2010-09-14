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
//   idle

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Cluster);
  Cluster(sc_module_name name, uint16_t ID);
  virtual ~Cluster();

//==============================//
// Methods
//==============================//

public:

  virtual double   area() const;
  virtual double   energy() const;

  // Initialise the instructions a Cluster will execute.
  virtual void     storeData(std::vector<Word>& data);

  virtual int32_t  getRegVal(uint8_t reg, bool indirect = false) const;
  void             writeReg(uint8_t reg, int32_t value, bool indirect = false);
  virtual uint16_t getInstIndex() const;
  virtual bool     getPredReg() const;

  // Returns the channel ID of the specified cluster's instruction packet FIFO.
  static uint32_t  IPKFIFOInput(uint16_t ID);

  // Returns the channel ID of the specified cluster's instruction packet cache.
  static uint32_t  IPKCacheInput(uint16_t ID);

  // Returns the channel ID of the specified cluster's input channel.
  static uint32_t  RCETInput(uint16_t ID, uint8_t channel);

  void             updateCurrentPacket(Address addr);

private:

  void             stallPipeline();
  void             updateIdle();

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
  sc_signal<bool>          fetchIdle, decodeIdle, executeIdle, writeIdle;

  // To/from fetch stage
  flag_signal<Instruction> nextInst;
  sc_signal<bool>          fetchStallSig;

  // To/from decode stage
  sc_signal<bool>          decStallSig, stallFetchSig;

  // To/from execute stage

  // To/from write stage
  sc_signal<bool>          writeStallSig;

};

#endif /* CLUSTER_H_ */
