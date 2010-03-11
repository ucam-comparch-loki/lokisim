/*
 * RegisterTest.cpp
 *
 *  Created on: 26 Jan 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "../TileComponents/Pipeline/IndirectRegisterFile.h"

/* Tests that all of the register ports and the internal representation work
 * correctly */
//TEST(RegisterTest, RegistersWorkCorrectly) {
//
//  IndirectRegisterFile regs("regs", 1);
//  Data d1(1000000);
//  Data d2(7);
//  Data d3(1);
//
//  sc_buffer<Word> inData, out1, out2;
//  sc_buffer<short> indWrite, read1, read2, write;
//  sc_buffer<bool> isIndirect;
//
//  regs.indWriteAddr(indWrite);
//  regs.readAddr1(read1);
//  regs.readAddr2(read2);
//  regs.indRead(isIndirect);
//  regs.writeAddr(write);
//  regs.writeData(inData);
//  regs.out1(out1);
//  regs.out2(out2);
//
//  TIMESTEP;
//
//  // Test writing
//  write.write(1); // Should be 1, but rewriting doesn't work yet
//  inData.write(d3); // Put 1 at position 0
//
//  TIMESTEP;
//
//  // Test rewriting
//  write.write(1);
//  inData.write(d2); // Put 7 at position 1 (0) (see if it works when not changing addr)
//
//  TIMESTEP;
//
//  // Test reading
//  read1.write(1);
//
//  TIMESTEP;
//
//  ASSERT_EQ(7, (static_cast<Data>(out1.read()).getData())) << "Rewriting didn't work.";
//
//  // Test indirect writes
//  indWrite.write(1);  // Should correspond to position 7
//  inData.write(d1);   // Put 1000000 at position 7
//
//  TIMESTEP;
//
//  read1.write(7);     // Read from position 7 (should give 1000000)
//  write.write(0);
//  inData.write(d3);   // Write 1 to position 0 (1)
//
//  TIMESTEP;
//
//  EXPECT_EQ(1000000, (static_cast<Data>(out1.read()).getData()));
//
//  // Test indirect reads
//  isIndirect.write(true);
//  read2.write(1);   // Should correspond to position 7
//
//  TIMESTEP;
//
//  EXPECT_EQ(1000000, (static_cast<Data>(out2.read()).getData()));
//
//  // Test two writes at once
//  isIndirect.write(false);
//  read1.write(0);     // Should give 1
//  read2.write(7);     // Should give 1000000
//
//  TIMESTEP;
//
//  EXPECT_EQ(1, (static_cast<Data>(out1.read()).getData()));
//  EXPECT_EQ(1000000, (static_cast<Data>(out2.read()).getData()));
//
//}
