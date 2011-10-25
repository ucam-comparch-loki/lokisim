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

void ALU::execute(DecodedInst& dec) const {

  assert(dec.isALUOperation());

  bool pred = parent()->readPredicate();

  // Cast to 32 bits because our architecture is supposed to use 32-bit
  // arithmetic.
  int32_t val1 = dec.operand1();
  int32_t val2 = dec.operand2();
  int32_t result;

  switch(dec.function()) {
    case InstructionMap::FN_NOR:     result = ~(val1 | val2); break;
    case InstructionMap::FN_AND:     result = val1 & val2; break;
    case InstructionMap::FN_OR:      result = val1 | val2; break;
    case InstructionMap::FN_XOR:     result = val1 ^ val2; break;

    case InstructionMap::FN_SETEQ:   result = (val1 == val2); break;
    case InstructionMap::FN_SETNE:   result = (val1 != val2); break;
    case InstructionMap::FN_SETLT:   result = (val1 < val2); break;
    case InstructionMap::FN_SETLTU:  result = ((uint32_t)val1 < (uint32_t)val2); break;
    case InstructionMap::FN_SETGTE:  result = (val1 >= val2); break;
    case InstructionMap::FN_SETGTEU: result = ((uint32_t)val1 >= (uint32_t)val2); break;

    case InstructionMap::FN_SLL:     result = val1 << val2; break;
    case InstructionMap::FN_SRL:     result = (uint32_t)val1 >> val2; break;
    case InstructionMap::FN_SRA:     result = val1 >> val2; break;

    case InstructionMap::FN_ADDU:    result = val1 + val2; break;
    case InstructionMap::FN_SUBU:    result = val1 - val2; break;

    case InstructionMap::FN_PSEL:    result = pred ? val1 : val2; break;

    case InstructionMap::FN_MULHW:   result = ((int64_t)val1 * (int64_t)val2) >> 32; break;
    case InstructionMap::FN_MULLW:   result = ((int64_t)val1 * (int64_t)val2) & 0xFFFFFFFF; break;
    case InstructionMap::FN_MULHWU:  result = ((uint64_t)((uint32_t)val1) *
                                               (uint64_t)((uint32_t)val2)) >> 32; break;

    case InstructionMap::FN_CLZ:
      // Would prefer to use a single instruction for this, but I don't expect
      // this is going to be called often.
      // If there are any bits set, copy them into every less-significant position.
      val1 |= (val1 >> 1);
      val1 |= (val1 >> 2);
      val1 |= (val1 >> 4);
      val1 |= (val1 >> 8);
      val1 |= (val1 >> 16);
      result = 32 - __builtin_popcount(val1);
      break;

    default:
      cerr << "Invalid function code: " << dec.function() << endl;
      cerr << dec << endl;
      assert(false);
  }

  dec.result(result);

  if(dec.setsPredicate()) {

    bool newPredicate;

    switch(dec.function()) {
      // For additions and subtractions, the predicate signals overflow or
      // underflow.
      case InstructionMap::FN_ADDU: {
        int64_t result64 = (int64_t)val1 + (int64_t)val2;
        newPredicate = (result64 > INT_MAX) || (result64 < INT_MIN);
        break;
      }

      // The 68k and x86 set the borrow bit if a - b < 0 for subtractions.
      // The 6502 and PowerPC treat it as a carry bit.
      // http://en.wikipedia.org/wiki/Carry_flag#Carry_flag_vs._Borrow_flag
      case InstructionMap::FN_SUBU: {
        int64_t result64 = (int64_t)val1 - (int64_t)val2;
        newPredicate = (result64 > INT_MAX) || (result64 < INT_MIN);
        break;
      }

      // Otherwise, it holds the least significant bit of the result.
      // Potential alternative: newPredicate = (result != 0)
      default:
        newPredicate = result&1;
    }

    setPred(newPredicate);
  }

}

void ALU::setPred(bool val) const {
  parent()->writePredicate(val);
}

ExecuteStage* ALU::parent() const {
  return static_cast<ExecuteStage*>(this->get_parent());
}

int32_t ALU::readReg(RegisterIndex reg) const {return parent()->readReg(reg);}
int32_t ALU::readWord(MemoryAddr addr) const {return parent()->readWord(addr);}
int32_t ALU::readByte(MemoryAddr addr) const {return parent()->readByte(addr);}

void ALU::writeReg(RegisterIndex reg, Word data) const {parent()->writeReg(reg, data);}
void ALU::writeWord(MemoryAddr addr, Word data) const {parent()->writeWord(addr, data);}
void ALU::writeByte(MemoryAddr addr, Word data) const {parent()->writeByte(addr, data);}

ALU::ALU(const sc_module_name& name, const ComponentID& ID) : Component(name, ID) {
  // Nothing
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
  // Check regularly to ensure that the implementation exactly matches that in
  // Alex's interp.c.

  switch (code) {
    case 0x1: { /* SYS_exit */
      RETURN_CODE = readReg(13);
      std::cerr << "Simulation ended with sys_exit after "
                << (int)sc_core::sc_time_stamp().to_default_time_units() << " cycles." << endl;
      Instrumentation::endExecution();
      break;
    }
    case 0x2: { /* SYS_open */
      char fname[1024];
      int mode = (int)convertTargetFlags((unsigned)readReg(14));
      int perm = (int)readReg(15);
      int i;
      int fd;
      int start = readReg(13);
      /* read fname from Loki memory */
      for (i=0; i < 1024; i++) {
        char next = readByte(start + i);
        fname[i] = next;
        if(next == '\0') break;
      }
      fd = open(fname, mode, perm);
      if (fd < 0) {
        perror("problem opening file");
      }
      /* FIXME - set errno */
      writeReg(11, fd);
      break;
    }
    case 0x3: { /* SYS_close */
      int fd = readReg(13);
      if(fd <= 2) { // Don't allow simulated program to close stdio pipes.
        fsync(fd);
        writeReg(11, 0);
      }
      else writeReg(11, close(fd));
      /* FIXME - set errno */
      break;
    }
    case 0x4: { /* SYS_read */
      int fd = readReg(13);
      unsigned len = (unsigned)readReg(15);
      char *buf = (char*)malloc(len);
      uint i;
      writeReg(11, read(fd, buf, len));
      int start = readReg(14);
      for (i=0; i < len; i++) {
        writeByte(start+i, buf[i]);
      }
      free(buf);
      /* FIXME - set errno */
      break;
    }
    case 0x5: { /* SYS_write */
      unsigned len = (unsigned)readReg(15);
      char *str = (char*)malloc(len);
      uint i;
      int fd = readReg(13);
      int start = readReg(14);
      /* copy string out of Loki memory */
      for (i=0; i < len; i++) {
        str[i] = readByte(start + i);
      }
      writeReg(11, write(fd, str, len));
      free(str);
      break;
    }
    case 0x6: { /* SYS_lseek */
      int fd = readReg(13);
      int offset = readReg(14);
      int whence = readReg(15);
      writeReg(11, lseek(fd, offset, whence));
      /* implicit assumption that whence value meanings are the
       * same on host as the target OS */
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
