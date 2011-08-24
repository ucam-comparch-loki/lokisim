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
#include <vector>

// The typedef for Operation is forced to be after the enumeration has been
// defined. There is therefore one typedef in the class definition, allowing
// methods to use it, and one outside, allowing other files to use it.

// Opcodes used in the binary instruction encoding.
typedef short opcode_t;

// Textual representation of an instruction.
typedef std::string inst_name_t;

class InstructionMap {

public:

  enum Operation {

    NOP,          // REMOVE

    LDW,          // Load word                            ldw rs, immed -> rch
    LDHWU,        // Load halfword (zero-extended)        ldhwu rs, immed -> rch
    LDBU,         // Load byte (zero-extended)            ldbu rs, immed -> rch
    STW,          // Store word                           stw rs, rt, immed -> rch
    STHW,         // Store halfword                       sthw rs, rt, immed -> rch
    STB,          // Store byte                           stb rs, rt, immed -> rch
    STWADDR,      // REMOVE
    STBADDR,      // REMOVE

    LDVECTOR,     // REMOVE
    LDSTREAM,     // REMOVE
    STVECTOR,     // REMOVE
    STSTREAM,     // REMOVE

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

    NAND,         // REMOVE
    CLR,          // REMOVE
    ORC,          // REMOVE
    POPC,         // REMOVE
    RSUBI,        // REMOVE

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

    SETFETCHCH,   // REMOVE
    IBJMP,        // In buffer jump                       ibjmp immed
    FETCH,        // Fetch instruction packet             fetch rs, immed
    PSELFETCH,    // Fetch dependent on predicate         psel.fetch rs rt
    FETCHPST,     // Fetch persistent (repeat execution)  fetchpst rs, immed
    RMTFETCH,     // REMOVE
    RMTFETCHPST,  // REMOVE
    FILL,         // Fill (fetch but don't execute)       fill rs, immed
    RMTEXECUTE,   // Remote execute (specify target)      rmtexecute -> rch
    NXIPK,        // Start next IPK immediately           nxipk

    SETCHMAP,     // Set channel map entry                setchmap immed, rs

    SYSCALL       // System call                          syscall immed

  };

  typedef InstructionMap::Operation operation_t;

  // Return whether the given operation should specify a particular field.
  static bool hasDestReg(operation_t operation);
  static bool hasSrcReg1(operation_t operation);
  static bool hasSrcReg2(operation_t operation);
  static bool hasImmediate(operation_t operation);
  static bool hasRemoteChannel(operation_t operation);

  // Returns whether the given operation uses the ALU.
  static bool isALUOperation(operation_t operation);

  // Returns whether the operation stores its result in a register.
  static bool storesResult(operation_t operation);

  // Returns the opcode of the given operation name.
  static opcode_t opcode(const inst_name_t& name);

  // Returns the decoded operation value from the given opcode.
  static operation_t operation(opcode_t opcode);

  // Returns the name of the given instruction.
  static const inst_name_t& name(operation_t operation);

  // Returns the total number of instructions currently supported.
  static int numInstructions();

private:
  static std::map<inst_name_t, opcode_t> nto; // name to opcode
  static std::map<opcode_t, operation_t> oti; // opcode to instruction
  static std::map<operation_t, inst_name_t> itn; // instruction to name

  // Fill up the maps with the correct values.
  static void initialise();

  // Add a single instruction to all maps.
  static void addToMaps(const inst_name_t& name, opcode_t opcode, operation_t instruction);

  // Generate a vector of boolean values showing whether each instruction
  // satisfies a particular property.
  static const std::vector<bool>& bitVector(bool defaultVal, const operation_t exceptions[], int numExceptions);

};

// Operation for a core to perform.
typedef InstructionMap::Operation operation_t;

#endif /* INSTRUCTIONMAP_H_ */
