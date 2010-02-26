/*
 * InstructionMap.h
 *
 * Class containing mappings between different representations of instruction
 * opcodes: the name of the instruction, the opcode, and a general integer used
 * by the simulator. Having this level of abstraction allows us to easily
 * modify the subset of instructions used, or the coding of instructions.
 *
 *  Created on: 18 Jan 2010
 *      Author: db434
 */

#ifndef INSTRUCTIONMAP_H_
#define INSTRUCTIONMAP_H_

#include <map>
#include <string>

class InstructionMap {

  static std::map<short, int> oti; // opcode to instruction
  static std::map<std::string, short> nto; // name to opcode

  static void initialise();

public:

  static bool hasImmediate(short operation);
  static bool hasRemoteChannel(short operation);
  static bool isALUOperation(short operation);

  static short operation(short opcode);
  static short opcode(std::string& name);

  enum Instructions {

    NOP,          // No operation                         nop

    LD,           // Load word                            ld rs, immed -> rch
    LDB,          // Load byte                            ldb rs, immed -> rch
    ST,           // Store word                           st rs, rt, immed -> rch
    STB,          // Store byte                           stb rs, rt, immed -> rch
    STADDR,       // Store address                        staddr rt, immed -> rch
    STBADDR,      // Store byte address                   stbaddr rt, immed -> rch

    SLL,          // Shift left logical                   sll rd, rs, immed
    SRL,          // Shift right logical                  srl rd, rs, immed
    SRA,          // Shift right arithmetic               sra rd, rs, immed
    SLLV,         // Shift left logical variable          sllv rd, rs, rt
    SRLV,         // Shift right logical variable         srlv rd, rs, rt
    SRAV,         // Shift right arithmetic variable      srav rd, rs, rt

    SEQ,          // Set if equal                         seq rd, rs, rt
    SNE,          // Set if not equal                     sne rd, rs, rt
    SLT,          // Set if less than                     slt rd, rs, rt
    SLTU,         // Set if less than unsigned            sltu rd, rs, rt
    SEQI,         // Set if equal immediate               seqi rd, rs, immed
    SNEI,         // Set if not equal immediate           snei rd, rs, immed
    SLTI,         // Set if less than immediate           slti rd, rs, immed
    SLTIU,        // Set if less than unsigned immediate  sltiu rd, rs, immed

    LUI,          // Load upper immediate                 lui rd, immed

    PSEL,         // Predicated selection (rd = p?rs:rt)  psel rd, rs, rt

    CLZ,          // Count leading zeroes                 clz rd, rs (?)

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
    POPC,         // Population count (number of 1s)      popc rd, rs (?)
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
    FETCHPST,     // Fetch persistent (repeat execution)  fetchpst rs, immed
    RMTFETCH,     // "Fetch" to remote cluster            rmtfetch rs, immed -> rch
    RMTFETCHPST,  // "Fetch persistent" to remote cluster rmtfetchpst rs, immed -> rch
    RMTFILL,      // Remote fill (no auto execution)      rmtfill rs, immed -> rch
    RMTEXECUTE,   // Remote execute (specify target)      rmtexecute -> rch
    RMTNXIPK      // "Next IPK" to remote cluster         rmtnxipk -> rch

  };

};

#endif /* INSTRUCTIONMAP_H_ */
