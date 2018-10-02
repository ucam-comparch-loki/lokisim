/*
 * Accelerator.cpp
 *
 *  Created on: 2 Jul 2018
 *      Author: db434
 */

#include "Accelerator.h"
#include "../../Utility/Assert.h"
#include "../AcceleratorTile.h"
#include "Configuration.h"

Accelerator::Accelerator(sc_module_name name, ComponentID id, Configuration cfg) :
    LokiComponent(name, id),
    iMulticast(MULTICAST_NETWORK_SIZE, "iMulticast"),
    oMulticast("oMulticast"),
    oReadyData(1, "oReadyData"),
    control("control", cfg),
    in1("dma_in1", ComponentID(id.tile, id.position), cfg.dma1Ports()),
    in2("dma_in2", ComponentID(id.tile, id.position+1), cfg.dma2Ports()),
    out("dma_out", ComponentID(id.tile, id.position+2), cfg.dma3Ports()),
    compute("compute", cfg),
    inputMux("mux", MULTICAST_NETWORK_SIZE) {

  // The mapping of network addresses to DMA units is currently quite awkward
  // and doesn't account for other accelerators in the same tile.
  loki_assert(ACCELERATORS_PER_TILE == 1);

  control.iClock(iClock);
  control.iParameter(paramSig);
  control.oCores(oMulticast);
  control.oReady(oReadyData[0]);
  control.oDMA1Command(toIn1);  in1.iCommand(toIn1);
  control.oDMA2Command(toIn2);  in2.iCommand(toIn2);
  control.oDMA3Command(toOut);  out.iCommand(toOut);

  toPEs1.init(compute.in1, "toPEs1");
  toPEs2.init(compute.in2, "toPEs2");
  fromPEs.init(compute.out, "fromPEs");
  compute.in1(toPEs1);  in1.oDataToPEs(toPEs1);
  compute.in2(toPEs2);  in2.oDataToPEs(toPEs2);
  compute.out(fromPEs); out.iDataFromPEs(fromPEs);
  compute.in1Ready(in1Ready); compute.in2Ready(in2Ready);
  compute.outReady(outReady);
  compute.iClock(iClock);
  compute.tick(tickSig);

  in1.iTick(tickSig); in1.oDataValid(in1Ready);
  in2.iTick(tickSig); in2.oDataValid(in2Ready);
  out.iTick(tickSig); out.oReadyForData(outReady);

  // Whenever data arrives on any channel, pass it to the control unit. This
  // will break if multiple cores attempt to communicate simultaneously.
  // Could use the muxHold signal to change this if necessary.
  muxHold.write(false);
  inputMux.iData(iMulticast);
  inputMux.oData(muxOutput);
  inputMux.iHold(muxHold);

  SC_METHOD(receiveParameter);
  sensitive << muxOutput;
  dont_initialize();

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
  parent()->magicMemoryAccess(opcode, address, returnChannel, data);
}

void Accelerator::receiveParameter() {
  loki_assert(muxOutput.valid());
  paramSig.write(muxOutput.read().payload().toUInt());
  muxOutput.ack();
}

AcceleratorTile* Accelerator::parent() const {
  return static_cast<AcceleratorTile*>(this->get_parent_object());
}
