/*
 * ALU.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "ALU.h"
#include <unistd.h>

#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/Instruction.h"
#include "../../../Utility/Arguments.h"
#include "../../../Utility/Assert.h"
#include "../../../Utility/Instrumentation.h"
#include "../../../Exceptions/InvalidOptionException.h"
#include "ExecuteStage.h"

void ALU::execute(DecodedInst& dec) {

  loki_assert(dec.isExecuteStageOperation());

  // Wait until the final cycle to compute the result.
  if (cyclesRemaining > 0)
    cyclesRemaining--;
  else
    cyclesRemaining = getFunctionLatency(dec.function());

  if (cyclesRemaining > 0)
    return;

  bool pred = parent().readPredicate();

  // Cast to 32 bits because our architecture is supposed to use 32-bit
  // arithmetic.
  int32_t val1 = dec.operand1();
  int32_t val2 = dec.operand2();
  int32_t result;

  switch (dec.function()) {
    case ISA::FN_NOR:     result = ~(val1 | val2); break;
    case ISA::FN_AND:     result = val1 & val2; break;
    case ISA::FN_OR:      result = val1 | val2; break;
    case ISA::FN_XOR:     result = val1 ^ val2; break;

    case ISA::FN_SETEQ:   result = (val1 == val2); break;
    case ISA::FN_SETNE:   result = (val1 != val2); break;
    case ISA::FN_SETLT:   result = (val1 < val2); break;
    case ISA::FN_SETLTU:  result = ((uint32_t)val1 < (uint32_t)val2); break;
    case ISA::FN_SETGTE:  result = (val1 >= val2); break;
    case ISA::FN_SETGTEU: result = ((uint32_t)val1 >= (uint32_t)val2); break;

    case ISA::FN_SLL:     result = val1 << val2; break;
    case ISA::FN_SRL:     result = (uint32_t)val1 >> val2; break;
    case ISA::FN_SRA:     result = val1 >> val2; break;

    case ISA::FN_ADDU:    result = val1 + val2; break;
    case ISA::FN_SUBU:    result = val1 - val2; break;

    case ISA::FN_PSEL:    result = pred ? val1 : val2; break;

    case ISA::FN_MULHW:   result = ((int64_t)val1 * (int64_t)val2) >> 32; break;
    case ISA::FN_MULLW:   result = ((int64_t)val1 * (int64_t)val2) & 0xFFFFFFFF; break;
    case ISA::FN_MULHWU:  result = ((uint64_t)((uint32_t)val1) *
                                               (uint64_t)((uint32_t)val2)) >> 32; break;

    case ISA::FN_CLZ:
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
    case ISA::FN_MULHW:
    case ISA::FN_MULHWU:
    case ISA::FN_MULLW:
      cycles = 1;
      break;

    default:
      cycles = 0;
      break;
  }

  return cycles;
}

void ALU::setPredicate(bool val) const {
  parent().writePredicate(val);
}

ExecuteStage& ALU::parent() const {
  return static_cast<ExecuteStage&>(*(this->get_parent_object()));
}

int32_t ALU::readReg(RegisterIndex reg) const {return parent().readReg(reg);}
int32_t ALU::readWord(MemoryAddr addr) const {return parent().readWord(addr);}
int32_t ALU::readByte(MemoryAddr addr) const {return parent().readByte(addr);}

void ALU::writeReg(RegisterIndex reg, Word data) const {parent().writeReg(reg, data);}
void ALU::writeWord(MemoryAddr addr, Word data) const {parent().writeWord(addr, data);}
void ALU::writeByte(MemoryAddr addr, Word data) const {parent().writeByte(addr, data);}

ALU::ALU(const sc_module_name& name) : LokiComponent(name) {
  cyclesRemaining = 0;
}

//============================================================================//
// System call stuff
//============================================================================//

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../../../Utility/SystemCall.h"

void ALU::systemCall(DecodedInst& dec) const {
  // Note: for now, all system calls will complete instantaneously. This is
  // not at all realistic, so performance of programs executing system calls
  // should be measured with care.

  // Argument/result positions, according to compiler's ABI.
  // https://svr-rdm34-issue.cl.cam.ac.uk/w/loki/abi/
  static int result1Reg = 11;
  static int result2Reg = 12;
  static int arg1Reg = 13;
  static int arg2Reg = 14;
  static int arg3Reg = 15;

  SystemCall code = (SystemCall)dec.immediate();

  switch (code) {
    case SYS_EXIT: {
      RETURN_CODE = readReg(arg1Reg);
      std::cerr << "Simulation ended with sys_exit (arg " << (uint)RETURN_CODE << ") after "
                << Instrumentation::currentCycle() << " cycles." << endl;
      Instrumentation::endExecution();
      break;
    }

    case SYS_OPEN: {
      char fname[1024];
      int start = readReg(arg1Reg);
      int mode = (int)convertTargetFlags((unsigned int)readReg(arg2Reg));
      int perm = (int)readReg(arg3Reg);

      // Read filename from memory.
      for (int i=0; i < 1024; i++) {
        char next = readByte(start + i);
        fname[i] = next;
        if (next == '\0') break;
      }

      int fd = open(fname, mode, perm);
      if (fd < 0) {
        LOKI_ERROR << "problem opening file: " << fname << endl;
      }

      writeReg(result1Reg, fd);
      break;
    }

    case SYS_CLOSE: {
      int fd = readReg(arg1Reg);

      // Don't allow simulated program to close stdio pipes.
      if (fd <= 2) {
        fsync(fd);
        writeReg(result1Reg, 0);
      }
      else
        writeReg(result1Reg, close(fd));

      break;
    }

    case SYS_READ: {
      int fd = readReg(arg1Reg);
      int start = readReg(arg2Reg);
      unsigned int len = (unsigned int)readReg(arg3Reg);

      char* buf = (char*)malloc(len);
      writeReg(result1Reg, read(fd, buf, len));

      for (uint i=0; i < len; i++) {
        writeByte(start+i, buf[i]);
      }

      free(buf);
      break;
    }

    case SYS_WRITE: {
      int fd = readReg(arg1Reg);
      int start = readReg(arg2Reg);
      unsigned int len = (unsigned int)readReg(arg3Reg);
      char *str = (char*)malloc(len);

      // Copy string out of memory.
      for (uint i=0; i < len; i++) {
        str[i] = readByte(start + i);
      }

      writeReg(result1Reg, write(fd, str, len));
      free(str);
      break;
    }

    case SYS_SEEK: {
      int fd = readReg(arg1Reg);
      int offset = readReg(arg2Reg);
      int whence = readReg(arg3Reg);
      writeReg(result1Reg, lseek(fd, offset, whence));
      break;
    }

    case SYS_TILE_ID: {
      LOKI_WARN << "syscall 0x10 (tile ID) is deprecated. Use control register 1 instead." << endl;
      int tile = parent().id().tile.flatten(Encoding::softwareTileID);
      writeReg(result1Reg, tile);
      break;
    }
    case SYS_POSITION: {
      LOKI_WARN << "syscall 0x11 (core ID) is deprecated. Use control register 1 instead." << endl;
      int position = parent().id().position;
      writeReg(result1Reg, position);
      break;
    }

    case SYS_ENERGY_LOG_ON:
      ENERGY_TRACE = 1;
      Instrumentation::start();
      break;
    case SYS_ENERGY_LOG_OFF:
      Instrumentation::stop();
      ENERGY_TRACE = 0;
      break;
    case SYS_DEBUG_ON:
      DEBUG = 1;
      break;
    case SYS_DEBUG_OFF:
      DEBUG = 0;
      break;
    case SYS_INST_TRACE_ON:
      LOKI_WARN << "syscall 0x24 (start instruction trace) is deprecated." << endl;
      break;
    case SYS_INST_TRACE_OFF:
      LOKI_WARN << "syscall 0x25 (end instruction trace) is deprecated." << endl;
      break;
    case SYS_SNAPSHOT: {
      // TODO
      break;
    }
    case SYS_CLEAR_STATS:
      LOKI_WARN << "wiping statistics after " << Instrumentation::currentCycle() << " cycles." << endl;
      Instrumentation::reset();
      break;
    case SYS_START_STATS:
      LOKI_WARN << "starting statistics collection at cycle " << Instrumentation::currentCycle() << endl;
      Instrumentation::start();
      break;
    case SYS_FREEZE_STATS:
      LOKI_WARN << "ending statistics collection at cycle " << Instrumentation::currentCycle() << endl;
      Instrumentation::stop();
      break;

    case SYS_CURRENT_CYCLE: {
      cycle_count_t cycle = Instrumentation::currentCycle();
      writeReg(result1Reg, cycle >> 32);
      writeReg(result2Reg, cycle & 0xFFFFFFFF);
      break;
    }

    default:
      throw InvalidOptionException("system call opcode", code);
      break;
  }
}

// Convert_target_flags taken from moxie interp
#define CHECK_FLAG(T,H) if(targetFlags & T) { hflags |= H; targetFlags ^= T; }

// Convert between newlib flag constants and Linux ones.
uint ALU::convertTargetFlags(uint targetFlags) const {
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

  if (targetFlags != 0x0)
    LOKI_ERROR << "problem converting target open flags for host. " << LOKI_HEX(targetFlags) << endl;

  return hflags;
}
