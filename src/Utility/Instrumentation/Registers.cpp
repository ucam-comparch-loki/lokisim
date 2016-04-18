/*
 * Registers.cpp
 *
 *  Created on: 21 Jan 2011
 *      Author: db434
 */

// Pull in macros for min/max of various datatypes from stdint.h.
#define __STDC_LIMIT_MACROS

#include "Registers.h"

using namespace Instrumentation;

count_t Registers::numForwards_ = 0;

bool    Registers::detailedLogging = false;

count_t Registers::operations[3];
count_t Registers::popCount[3];
count_t Registers::hammingDist[3];
count_t Registers::bypasses[3];
count_t Registers::cyclesActive = 0;

// Call init() to create arrays of the appropriate length, once all of the
// parameter values have been decided.
count_t* Registers::writesPerReg = NULL;
count_t* Registers::readsPerReg = NULL;

// The sizes of the values read and written.
// Index 0 = writes, 1 = read port 1, 2 = read port 2
count_t Registers::zero[3],
        Registers::uint8[3], Registers::uint16[3], Registers::uint24[3], Registers::uint32[3],
        Registers::int8[3],  Registers::int16[3],  Registers::int24[3],  Registers::int32[3];

void Registers::forward(PortIndex port)               {numForwards_++;}

void Registers::init() {
  writesPerReg = new count_t[NUM_PHYSICAL_REGISTERS];
  readsPerReg  = new count_t[NUM_PHYSICAL_REGISTERS];

  numForwards_ = cyclesActive = 0;
  memset(operations,  0, 3 * sizeof(count_t));
  memset(popCount,    0, 3 * sizeof(count_t));
  memset(hammingDist, 0, 3 * sizeof(count_t));
  memset(bypasses,    0, 3 * sizeof(count_t));
  memset(zero,        0, 3 * sizeof(count_t));
  memset(uint8,       0, 3 * sizeof(count_t));
  memset(uint16,      0, 3 * sizeof(count_t));
  memset(uint24,      0, 3 * sizeof(count_t));
  memset(uint32,      0, 3 * sizeof(count_t));
  memset(int8,        0, 3 * sizeof(count_t));
  memset(int16,       0, 3 * sizeof(count_t));
  memset(int24,       0, 3 * sizeof(count_t));
  memset(int32,       0, 3 * sizeof(count_t));
}

void Registers::end() {
  delete[] writesPerReg;
  delete[] readsPerReg;
}

void Registers::write(RegisterIndex reg, int oldData, int newData) {
  operations[WR]++;
  popCount[WR] += popcount(newData);
  hammingDist[WR] += hammingDistance(oldData, newData);

  if(detailedLogging) {
    writesPerReg[reg]++;
    valueSize(WR, newData);
  }
}

void Registers::read(PortIndex port, RegisterIndex reg, int oldData, int newData) {
  operations[port]++;
  popCount[port] += popcount(newData);
  hammingDist[port] += hammingDistance(oldData, newData);

  if(detailedLogging) {
    readsPerReg[reg]++;
    valueSize(port, newData);
  }
}

void Registers::activity() {
  cyclesActive++;
}

void Registers::bypass(PortIndex port) {
  bypasses[port]++;
}

// stdint.h (understandably) doesn't have the range for 24 bit values.
#define INT24_MAX (8388607)
#define INT24_MIN (-8388607-1)
#define UINT24_MAX (0x00FFFFFF)

void Registers::valueSize(PortIndex port, int32_t data) {
  if(data == 0) {
    zero[port]++;
    return;
  }

  // Treat data as signed.
  if(data >= INT8_MIN && data <= INT8_MAX)
    int8[port]++;
  else if(data >= INT16_MIN && data <= INT16_MAX)
    int16[port]++;
  else if(data >= INT24_MIN && data <= INT24_MAX)
    int24[port]++;
  else
    int32[port]++;

  // Treat data as unsigned.
  uint32_t udata = (uint32_t)data;
  if(udata <= UINT8_MAX)
    uint8[port]++;
  else if(udata <= UINT16_MAX)
    uint16[port]++;
  else if(udata <= UINT24_MAX)
    uint24[port]++;
  else
    uint32[port]++;
}

count_t Registers::numReads()                   {return operations[RD1] + operations[RD2];}
count_t Registers::numWrites()                  {return operations[WR];}
count_t Registers::numForwards()                {return numForwards_;}

count_t Registers::numReads(RegisterIndex reg)  {return readsPerReg[reg];}
count_t Registers::numWrites(RegisterIndex reg) {return writesPerReg[reg];}

void Registers::printStats() {
  if(numReads() == 0 && numWrites() == 0) return;

  cout <<
    "Registers:\n" <<
    "  Reads:    " << numReads() << "\n" <<
    "  Writes:   " << numWrites() << "\n" <<
    "  Forwards: " << numForwards() << "\t(" << percentage(numForwards(),numReads()) << ")\n";
}

void Registers::dumpEventCounts(std::ostream& os) {
  os << "<registers rdports=\"2\" wrports=\"1\" entries=\"32\" width=\"32\">\n"
     << xmlNode("instances", NUM_CORES)     << "\n"
     << xmlNode("active", cyclesActive)     << "\n"
     << xmlNode("w_op", operations[WR])     << "\n"
     << xmlNode("w_oc", popCount[WR])       << "\n"
     << xmlNode("w_hd", hammingDist[WR])    << "\n"
     << xmlNode("rd1_op", operations[RD1])  << "\n"
     << xmlNode("rd1_oc", popCount[RD1])    << "\n"
     << xmlNode("rd1_hd", hammingDist[RD1]) << "\n"
     << xmlNode("by1", bypasses[RD1])       << "\n"
     << xmlNode("rd2_op", operations[RD2])  << "\n"
     << xmlNode("rd2_oc", popCount[RD2])    << "\n"
     << xmlNode("rd2_hd", hammingDist[RD2]) << "\n"
     << xmlNode("by2", bypasses[RD2])       << "\n"
     << xmlEnd("registers")                 << "\n";

//  if(!detailedLogging)
//    return;
//
//  os << "data type\twrites\tread 1\tread 2\n"
//        "zero\t\t"   << zero[WR]   << "\t" << zero[RD1]   << "\t" << zero[RD2]   << "\n"
//        "uint8\t\t"  << uint8[WR]  << "\t" << uint8[RD1]  << "\t" << uint8[RD2]  << "\n"
//        "uint16\t\t" << uint16[WR] << "\t" << uint16[RD1] << "\t" << uint16[RD2] << "\n"
//        "uint24\t\t" << uint24[WR] << "\t" << uint24[RD1] << "\t" << uint24[RD2] << "\n"
//        "uint32\t\t" << uint32[WR] << "\t" << uint32[RD1] << "\t" << uint32[RD2] << "\n"
//        "int8\t\t"   << int8[WR]   << "\t" << int8[RD1]   << "\t" << int8[RD2]   << "\n"
//        "int16\t\t"  << int16[WR]  << "\t" << int16[RD1]  << "\t" << int16[RD2]  << "\n"
//        "int24\t\t"  << int24[WR]  << "\t" << int24[RD1]  << "\t" << int24[RD2]  << "\n"
//        "int32\t\t"  << int32[WR]  << "\t" << int32[RD1]  << "\t" << int32[RD2]  << "\n"
//        "\n";
}
