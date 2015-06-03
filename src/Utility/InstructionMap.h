/*
 * InstructionMap.h
 *
 * Listing of all instructions, their formats, and other associated information.
 *
 * Automatically generated by update_isa.py on 2015-06-03.
 *
 */
 
#ifndef INSTRUCTIONMAP_H_
#define INSTRUCTIONMAP_H_

#include <string>

typedef std::string inst_name_t;

class InstructionMap {

public:

  enum Opcode {

    OP_NOR = 0,         // nor rd, rs, rt (-> ch)
    OP_AND = 0,         // and rd, rs, rt (-> ch)
    OP_OR = 0,          // or rd, rs, rt (-> ch)
    OP_XOR = 0,         // xor rd, rs, rt (-> ch)
    OP_SETEQ = 0,       // seteq rd, rs, rt (-> ch)
    OP_SETNE = 0,       // setne rd, rs, rt (-> ch)
    OP_SETLT = 0,       // setlt rd, rs, rt (-> ch)
    OP_SETLTU = 0,      // setltu rd, rs, rt (-> ch)
    OP_SETGTE = 0,      // setgte rd, rs, rt (-> ch)
    OP_SETGTEU = 0,     // setgteu rd, rs, rt (-> ch)
    OP_SLL = 0,         // sll rd, rs, rt (-> ch)
    OP_SRL = 0,         // srl rd, rs, rt (-> ch)
    OP_SRA = 0,         // sra rd, rs, rt (-> ch)
    OP_ADDU = 0,        // addu rd, rs, rt (-> ch)
    OP_SUBU = 0,        // subu rd, rs, rt (-> ch)
    OP_NOR_P = 1,       // nor.p rd, rs, rt (-> ch)
    OP_AND_P = 1,       // and.p rd, rs, rt (-> ch)
    OP_OR_P = 1,        // or.p rd, rs, rt (-> ch)
    OP_XOR_P = 1,       // xor.p rd, rs, rt (-> ch)
    OP_SETEQ_P = 1,     // seteq.p rd, rs, rt (-> ch)
    OP_SETNE_P = 1,     // setne.p rd, rs, rt (-> ch)
    OP_SETLT_P = 1,     // setlt.p rd, rs, rt (-> ch)
    OP_SETLTU_P = 1,    // setltu.p rd, rs, rt (-> ch)
    OP_SETGTE_P = 1,    // setgte.p rd, rs, rt (-> ch)
    OP_SETGTEU_P = 1,   // setgteu.p rd, rs, rt (-> ch)
    OP_SRL_P = 1,       // srl.p rd, rs, rt (-> ch)
    OP_ADDU_P = 1,      // addu.p rd, rs, rt (-> ch)
    OP_SUBU_P = 1,      // subu.p rd, rs, rt (-> ch)
    OP_NORI = 2,        // nori rd, rs, immed (-> ch)
    OP_NORI_P = 3,      // nori.p rd, rs, immed (-> ch)
    OP_ANDI = 6,        // andi rd, rs, immed (-> ch)
    OP_ANDI_P = 7,      // andi.p rd, rs, immed (-> ch)
    OP_ORI = 10,        // ori rd, rs, immed (-> ch)
    OP_ORI_P = 11,      // ori.p rd, rs, immed (-> ch)
    OP_XORI = 14,       // xori rd, rs, immed (-> ch)
    OP_XORI_P = 15,     // xori.p rd, rs, immed (-> ch)
    OP_SETEQI = 18,     // seteqi rd, rs, immed (-> ch)
    OP_SETEQI_P = 19,   // seteqi.p rd, rs, immed (-> ch)
    OP_SETNEI = 22,     // setnei rd, rs, immed (-> ch)
    OP_SETNEI_P = 23,   // setnei.p rd, rs, immed (-> ch)
    OP_SETLTI = 26,     // setlti rd, rs, immed (-> ch)
    OP_SETLTI_P = 27,   // setlti.p rd, rs, immed (-> ch)
    OP_SETLTUI = 30,    // setltui rd, rs, immed (-> ch)
    OP_SETLTUI_P = 31,  // setltui.p rd, rs, immed (-> ch)
    OP_SETGTEI = 34,    // setgtei rd, rs, immed (-> ch)
    OP_SETGTEI_P = 35,  // setgtei.p rd, rs, immed (-> ch)
    OP_SETGTEUI = 38,   // setgteui rd, rs, immed (-> ch)
    OP_SETGTEUI_P = 39, // setgteui.p rd, rs, immed (-> ch)
    OP_SLLI = 42,       // slli rd, rs, shamt (-> ch)
    OP_SRLI = 46,       // srli rd, rs, shamt (-> ch)
    OP_SRLI_P = 47,     // srli.p rd, rs, shamt (-> ch)
    OP_SRAI = 50,       // srai rd, rs, shamt (-> ch)
    OP_ADDUI = 54,      // addui rd, rs, immed (-> ch)
    OP_ADDUI_P = 55,    // addui.p rd, rs, immed (-> ch)
    OP_CREGRDI = 99,    // cregrdi rd, immed (-> ch)
    OP_CREGWRI = 95,    // cregwri rs, immed
    OP_SCRATCHWR = 76,  // scratchwr rs, rt
    OP_SCRATCHWRI = 78, // scratchwri rs, immed
    OP_SCRATCHRD = 110, // scratchrd rd, rs (-> ch)
    OP_SCRATCHRDI = 109,// scratchrdi rd, immed (-> ch)
    OP_SETCHMAP = 80,   // setchmap rs, rt
    OP_SETCHMAPI = 82,  // setchmapi rs, immed
    OP_GETCHMAP = 106,  // getchmap rd, rs (-> ch)
    OP_GETCHMAPI = 105, // getchmapi rd, immed (-> ch)
    OP_PSEL = 4,        // psel rd, rs, rt (-> ch)
    OP_RMTNXIPK = 65,   // rmtnxipk -> ch
    OP_MULHW = 8,       // mulhw rd, rs, rt (-> ch)
    OP_RMTEXECUTE = 69, // rmtexecute -> ch
    OP_PSEL_FETCH = 68, // psel.fetch rs, rt
    OP_PSEL_FETCHR = 96,// psel.fetchr immed, immed
    OP_FETCH = 86,      // fetch rs
    OP_FETCHR = 85,     // fetchr immed
    OP_FETCHPST = 90,   // fetchpst rs
    OP_FETCHPSTR = 93,  // fetchpstr immed
    OP_FILL = 94,       // fill rs
    OP_FILLR = 89,      // fillr immed
    OP_MULLW = 12,      // mullw rd, rs, rt (-> ch)
    OP_IBJMP = 77,      // ibjmp immed
    OP_IRDR = 98,       // irdr rd, rs (-> ch)
    OP_MULHWU = 16,     // mulhwu rd, rs, rt (-> ch)
    OP_SYSCALL = 73,    // syscall immed
    OP_IWTR = 64,       // iwtr rs, rt (-> ch)
    OP_WOCHE = 81,      // woche immed -> ch
    OP_CLZ = 62,        // clz rd, rs (-> ch)
    OP_LUI = 92,        // lui rd, immed
    OP_LLI = 107,       // lli rd, immed
    OP_STW = 72,        // stw rs, rt, immed -> ch
    OP_LDW = 66,        // ldw rs, immed -> ch
    OP_STHW = 84,       // sthw rs, rt, immed -> ch
    OP_LDHWU = 70,      // ldhwu rs, immed -> ch
    OP_STB = 88,        // stb rs, rt, immed -> ch
    OP_LDBU = 74,       // ldbu rs, immed -> ch
    OP_LDL = 67,        // ldl rs, immed -> ch
    OP_STC = 32,        // stc rs, rt, immed -> ch
    OP_LDADD = 36,      // ldadd rs, rt, immed -> ch
    OP_LDOR = 40,       // ldor rs, rt, immed -> ch
    OP_LDAND = 44,      // ldand rs, rt, immed -> ch
    OP_LDXOR = 48,      // ldxor rs, rt, immed -> ch
    OP_EXCHANGE = 52,   // exchange rs, rt, immed -> ch
    OP_SELCH = 97,      // selch rd, immed
    OP_TSTCHI = 101,    // tstchi rd, immed (-> ch)
    OP_TSTCHI_P = 103,  // tstchi.p rd, immed (-> ch)
    OP_SENDCONFIG = 71, // sendconfig rs, immed -> ch
    OP_NXIPK = 5        // nxipk

  };
  
  enum Function {
    FN_NOR = 0,
    FN_AND = 1,
    FN_OR = 2,
    FN_XOR = 3,
    FN_SETEQ = 4,
    FN_SETNE = 5,
    FN_SETLT = 6,
    FN_SETLTU = 7,
    FN_SETGTE = 8,
    FN_SETGTEU = 9,
    FN_SLL = 10,
    FN_SRL = 11,
    FN_SRA = 12,
    FN_ADDU = 13,
    FN_SUBU = 14,
    FN_CREGRDI = -1,
    FN_PSEL = 16,
    FN_RMTNXIPK = 31,
    FN_MULHW = 17,
    FN_MULLW = 18,
    FN_MULHWU = 19,
    FN_CLZ = 20,
  };
  
  enum Format {
    FMT_FF,     // Fetch format                  (rs,immed)
    FMT_PFF,    // Predicated fetch format       (immed:16s, immed:7s)
    FMT_0R,     // Zero registers                (unused)        (immed)
    FMT_0Rnc,   // Zero registers, no channel    (immed)
    FMT_1R,     // One register                  (rd,immed)      (rs,immed)
    FMT_1Rnc,   // One register, no channel      (rd,immed)      (rs,immed)       (rd,unused)
    FMT_2R,     // Two registers                 (rd,rs,immed)   (rs,rt,unused)
    FMT_2Rnc,   // Two registers, no channel     (rs,rt,unused)
    FMT_2Rs,    // Two registers, shift amount   (rd,rs,shamt)
    FMT_3R      // Three registers               (rd,rs,rt)
  };
  
  typedef InstructionMap::Opcode opcode_t;
  typedef InstructionMap::Function function_t;
  typedef InstructionMap::Format format_t;
  
  // Simple true/false questions to ask of each operation.
  static bool storesResult(opcode_t opcode);
  static bool hasDestReg(opcode_t opcode);
  static bool hasSrcReg1(opcode_t opcode);
  static bool hasSrcReg2(opcode_t opcode);
  static bool hasImmediate(opcode_t opcode);
  static bool hasRemoteChannel(opcode_t opcode);
  static bool setsPredicate(opcode_t opcode);
  static bool isALUOperation(opcode_t opcode);
  static bool hasSignedImmediate(opcode_t opcode);
  
  // The total number of instructions currently supported.
  static int numInstructions();
  
  // Convert back and forth between names and opcodes.
  static opcode_t opcode(const inst_name_t& name);
  static const inst_name_t& name(opcode_t opcode, function_t function = (function_t)0);
  
  static function_t function(const inst_name_t& name);
  static function_t function(opcode_t opcode);
  static format_t format(opcode_t opcode);

};

typedef InstructionMap::Opcode opcode_t;
typedef InstructionMap::Function function_t;
typedef InstructionMap::Format format_t;

#endif /* INSTRUCTIONMAP_H_ */
