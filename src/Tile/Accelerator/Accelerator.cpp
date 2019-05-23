/*
 * Accelerator.cpp
 *
 *  Created on: 2 Jul 2018
 *      Author: db434
 */

#include "Accelerator.h"
#include "../../Utility/Assert.h"
#include "../AcceleratorTile.h"

Accelerator::Accelerator(sc_module_name name, ComponentID id,
                         const accelerator_parameters_t& params) :
    LokiComponent(name),
    iMulticast("iMulticast", numMulticastInputs),
    oMulticast("oMulticast"),
    id(id),
    control("control", params),
    in1("dma_in1", ComponentID(id.tile, id.position), params.dma1Ports()),
    in2("dma_in2", ComponentID(id.tile, id.position+1), params.dma2Ports()),
    out("dma_out", ComponentID(id.tile, id.position+2), params.dma3Ports()),
    compute("compute", params) {

  control.clock(clock);
  iMulticast[0](control.iParameter);
  oMulticast(control.oCores);
  control.oDMA1Command(toIn1);  in1.iCommand(toIn1);
  control.oDMA2Command(toIn2);  in2.iCommand(toIn2);
  control.oDMA3Command(toOut);  out.iCommand(toOut);

  toPEs1.init("toPEs1", compute.in1);
  toPEs2.init("toPEs2", compute.in2);
  fromPEs.init("fromPEs", compute.out);
  compute.in1(toPEs1);  in1.oDataToPEs(toPEs1);
  compute.in2(toPEs2);  in2.oDataToPEs(toPEs2);
  compute.out(fromPEs); out.iDataFromPEs(fromPEs);
  compute.in1Ready(in1Ready); compute.in2Ready(in2Ready);
  compute.outReady(outReady);
  compute.clock(clock);
  compute.inputTick(inTickSig);
  compute.outputTick(outTickSig);


  in1.iTick(inTickSig); in1.oDataValid(in1Ready);
  in2.iTick(inTickSig); in2.oDataValid(in2Ready);
  out.iTick(outTickSig); out.oReadyForData(outReady);

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

ChannelIndex Accelerator::memoryAccessChannel() const {
  // Components are given IDs in the order: cores, memories, accelerators.
  // Components connect to memory in the order: cores, accelerators.
  // Need to subtract the number of memories to go from an accelerator ID to
  // a memory channel.
  return id.position - parent().numMemories();
}

AcceleratorTile& Accelerator::parent() const {
  return *(static_cast<AcceleratorTile*>(this->get_parent_object()));
}
