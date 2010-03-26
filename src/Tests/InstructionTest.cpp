/*
 * InstructionTest.cpp
 *
 *  Created on: 25 Jan 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "../Datatype/Instruction.h"
#include "../Utility/InstructionMap.h"


/* Tests that Instructions are parsed correctly */
TEST(InstructionTest, InstructionParsesCorrectly) {

  std::string inst = "p?addu.p r18 r25 ch1  ; this is a comment";
  Instruction i = Instruction(inst);

  EXPECT_EQ(InstructionMap::ADDU, i.getOp());
  EXPECT_EQ(18, i.getDest());
  EXPECT_EQ(25, i.getSrc1());
  EXPECT_EQ(29, i.getSrc2());
  EXPECT_EQ(Instruction::P, i.getPredicate());
  EXPECT_EQ(true, i.getSetPredicate());

  inst = "ld.eop r18 -678 > 41  ; this is a comment";
  Instruction j = Instruction(inst);

  EXPECT_EQ(InstructionMap::LD, j.getOp());
  EXPECT_EQ(18, j.getDest());
  EXPECT_EQ(-678, j.getImmediate());
  EXPECT_EQ(41, j.getRchannel());
  EXPECT_EQ(Instruction::END_OF_PACKET, j.getPredicate());
  EXPECT_EQ(false, j.getSetPredicate());

}
