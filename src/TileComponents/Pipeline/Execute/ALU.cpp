/*
 * ALU.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "ALU.h"
#include "ExecuteStage.h"
#include "../../../Utility/InstructionMap.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/Instruction.h"

bool ALU::execute(DecodedInst& dec) const {

  if(dec.hasResult()) return true;

  bool execute = shouldExecute(dec.predicate());

  if(dec.isALUOperation())
    Instrumentation::operation(id, dec, execute);

  if(dec.operation() == InstructionMap::NOP) return false;
  if(!execute) return false;

  bool pred = parent()->readPredicate();

  // Cast to 32 bits because our architecture is supposed to use 32-bit
  // arithmetic.
  int32_t val1 = dec.operand1();
  int32_t val2 = dec.operand2();
  int32_t result;

  if(DEBUG) cout << this->name() << ": executing " <<
    dec.name() << " on " << val1 << " and " << val2 << endl;

  switch(dec.operation()) {

    case InstructionMap::SLLI:
    case InstructionMap::SLL:      result = val1 << val2; break;
    case InstructionMap::SRLI:
    case InstructionMap::SRL:      result = (uint32_t)val1 >> val2; break;
    case InstructionMap::SRAI:
    case InstructionMap::SRA:      result = val1 >> val2; break;

    case InstructionMap::SETEQ:
    case InstructionMap::SETEQI:   result = (val1 == val2); break;
    case InstructionMap::SETNE:
    case InstructionMap::SETNEI:   result = (val1 != val2); break;
    case InstructionMap::SETLT:
    case InstructionMap::SETLTI:   result = (val1 < val2); break;
    case InstructionMap::SETLTU:
    case InstructionMap::SETLTUI:  result = ((uint32_t)val1 < (uint32_t)val2); break;
    case InstructionMap::SETGTE:   result = (val1 >= val2); break;
    case InstructionMap::SETGTEU:
    case InstructionMap::SETGTEUI: result = ((uint32_t)val1 >= (uint32_t)val2); break;

    case InstructionMap::LUI:      result = val2 << 16; break;

    case InstructionMap::PSEL:     result = pred ? val1 : val2; break;

//    case InstructionMap::CLZ:    result = 32 - math.log(2, val1) + 1; break;

    case InstructionMap::NOR:
    case InstructionMap::NORI:     result = ~(val1 | val2); break;
    case InstructionMap::AND:
    case InstructionMap::ANDI:     result = val1 & val2; break;
    case InstructionMap::OR:
    case InstructionMap::ORI:      result = val1 | val2; break;
    case InstructionMap::XOR:
    case InstructionMap::XORI:     result = val1 ^ val2; break;

    case InstructionMap::NAND:     result = ~(val1 & val2); break;
    case InstructionMap::CLR:      result = val1 & ~val2; break;
    case InstructionMap::ORC:      result = val1 | ~val2; break;
    case InstructionMap::POPC:     result = __builtin_popcount(val1); break;
    case InstructionMap::RSUBI:    result = val2 - val1; break;

    case InstructionMap::LDW:
    case InstructionMap::LDHWU:
    case InstructionMap::LDBU:
    case InstructionMap::STWADDR:
    case InstructionMap::STBADDR:
    case InstructionMap::ADDU:
    case InstructionMap::ADDUI:  result = val1 + val2; break;
    case InstructionMap::SUBU:   result = val1 - val2; break;
    case InstructionMap::MULHW:  result = ((int64_t)val1 * (int64_t)val2) >> 32; break;
                                 // Convert to unsigned ints, then pad to 64 bits.
    case InstructionMap::MULHWU: result = ((uint64_t)((uint32_t)val1) *
                                           (uint64_t)((uint32_t)val2)) >> 32; break;
    case InstructionMap::MULLW:  result = ((int64_t)val1 * (int64_t)val2) & -1; break;

    case InstructionMap::SYSCALL:  result = 0; systemCall(val2); break;

    default: result = val1; // Is this a good default?
  }

  dec.result(result);

  if(dec.setsPredicate()) {

    bool newPredicate;

    switch(dec.operation()) {
      // For additions and subtractions, the predicate signals overflow or
      // underflow.
      case InstructionMap::ADDU:
      case InstructionMap::ADDUI: {
        int64_t result64 = (int64_t)val1 + (int64_t)val2;
        newPredicate = (result64 > INT_MAX) || (result64 < INT_MIN);
        break;
      }

      // The 68k and x86 set the borrow bit if a - b < 0 for subtractions.
      // The 6502 and PowerPC treat it as a carry bit.
      // http://en.wikipedia.org/wiki/Carry_flag
      case InstructionMap::SUBU: {
        int64_t result64 = (int64_t)val1 - (int64_t)val2;
        newPredicate = (result64 > INT_MAX) || (result64 < INT_MIN);
        break;
      }

      case InstructionMap::RSUBI: {
        int64_t result64 = (int64_t)val2 - (int64_t)val1;
        newPredicate = (result64 > INT_MAX) || (result64 < INT_MIN);
        break;
      }

      // Otherwise, it holds the least significant bit of the result.
      // Potential alternative: newPredicate = (result != 0)
      default: newPredicate = result&1;
    }

    setPred(newPredicate);
  }

  return true;

}

void ALU::setPred(bool val) const {
  parent()->writePredicate(val);
}

/* Determine whether this instruction should be executed. */
bool ALU::shouldExecute(short predBits) const {
  bool pred = parent()->readPredicate();

  bool result = (predBits == Instruction::ALWAYS) ||
                (predBits == Instruction::END_OF_PACKET) ||
                (predBits == Instruction::P     &&  pred) ||
                (predBits == Instruction::NOT_P && !pred);

  return result;

}

ExecuteStage* ALU::parent() const {
  return dynamic_cast<ExecuteStage*>(this->get_parent());
}

int32_t ALU::readReg(RegisterIndex reg) const {return parent()->readReg(reg);}
int32_t ALU::readWord(MemoryAddr addr) const {return parent()->readWord(addr);}
int32_t ALU::readByte(MemoryAddr addr) const {return parent()->readByte(addr);}

void ALU::writeReg(RegisterIndex reg, Word data) const {parent()->writeReg(reg, data);}
void ALU::writeWord(MemoryAddr addr, Word data) const {parent()->writeWord(addr, data);}
void ALU::writeByte(MemoryAddr addr, Word data) const {parent()->writeByte(addr, data);}

ALU::ALU(sc_module_name name) : Component(name) {
  id = parent()->id;
}

//==============================//
// System call stuff
//==============================//

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void ALU::systemCall(int code) const {
  // Note: for now, all system calls will complete instantaneously. This is
  // not at all realistic, so performance of programs executing system calls
  // should be measured with care.

  switch (code) {
    case 0x1: { /* SYS_exit */
      RETURN_CODE = readReg(6);
      std::cerr << "Simulation ended with sys_exit." << endl;
      Instrumentation::endExecution();
      break;
    }
    case 0x2: { /* SYS_open */
      char fname[1024];
      int mode = (int)convertTargetFlags((unsigned)readReg(7));
      int perm = (int)readReg(8);
      int i;
      int fd;
      /* read fname from Loki memory */
      for (i=0; i < 1024; i++) {
        char next = readByte(readReg(6) + i);
        fname[i] = next;
        if(next == '\0') break;
      }
      fd = open(fname, mode, perm);
      if (fd < 0) {
        perror("problem opening file");
      }
      /* FIXME - set errno */
      writeReg(4, fd);
      break;
    }
    case 0x3: { /* SYS_close */
      int fd = readReg(6);
      if(fd <= 2) { // Don't allow simulated program to close stdio pipes.
        fsync(fd);
        writeReg(4, 0);
      }
      else writeReg(4, close(fd));
      /* FIXME - set errno */
      break;
    }
    case 0x4: { /* SYS_read */
      int fd = readReg(6);
      unsigned len = (unsigned)readReg(8);
      char *buf = (char*)malloc(len);
      uint i;
      writeReg(4, read(fd, buf, len));
      for (i=0; i < len; i++) {
        writeByte(readReg(7)+i, buf[i]);
      }
      free(buf);
      /* FIXME - set errno */
      break;
    }
    case 0x5: { /* SYS_write */
      unsigned len = (unsigned)readReg(8);
      char *str = (char*)malloc(len);
      uint i;
      int fd = readReg(6);
      /* copy string out of Loki memory */
      for (i=0; i < len; i++) {
        str[i] = readByte(readReg(7) + i);
      }
      writeReg(4, write(fd, str, len));
      free(str);
      break;
    }
  }
}

/* Convert_target_flags taken from moxie interp */
#define CHECK_FLAG(T,H) if(tflags & T) { hflags |= H; tflags ^= T; }

/* Convert between newlib flag constants and Linux ones. */
uint ALU::convertTargetFlags(uint tflags) const {
  uint hflags = 0x0;

  CHECK_FLAG(0x0001, O_WRONLY);
  CHECK_FLAG(0x0002, O_RDWR);
  CHECK_FLAG(0x0008, O_APPEND);
  CHECK_FLAG(0x0200, O_CREAT);
  CHECK_FLAG(0x0400, O_TRUNC);
  CHECK_FLAG(0x0800, O_EXCL);
  CHECK_FLAG(0x2000, O_SYNC);

  if(tflags != 0x0)
    fprintf (stderr,
       "Simulator Error: problem converting target open flags for host.  0x%x\n",
       tflags);

  return hflags;
}
