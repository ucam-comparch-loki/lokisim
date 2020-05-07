/*
 * Accelerator.cpp
 *
 *  Created on: 2 Jul 2018
 *      Author: db434
 */

#include "Accelerator.h"
#include "../../Utility/Assert.h"
#include "../AcceleratorTile.h"

Accelerator::Accelerator(sc_module_name name, ComponentID id, uint numMemoryBanks,
                         const accelerator_parameters_t& params) :
    LokiComponent(name),
    iMulticast("iMulticast", numMulticastInputs),
    oMulticast("oMulticast"),
    iMemory("iMemory", 3, numMemoryBanks),
    oMemory("oMemory", 3, numMemoryBanks),
    id(id),
    in1("dma_in1", ComponentID(id.tile, id.position), params.dma1Ports(), numMemoryBanks),
    in2("dma_in2", ComponentID(id.tile, id.position+1), params.dma2Ports(), numMemoryBanks),
    out("dma_out", ComponentID(id.tile, id.position+2), params.dma3Ports(), numMemoryBanks),
    control("control", params),
    compute("compute", params),
    toPEs1(params.dma1Ports()),
    toPEs2(params.dma2Ports()),
    fromPEs(params.dma3Ports()) {

  checkParameters(params);

  control.clock(clock);
  iMulticast[0](control.iParameter);
  oMulticast(control.oCores);
  control.oDMA1Command(toIn1);  in1.iCommand(toIn1);
  control.oDMA2Command(toIn2);  in2.iCommand(toIn2);
  control.oDMA3Command(toOut);  out.iCommand(toOut);

  compute.clock(clock);
  compute.in1(toPEs1);  in1.oDataToPEs(toPEs1);
  compute.in2(toPEs2);  in2.oDataToPEs(toPEs2);
  compute.out(fromPEs); out.iDataFromPEs(fromPEs);

  in1.clock(clock);           in2.clock(clock);
  out.clock(clock);

  iMemory[0](in1.iResponse);  oMemory[0](in1.oRequest);
  iMemory[1](in2.iResponse);  oMemory[1](in2.oRequest);
  iMemory[2](out.iResponse);  oMemory[2](out.oRequest);

}

void Accelerator::deliverDataInternal(const NetworkData& flit) {
  // Each DMA has a unique address. Subtract the base address of this
  // accelerator to determine which DMA is being addressed.
  int dma = flit.channelID().component.position - id.position;

  switch (dma) {
    case 0:
      in1.deliverDataInternal(flit);
      break;
    case 1:
      in2.deliverDataInternal(flit);
      break;
    case 2:
      out.deliverDataInternal(flit);
      break;
    default:
      throw InvalidOptionException("Accelerator DMA address", dma);
      break;
  }
}

void Accelerator::magicMemoryAccess(MemoryOpcode opcode, MemoryAddr address,
                                    ChannelID returnChannel, Word data) {
  parent().magicMemoryAccess(opcode, address, returnChannel, data);
}

bool Accelerator::isIdle() const {
  // Should also check whether any compute stage is active?
  // I think this is okay because the output requests are queued up well in
  // advance.
  return in1.isIdle() && in2.isIdle() && out.isIdle();
}

void Accelerator::checkParameters(const accelerator_parameters_t& params) {
	// Broadcasting along both rows and columns means we're just doing the same
	// computation on every PE.
	if (params.broadcastCols && params.broadcastRows)
		LOKI_WARN << this->name() << " configured to broadcast on both rows and columns.\n"
		    "This will result in every PE performing the same computation." << endl;

	// It doesn't make sense to broadcast and accumulate along the same dimension.
	// This is equivalent to multiplying the single result by whatever value was
	// broadcast.
	// i.e. a*X + b*X + c*X + d*X = (a+b+c+d)*X, so why do so many multiplies?
	if ((params.broadcastCols && params.accumulateCols) ||
			(params.broadcastRows && params.accumulateRows))
		LOKI_WARN << this->name() << " configured to broadcast and accumulate on same dimension.\n"
		    "It would make more sense to multiply the final result by the broadcasted value." << endl;
}

const sc_event& Accelerator::finishedComputationEvent() const {
  // TODO: This assumes that the output DMA becomes completely inactive.
  // This may not be the case if another convolution is queued up to execute
  // immediately.
  return out.becameIdleEvent();
}

ChannelIndex Accelerator::memoryAccessChannel(ComponentID id) const {
  // Components are given IDs in the order: cores, memories, accelerators.
  // Components connect to memory in the order: cores, accelerators.
  // Need to subtract the number of memories to go from an accelerator ID to
  // a memory channel.
  return id.position - parent().numMemories();
}

AcceleratorTile& Accelerator::parent() const {
  return *(static_cast<AcceleratorTile*>(this->get_parent_object()));
}
