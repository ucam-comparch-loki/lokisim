/*
 * SystemCall.cpp
 *
 *  Created on: Aug 19, 2019
 *      Author: db434
 */

#include "SystemCall.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../../Exceptions/InvalidOptionException.h"
#include "../../Utility/Instrumentation.h"
#include "Core.h"

namespace Compute {

SystemCall::SystemCall(sc_module_name name) : LokiComponent(name) {
  // Nothing
}

void SystemCall::call(Code code) {
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

  switch (code) {
    case SYS_EXIT: {
      RETURN_CODE = readRegister(arg1Reg);
      std::cerr << "Simulation ended with sys_exit (arg " << (uint)RETURN_CODE << ") after "
                << Instrumentation::currentCycle() << " cycles." << endl;
      Instrumentation::endExecution();
      break;
    }

    case SYS_OPEN: {
      char fname[1024];
      int start = readRegister(arg1Reg);
      int mode = (int)convertTargetFlags((unsigned int)readRegister(arg2Reg));
      int perm = (int)readRegister(arg3Reg);

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

      writeRegister(result1Reg, fd);
      break;
    }

    case SYS_CLOSE: {
      int fd = readRegister(arg1Reg);

      // Don't allow simulated program to close stdio pipes.
      if (fd <= 2) {
        fsync(fd);
        writeRegister(result1Reg, 0);
      }
      else
        writeRegister(result1Reg, close(fd));

      break;
    }

    case SYS_READ: {
      int fd = readRegister(arg1Reg);
      int start = readRegister(arg2Reg);
      unsigned int len = (unsigned int)readRegister(arg3Reg);

      char* buf = (char*)malloc(len);
      writeRegister(result1Reg, read(fd, buf, len));

      for (uint i=0; i < len; i++) {
        writeByte(start+i, buf[i]);
      }

      free(buf);
      break;
    }

    case SYS_WRITE: {
      int fd = readRegister(arg1Reg);
      int start = readRegister(arg2Reg);
      unsigned int len = (unsigned int)readRegister(arg3Reg);
      char *str = (char*)malloc(len);

      // Copy string out of memory.
      for (uint i=0; i < len; i++) {
        str[i] = readByte(start + i);
      }

      writeRegister(result1Reg, write(fd, str, len));
      free(str);
      break;
    }

    case SYS_SEEK: {
      int fd = readRegister(arg1Reg);
      int offset = readRegister(arg2Reg);
      int whence = readRegister(arg3Reg);
      writeRegister(result1Reg, lseek(fd, offset, whence));
      break;
    }

    case SYS_TILE_ID: {
      LOKI_WARN << "syscall 0x10 (tile ID) is deprecated. Use control register 1 instead." << endl;
      int tile = id().tile.flatten(Encoding::softwareTileID);
      writeRegister(result1Reg, tile);
      break;
    }
    case SYS_POSITION: {
      LOKI_WARN << "syscall 0x11 (core ID) is deprecated. Use control register 1 instead." << endl;
      int position = id().position;
      writeRegister(result1Reg, position);
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
      writeRegister(result1Reg, cycle >> 32);
      writeRegister(result2Reg, cycle & 0xFFFFFFFF);
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
uint SystemCall::convertTargetFlags(uint targetFlags) const {
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

Core& SystemCall::core() const {
  return static_cast<Core&>(*(this->get_parent_object()));
}

ComponentID SystemCall::id() const {
  return core().id;
}

int32_t SystemCall::readRegister(RegisterIndex reg) const {
  return core().debugRegisterRead(reg);
}
int32_t SystemCall::readByte(MemoryAddr addr) const {
  return core().debugReadByte(addr);
}

void SystemCall::writeRegister(RegisterIndex reg, int32_t data) const {
  core().debugRegisterWrite(reg, data);
}
void SystemCall::writeByte(MemoryAddr addr, int32_t data) const {
  core().debugWriteByte(addr, data);
}

} /* namespace Compute */
