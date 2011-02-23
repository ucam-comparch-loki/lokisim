/*
 * InstructionMap.h
 *
 * Class containing mappings between different representations of instruction
 * opcodes: the name of the instruction, the opcode, and a general integer used
 * by the simulator. Having this level of abstraction allows us to easily
 * modify the subset of instructions used, or the coding of instructions.
 *
 * Name = string representation
 * Opcode = intermediate representation stored in Instruction datatype
 * Instruction = select value sent to ALU
 *
 *  Created on: 18 Jan 2010
 *      Author: db434
 */

#ifndef INSTRUCTIONMAP_H_
#define INSTRUCTIONMAP_H_

#include <map>
#include <string>

class InstructionMap {

public:

  // Return whether the given operation should specify a particular field.
  static bool hasDestReg(short operation);
  static bool hasSrcReg1(short operation);
  static bool hasSrcReg2(short operation);
  static bool hasImmediate(short operation);
  static bool hasRemoteChannel(short operation);

  // Returns whether the given operation uses the ALU.
  static bool isALUOperation(short operation);

  // Returns whether the operation stores its result in a register.
  static bool storesResult(short operation);

  // Returns the opcode of the given operation name.
  static short opcode(const std::string& name);

  // Returns the decoded operation value from the given opcode.
  static short operation(short opcode);

  // Returns the name of the given instruction.
  static std::string& name(int operation);

  enum Operation {

    NOP,          // No operation (use or r0 r0 r0?)      nop

    LDW,          // Load word                            ldw rs, immed -> rch
    LDHWU,        // Load halfword (zero-extended)        ldhwu rs, immed -> rch
    LDBU,         // Load byte (zero-extended)            ldbu rs, immed -> rch
    STW,          // Store word                           stw rs, rt, immed -> rch
    STHW,         // Store halfword                       sthw rs, rt, immed -> rch
    STB,          // Store byte                           stb rs, rt, immed -> rch
    STWADDR,      // Store address                        stwaddr rt, immed -> rch
    STBADDR,      // Store byte address                   stbaddr rt, immed -> rch

    LDVECTOR,     // Load data to all SIMD cores
    LDSTREAM,     // Load stream of data to this core
    STVECTOR,     // Store data from all SIMD cores
    STSTREAM,     // Store stream of data from this core

    SLL,          // Shift left logical variable          sll rd, rs, rt
    SRL,          // Shift right logical variable         srl rd, rs, rt
    SRA,          // Shift right arithmetic variable      sra rd, rs, rt
    SLLI,         // Shift left logical                   slli rd, rs, immed
    SRLI,         // Shift right logical                  srli rd, rs, immed
    SRAI,         // Shift right arithmetic               srai rd, rs, immed

    SETEQ,        // Set if equal                         seteq rd, rs, rt
    SETNE,        // Set if not equal                     setne rd, rs, rt
    SETLT,        // Set if less than                     setlt rd, rs, rt
    SETLTU,       // Set if less than unsigned            setltu rd, rs, rt
    SETGTE,       // Set if greater than or equal         setgte rd, rs, rt
    SETGTEU,      // Set if >= unsigned                   setgteu rd, rs, rt
    SETEQI,       // Set if equal immediate               seteqi rd, rs, immed
    SETNEI,       // Set if not equal immediate           setnei rd, rs, immed
    SETLTI,       // Set if less than immediate           setlti rd, rs, immed
    SETLTUI,      // Set if less than unsigned immediate  setltiu rd, rs, immed
    SETGTEI,      // Set if >= immediate                  setgtei rd, rs, immed
    SETGTEUI,     // Set if >= unsigned immediate         setgteiu rd, rs, immed

    LUI,          // Load upper immediate                 lui rd, immed

    PSEL,         // Predicated selection (rd = p?rs:rt)  psel rd, rs, rt

    CLZ,          // Count leading zeroes                 clz rd, rs

    NOR,          // Nor                                  nor rd, rs, rt
    AND,          // And                                  and rd, rs, rt
    OR,           // Or                                   or rd, rs, rt
    XOR,          // Xor                                  xor rd, rs, rt
    NORI,         // Nor immediate                        nori rd, rs, immed
    ANDI,         // And immediate                        andi rd, rs, immed
    ORI,          // Or immediate                         ori rd, rs, immed
    XORI,         // Xor immediate                        xori rd, rs, immed

    NAND,         // Nand                                 nand rd, rs, rt
    CLR,          // Bitwise clear (rd = rs & ~rt)        clr rd, rs, rt
    ORC,          // Or complement (rd = rs | ~rt)        orc rd, rs, rt
    POPC,         // Population count (number of 1s)      popc rd, rs
    RSUBI,        // Reverse subtract immed (immed-rs)    rsubi rd, rs, immed

    ADDU,         // Add unsigned                         addu rd, rs, rt
    ADDUI,        // Add unsigned immediate               addui rd, rs, immed
    SUBU,         // Subtract unsigned                    subu rd, rs, rt
    MULHW,        // Multiply high word                   mulhw rd, rs, rt
    MULLW,        // Multiply low word                    mullw rd, rs, rt
    MULHWU,       // Multiply high word unsigned          mulhwu rd, rs, rt

    IRDR,         // Indirect read register               irdr rd, *rs
    IWTR,         // Indirect write register              iwtr *rd, rs

    WOCHE,        // Wait until output channel empty      woche ch
    TSTCH,        // Test channel (see if holding data)   tstch rd, ch
    SELCH,        // Select channel (with data)           selch rd

    SETFETCHCH,   // Set fetch channel (ch to fetch from) setfetchch ch
    IBJMP,        // In buffer jump                       ibjmp immed
    FETCH,        // Fetch instruction packet             fetch rs, immed
    PSELFETCH,    // Fetch dependent on predicate         psel.fetch rs rt
    FETCHPST,     // Fetch persistent (repeat execution)  fetchpst rs, immed
    RMTFETCH,     // "Fetch" to remote cluster            rmtfetch rs, immed -> rch
    RMTFETCHPST,  // "Fetch persistent" to remote cluster rmtfetchpst rs, immed -> rch
    RMTFILL,      // Remote fill (no auto execution)      rmtfill rs, immed -> rch
    RMTEXECUTE,   // Remote execute (specify target)      rmtexecute -> rch
    RMTNXIPK,     // "Next IPK" to remote cluster         rmtnxipk -> rch

    SETCHMAP,     // Set channel map entry                setchmap immed, rs

    SYSCALL       // System call                          syscall immed

  };

private:
  static std::map<std::string, short> nto; // name to opcode
  static std::map<short, int> oti; // opcode to instruction
  static std::map<int, std::string> itn; // instruction to name

  // Fill up the maps with the correct values.
  static void initialise();

  // Add a single instruction to all maps.
  static void addToMaps(const std::string& name, short opcode, int instruction);

};

#endif /* INSTRUCTIONMAP_H_ */
