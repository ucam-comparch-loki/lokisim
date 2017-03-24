/*
 * DecodedInst.h
 *
 * A container for all information an instruction may acquire as it passes
 * through the pipeline.
 *
 *  Created on: 11 Sep 2010
 *      Author: daniel
 */

#ifndef DECODEDINST_H_
#define DECODEDINST_H_

#include <inttypes.h>
#include "systemc.h"
#include "Identifier.h"
#include "Instruction.h"
#include "../Network/NetworkTypedefs.h"
#include "../Tile/Core/Fetch/InstructionStore.h"

class DecodedInst {

public:

  // All possible places an operand could come from. Could be used to drive
  // select signals for multiplexers at ALU inputs.
  enum OperandSource {
    NONE,       // This instruction does not have this operand
    REGISTER,   // The operand comes from a register
    CHANNEL,    // The operand is to be read from a channel-end
    IMMEDIATE,  // The operand is an immediate, encoded in the instruction
    BYPASS      // The operand needs to be forwarded from the previous instruction
  };

//============================================================================//
// Methods
//============================================================================//

public:

  opcode_t      opcode() const;
  function_t    function() const;
  format_t      format() const;
  RegisterIndex sourceReg1() const;
  RegisterIndex sourceReg2() const;
  RegisterIndex destination() const;
  int32_t       immediate() const;
  int32_t       immediate2() const;       // Just for psel.fetchr
  ChannelIndex  channelMapEntry() const;
  predicate_t   predicate() const;
  bool          setsPredicate() const;
  MemoryOpcode  memoryOp() const;

  OperandSource operand1Source() const;
  OperandSource operand2Source() const;
  bool          hasOperand1() const;
  bool          hasOperand2() const;

  int32_t       operand1() const;
  int32_t       operand2() const;
  int64_t       result() const;
  MemoryAddr    location() const;
  InstructionSource source() const;

  EncodedCMTEntry cmtEntry() const;
  const ChannelID networkDestination() const;

  bool          predicated() const;
  bool          hasResult() const;

  bool          hasDestReg() const;
  bool          hasSrcReg1() const;
  bool          hasSrcReg2() const;
  bool          hasImmediate() const;
  bool          isDecodeStageOperation() const;
  bool          isExecuteStageOperation() const;
  bool          endOfIPK() const;
  bool          persistent() const;
  bool          endOfNetworkPacket() const;
  bool          forRemoteExecution() const;
  const inst_name_t&  name() const;

  void    opcode(const opcode_t val);
  void    function(const function_t val);
  void    sourceReg1(const RegisterIndex val);
  void    sourceReg2(const RegisterIndex val);
  void    destination(const RegisterIndex val);
  void    immediate(const int32_t val);
  void    channelMapEntry(const ChannelIndex val);
  void    predicate(const predicate_t val);
  void    setsPredicate(const bool val);
  void    memoryOp(const MemoryOpcode val);

  void    operand1Source(const OperandSource src);
  void    operand2Source(const OperandSource src);

  void    operand1(const int32_t val);
  void    operand2(const int32_t val);
  void    result(const int64_t val);
  void    location(const MemoryAddr val);
  void    source(const InstructionSource val);

  void    persistent(const bool val);
  void    remoteExecute(const bool val);
  void    endOfNetworkPacket(const bool val);

  void    cmtEntry(const EncodedCMTEntry val);
  void    portClaim(const bool val);

  Instruction toInstruction() const;

  bool    sendsOnNetwork() const;
  bool    storesToRegister() const;

  // Provide this tile's ID so local communications can fill in their
  // destination addresses fully.
  const NetworkData toNetworkData(TileID tile) const;

  // Invalidate this instruction in such a way that it will never be able to
  // forward its result to another instruction. This may be useful if the
  // instruction will not be executed, or is more than one cycle old.
  void    preventForwarding();

  friend void sc_trace(sc_core::sc_trace_file*& tf, const DecodedInst& i, const std::string& txt) {
    sc_core::sc_trace(tf, i.opcode_,          txt + ".opcode");
    sc_core::sc_trace(tf, i.destReg_,         txt + ".rd");
    sc_core::sc_trace(tf, i.sourceReg1_,      txt + ".rs");
    sc_core::sc_trace(tf, i.sourceReg2_,      txt + ".rt");
    sc_core::sc_trace(tf, i.immediate_,       txt + ".immediate");
    sc_core::sc_trace(tf, i.channelMapEntry_, txt + ".channel");
    sc_core::sc_trace(tf, i.predicate_,       txt + ".predicate");

    sc_core::sc_trace(tf, i.operand1_,        txt + ".operand1");
    sc_core::sc_trace(tf, i.operand2_,        txt + ".operand2");
    sc_core::sc_trace(tf, i.result_,          txt + ".result");
  }

  bool operator== (const DecodedInst& other) const;

  DecodedInst& operator= (const DecodedInst& other);

  friend std::ostream& operator<< (std::ostream& os, DecodedInst const& v) {
    return v.print(os);
  }

private:

  // Sign extend a value of type T which is represented using B bits.
  // Set T to signed/unsigned to choose sign-extension technique.
  template <typename T, unsigned B>
  inline T signextend(const T x) {
    struct {T x:B;} s;
    return s.x = x;
  }

  // Holds implementation of the << operator, so it doesn't have to go in the
  // header.
  std::ostream& print(std::ostream& os) const;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  DecodedInst();
  DecodedInst(const Instruction i);

private:

  void init();

//============================================================================//
// Local state
//============================================================================//

private:
  // The original, encoded instruction.
  const Instruction   original_;

  opcode_t            opcode_;
  function_t          function_;
  format_t            format_;
  RegisterIndex       destReg_;
  RegisterIndex       sourceReg1_;
  RegisterIndex       sourceReg2_;
  int32_t             immediate_;
  ChannelIndex        channelMapEntry_;
  predicate_t         predicate_;
  bool                setsPred_;
  MemoryOpcode        memoryOp_;

  OperandSource       op1Source_;
  OperandSource       op2Source_;

  int32_t             operand1_;
  int32_t             operand2_;
  int64_t             result_;    // TODO: reduce to 32 bits if safe to do so

  // A snapshot of the channel map table entry when the instruction passes
  // through the Decode stage.
  EncodedCMTEntry     networkInfo;
  bool                portClaim_;
  bool                endOfPacket_;

  MemoryAddr          location_;  // The position in memory that this instruction comes from.
  InstructionSource   source_;    // Was the instruction from L0 cache or FIFO?

  bool                persistent_;
  bool                forRemoteExecution_;

  // Use to determine whether the result has already been set.
  // Can't just use != 0 because it may have been set to 0.
  bool                hasResult_;
};

#endif /* DECODEDINST_H_ */
