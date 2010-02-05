/*
 * ALUTest.cpp
 *
 * Class responsible for testing the behaviour of the ALU. Currently tests
 * each operation on a simple pair of operands. Testing could be more thorough
 * but it seems as though this should be enough.
 *
 * All tests but one are commented out until I can get Google Test and SystemC
 * working together nicely.
 *
 *  Created on: 20 Jan 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include <systemc.h>
#include "Test.h"
#include "../Pipeline/Execute/ALU.h"
#include "../InstructionMap.h"

/* The routine needed to test each operation:
 *  Select the operand
 *  Simulate for a unit of time, so the signal can get through the ALU
 *  Read the result
 *  Make sure the result is correct */
#define OPTEST(op, expected) select.write(InstructionMap::op);\
  TIMESTEP;\
  d = out.read();\
  EXPECT_EQ(expected, d.getData())

namespace {

// The fixture for testing the ALU
class ALUTest : public ::testing::Test {
protected:

  ALU *alu;
  sc_signal<Data> in1, in2, out;
  sc_signal<short> select;

  Data d;     // Used to store temporary values in

  ALUTest() {
    alu = new ALU("alu", 0);
    alu->in1(in1); alu->in2(in2); alu->out(out); alu->select(select);
  }

  virtual void SetUp() {
    Data *d1 = new Data(8), *d2 = new Data(7);
    in1.write(*d1); in2.write(*d2);
  }

  // Objects declared here can be used by all tests in the test case for Foo.
};

// Test that the ALU shift operations work correctly
//TEST_F(ALUTest, ShiftOperationsWorkCorrectly) {
//
//  OPTEST(SLL, 1024);      // 8 << 7 = 1024
//
//  in1.write(d);
//  OPTEST(SRL, 8);         // 1024 >> 7 = 8
//
//  Data *veryLarge = new Data(0xFFFFFFFF);
//  in1.write(*veryLarge);
//  OPTEST(SRA, 0xFFFFFFFF);  // 0xFFFFFFFF >>> 7 = 0xFFFFFFFF
//
//}

// Test that the ALU comparison operations work correctly
//TEST_F(ALUTest, ComparisonOperationsWorkCorrectly) {
//
//  OPTEST(SEQ, 0);         // (8 == 7) = 0
//
//  OPTEST(SNE, 1);         // (8 != 7) = 1
//
//  OPTEST(SLTU, 0);        // (8 < 7) = 0
//
//  Data *negative = new Data(-1);
//  in1.write(*negative);
//  OPTEST(SLT, 1);         // (-1 < 7) = 1
//
//}

// Test lui, psel and clz

// Test that the ALU bitwise operations work correctly
//TEST_F(ALUTest, BitwiseOperationsWorkCorrectly) {
//
//  OPTEST(NOR, 0xFFFFFFF0);  // ~(8 | 7) = all 1s apart from last 4 bits
//
//  OPTEST(AND, 0);           // 8 & 7 = 1000 & 0111 = 0
//
//  OPTEST(OR, 15);           // 8 | 7 = 1111 = 15
//
//  OPTEST(XOR, 15);          // 8 ^ 7 = 1111 = 15
//
//}

// Test that the ALU optional operations work correctly
//TEST_F(ALUTest, OptionalOperationsWorkCorrectly) {
//
//  OPTEST(NAND, -1);         // ~(8 & 7) = ~0 =  all 1s
//
//  OPTEST(CLR, 8);           // 8 & ~7 = 1000 & 11111000 = 8
//
//  OPTEST(ORC, 0xFFFFFFF8);  // 8 | ~7 = 1000 | 11111000 = 0xFFFFFFF8
//
//  //OPTEST(POPC, 1);        // 8 = 1000 => one 1
//
//  OPTEST(RSUBI, -1);        // 7 - 8 = -1
//
//}

// Test that the ALU arithmetic operations work correctly
//TEST_F(ALUTest, ArithmeticOperationsWorkCorrectly) {
//
//  OPTEST(ADDU, 15);         // 8 + 7 = 15
//
//  OPTEST(SUBU, 1);          // 8 - 7 = 1
//
//  OPTEST(MULHW, 0);         // Probably want a better test than 8*7
//
//  OPTEST(MULLW, 56);        // 8 * 7 = 56
//
////  OPTEST(MULHWU, 0);        // Probably want a better test
//
//}

}  // namespace
