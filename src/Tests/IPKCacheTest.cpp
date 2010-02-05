/*
 * IPKCacheTest.cpp
 *
 *  Created on: 2 Feb 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "../Pipeline/Fetch/InstructionPacketCache.h"

//TEST(IPKCacheTest, IPKCacheWorks) {
//
//  InstructionPacketCache cache("cache", 1);
//
//  sc_clock clock("clock", 1, SC_NS, 0.5);
//
//  sc_signal<bool> readSig, cacheHit, roomToFetch;
//  sc_signal<Instruction> in, out;
//  sc_buffer<Address> address;
//
//  cache.clock(clock);
//  cache.readInstruction(readSig);
//  cache.cacheHit(cacheHit);
//  cache.isRoomToFetch(roomToFetch);
//  cache.in(in);
//  cache.out(out);
//  cache.address(address);
//
//  Instruction i1("ld r4 6"), i2("ld r5 3"), i3("addu.eop r6 r4 r5");
//  Instruction i4("sll r3 r6 1"), i5("srl.eop r2 r3 1");
//  Address a1(1,1), a2(2,2);
//  Instruction temp;
//
//  address.write(a1);
//
//  TIMESTEP;
//
//  in.write(i1);
//
//  TIMESTEP;
//
////  address.write(a1);    // Makes the first packet execute again
//  EXPECT_EQ(true, roomToFetch.read());
//  in.write(i2);
//
//  TIMESTEP;
//
////  EXPECT_EQ(true, cacheHit);
//  address.write(a2);
//  in.write(i3);
//  readSig.write(!readSig.read());
//
//  TIMESTEP;
//
//  EXPECT_EQ(false, cacheHit);
//  temp = out.read();
//  EXPECT_EQ(i1, temp);
//  readSig.write(!readSig.read());
//
//  TIMESTEP;
//
//  in.write(i4);
//  temp = out.read();
//  EXPECT_EQ(i2, temp);
//  readSig.write(!readSig.read());
//
//  TIMESTEP;
//
//  address.write(a2);
//  in.write(i5);
//  temp = out.read();
//  EXPECT_EQ(i3, temp);
//  readSig.write(!readSig.read());
//
//  TIMESTEP;
//
//  EXPECT_EQ(true, cacheHit);
//  temp = out.read();
//  EXPECT_EQ(i4, temp);
//  readSig.write(!readSig.read());
//
//  TIMESTEP;
//
//  temp = out.read();
//  EXPECT_EQ(i5, temp);
//
//}
