/*
 * ALU.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "ALU.h"
#include "ExecuteStage.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/Instruction.h"
#include "../../../Utility/Arguments.h"
#include "../../../Utility/Instrumentation.h"
#include "../../../Exceptions/InvalidOptionException.h"

void ALU::execute(DecodedInst& dec) {

  assert(dec.isExecuteStageOperation());

  // Wait until the final cycle to compute the result.
  if (cyclesRemaining > 0)
    cyclesRemaining--;
  else
    cyclesRemaining = getFunctionLatency(dec.function());

  if (cyclesRemaining > 0)
    return;

  bool pred = parent()->readPredicate();

  // Cast to 32 bits because our architecture is supposed to use 32-bit
  // arithmetic.
  int32_t val1 = dec.operand1();
  int32_t val2 = dec.operand2();
  int32_t result;

  switch (dec.function()) {
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
      cerr << dec << endl;
      throw InvalidOptionException("ALU function code", dec.function());
      break;
  }

  dec.result(result);

}

bool ALU::busy() const {
  return (cyclesRemaining > 0);
}

cycle_count_t ALU::getFunctionLatency(function_t fn) {
  cycle_count_t cycles;

  switch (fn) {
    case InstructionMap::FN_MULHW:
    case InstructionMap::FN_MULHWU:
    case InstructionMap::FN_MULLW:
      cycles = 1;
      break;

    default:
      cycles = 0;
      break;
  }

  return cycles;
}

void ALU::setPredicate(bool val) const {
  parent()->writePredicate(val);
}

ExecuteStage* ALU::parent() const {
  return static_cast<ExecuteStage*>(this->get_parent_object());
}

int32_t ALU::readReg(RegisterIndex reg) const {return parent()->readReg(reg);}
int32_t ALU::readWord(MemoryAddr addr) const {return parent()->readWord(addr);}
int32_t ALU::readByte(MemoryAddr addr) const {return parent()->readByte(addr);}

void ALU::writeReg(RegisterIndex reg, Word data) const {parent()->writeReg(reg, data);}
void ALU::writeWord(MemoryAddr addr, Word data) const {parent()->writeWord(addr, data);}
void ALU::writeByte(MemoryAddr addr, Word data) const {parent()->writeByte(addr, data);}

ALU::ALU(const sc_module_name& name, const ComponentID& ID) : Component(name, ID) {
  cyclesRemaining = 0;
}

//============================================================================//
// System call stuff
//============================================================================//

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void ALU::systemCall(DecodedInst& dec) const {
  // Note: for now, all system calls will complete instantaneously. This is
  // not at all realistic, so performance of programs executing system calls
  // should be measured with care.
  // Check regularly to ensure that the implementation exactly matches that in
  // Alex's interp.c.

  int code = dec.immediate();

  switch (code) {
    case 0x1: { /* SYS_exit */
      RETURN_CODE = readReg(13);
      std::cerr << "Simulation ended with sys_exit (arg " << (uint)RETURN_CODE << ") after "
                << Instrumentation::currentCycle() << " cycles." << endl;
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
      //cerr << "+ SYS_open fname=" << fname << " mode=" << mode << " perm=" << mode << " fd=" << fd << endl;
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
      //cerr << "+ SYS_close fd=" << fd << endl;
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
      //cerr << "+ SYS_read fd=" << fd << " start=" << start << " len=" << len << " result=" << readReg(11) << endl;
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
      //cerr << "+ SYS_write fd=" << fd << " start=" << start << " len=" << len << " result=" << readReg(11) << endl;
      break;
    }
    case 0x6: { /* SYS_lseek */
      int fd = readReg(13);
      int offset = readReg(14);
      int whence = readReg(15);
      writeReg(11, lseek(fd, offset, whence));
      /* implicit assumption that whence value meanings are the
       * same on host as the target OS */
      //cerr << "+ SYS_write fd=" << fd << " offset=" << offset << " whence=" << whence << " result=" << readReg(11) << endl;
      break;
    }

    case 0x10: { /* tile ID */
      LOKI_WARN << "syscall 0x10 (tile ID) is deprecated. Use control register 1 instead." << endl;
      int tile = this->id.tile.flatten();
      writeReg(11, tile);
      break;
    }
    case 0x11: { /* position within tile */
      LOKI_WARN << "syscall 0x11 (core ID) is deprecated. Use control register 1 instead." << endl;
      int position = this->id.position;
      writeReg(11, position);
      break;
    }

    case 0x20: /* start energy log */
      ENERGY_TRACE = 1;
      Instrumentation::startEventLog();
      break;
    case 0x21: /* end energy log */
      Instrumentation::stopEventLog();
      ENERGY_TRACE = 0;
      break;
    case 0x22: /* start verbose debugging */
      DEBUG = 1;
      break;
    case 0x23: /* end verbose debugging */
      DEBUG = 0;
      break;
    case 0x24: /* start instruction trace */
      LOKI_WARN << "syscall 0x24 (start instruction trace) is deprecated." << endl;
      break;
    case 0x25: /* end instruction trace */
      LOKI_WARN << "syscall 0x25 (end instruction trace) is deprecated." << endl;
      break;
    case 0x26: { /* software triggered register file snapshot */
      // TODO
      break;
    }
    case 0x27: /* Clear execution stats */
      LOKI_WARN << "wiping statistics after " << Instrumentation::currentCycle() << " cycles." << endl;
      Instrumentation::clearStats();
      break;

    case 0x30: { /* get current cycle */
      cycle_count_t cycle = Instrumentation::currentCycle();
      writeReg(11, cycle >> 32);
      writeReg(12, cycle & 0xFFFFFFFF);
      break;
    }

    default:
      throw InvalidOptionException("system call opcode", code);
      break;
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
  // 0x0010 = internal "mark" flag
  // 0x0020 = internal "defer" flag
  CHECK_FLAG(0x0040, O_ASYNC);
  // 0x0080 = BSD "shared lock"
  // 0x0100 = BSD "exclusive lock"
  CHECK_FLAG(0x0200, O_CREAT);
  CHECK_FLAG(0x0400, O_TRUNC);
  CHECK_FLAG(0x0800, O_EXCL);
  // 0x1000 = sys5 "non-blocking I/O" - there's a more general version below
  CHECK_FLAG(0x2000, O_SYNC);
  CHECK_FLAG(0x4000, O_NONBLOCK);
  CHECK_FLAG(0x8000, O_NOCTTY);

  if (tflags != 0x0)
    fprintf (stderr,
       "Simulator Error: problem converting target open flags for host.  0x%x\n",
       tflags);

  return hflags;
}
