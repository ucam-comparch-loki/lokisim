/*
 * InstructionDecode.h
 *
 *  Created on: 9 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_INSTRUCTIONDECODE_H_
#define SRC_ISA_INSTRUCTIONDECODE_H_

#include <memory>
#include "InstructionInterface.h"
using std::shared_ptr;

typedef shared_ptr<InstructionInterface> DecodedInstruction;

DecodedInstruction decodeInstruction(Instruction encoded);

#endif /* SRC_ISA_INSTRUCTIONDECODE_H_ */
