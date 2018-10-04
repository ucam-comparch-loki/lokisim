/*
 * ComputeUnit.h
 *
 *  Created on: 16 Aug 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_COMPUTEUNIT_H_
#define SRC_TILE_ACCELERATOR_COMPUTEUNIT_H_

#include "../../LokiComponent.h"
#include "../../Utility/Assert.h"
#include "../../Utility/LokiVector2D.h"
#include "AdderTree.h"
#include "Configuration.h"
#include "Multiplier.h"

template<typename T>
class ComputeUnit: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput iClock;

  // The data being computed.
  LokiVector2D<sc_in<T>> in1;
  LokiVector2D<sc_in<T>> in2;
  LokiVector2D<sc_out<T>> out;

  // The input from another component is ready to be consumed.
  ReadyInput  in1Ready;
  ReadyInput  in2Ready;

  // The component receiving the output is ready to consume new data.
  ReadyInput  outReady;

  // The current tick being executed. This signal also acts as flow control,
  // with any connected components only supplying data when the tick reaches a
  // predetermined value.
  // TODO: need a separate input and output tick?
  sc_out<uint> tick;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(ComputeUnit<T>);

  ComputeUnit(sc_module_name name, const Configuration& config) :
      LokiComponent(name),
      iClock("clock"),
      in1("in1", config.dma1Ports().width, config.dma1Ports().height),
      in2("in2", config.dma2Ports().width, config.dma2Ports().height),
      out("out", config.dma3Ports().width, config.dma3Ports().height),
      tick("tick") {

    currentTick = 0;
    tickSignal.write(currentTick);
    tick.initialize(currentTick);

    // Use this a lot of times, so create a shorter name.
    size2d_t PEs = config.peArraySize();

    multipliers.init(PEs.width);
    for (uint col=0; col<PEs.width; col++) {
      for (uint row=0; row<PEs.height; row++) {
        Multiplier<T>* mul = new Multiplier<T>(sc_gen_unique_name("mul"), 1, 1);
        multipliers[col].push_back(mul);

        mul->tick(tickSignal);
      }
    }

    // Connect in1.
    for (uint col=0; col<PEs.width; col++) {
      if (config.broadcastRows()) {
        loki_assert(config.dma1Ports().width == 1);

        for (uint row=0; row<PEs.height; row++)
          multipliers[col][row].in1(in1[0][row]);
      }
      else {
        for (uint row=0; row<PEs.height; row++)
          multipliers[col][row].in1(in1[col][row]);
      }
    }

    // Connect in2.
    for (uint col=0; col<PEs.width; col++) {
      if (config.broadcastCols()) {
        loki_assert(config.dma2Ports().height == 1);

        for (uint row=0; row<PEs.height; row++)
          multipliers[col][row].in2(in2[col][0]);
      }
      else {
        for (uint row=0; row<PEs.height; row++)
          multipliers[col][row].in2(in2[col][row]);
      }
    }

    // Connect out.
    // Direct connection from each PE.
    if (!config.accumulateCols() && !config.accumulateRows())
      for (uint col=0; col<PEs.width; col++)
        for (uint row=0; row<PEs.height; row++)
          multipliers[col][row].out(out[col][row]);

    // Adders along columns.
    if (config.accumulateCols() && !config.accumulateRows()) {
      for (uint col=0; col<PEs.width; col++) {
        AdderTree<T>* adder =
            new AdderTree<T>(sc_gen_unique_name("adders"), PEs.height, 1, 1);
        adders.push_back(adder);
        adder->out(out[col][0]);

        for (uint row=0; row<PEs.height; row++) {
          sc_signal<T>* signal = new sc_signal<T>(sc_gen_unique_name("sig"));
          adder->in[row](*signal);
          multipliers[col][row].out(*signal);
          signals.push_back(signal);
        }
      }
    }

    // Adders along rows.
    if (!config.accumulateCols() && config.accumulateRows()) {
      for (uint row=0; row<PEs.height; row++) {
        AdderTree<T>* adder =
            new AdderTree<T>(sc_gen_unique_name("adders"), PEs.width, 1, 1);
        adders.push_back(adder);
        adder->out(out[0][row]);

        for (uint col=0; col<PEs.width; col++) {
          sc_signal<T>* signal = new sc_signal<T>(sc_gen_unique_name("sig"));
          adder->in[col](*signal);
          multipliers[col][row].out(*signal);
          signals.push_back(signal);
        }
      }
    }

    // Adders along both columns and rows.
    if (config.accumulateCols() && config.accumulateRows()) {
      for (uint col=0; col<PEs.width; col++) {
        AdderTree<T>* adder =
            new AdderTree<T>(sc_gen_unique_name("adders"), PEs.height, 1, 1);
        adders.push_back(adder);

        for (uint row=0; row<PEs.height; row++) {
          sc_signal<T>* signal = new sc_signal<T>(sc_gen_unique_name("sig"));
          adder->in[row](*signal);
          multipliers[col][row].out(*signal);
          signals.push_back(signal);
        }
      }

      AdderTree<T>* adder =
          new AdderTree<T>(sc_gen_unique_name("adders"), adders.size(), 1, 1);

      for (uint i=0; i<adders.size(); i++) {
        sc_signal<T>* signal = new sc_signal<T>(sc_gen_unique_name("sig"));
        adder->in[i](*signal);
        adders[i].out(*signal);
        signals.push_back(signal);
      }

      adders.push_back(adder);
      adder->out(out[0][0]);
    }

    SC_METHOD(newTick);
    sensitive << iClock.pos();
    // do initialise
  }

//============================================================================//
// Methods
//============================================================================//

private:

  void newTick() {
    // If output can't receive data, wait for it
    if (!outReady.read())
      next_trigger(outReady.posedge_event());

    // Else if input doesn't have new data, wait for it
    else if (!in1Ready.read())
      next_trigger(in1Ready.posedge_event());
    else if (!in2Ready.read())
      next_trigger(in2Ready.posedge_event());

    // Else compute
    else {
      triggerCompute();

      // TODO: account for latency. Perhaps put the output tick signal through
      // the same latency buffers as the data?
    }

  }

  void triggerCompute() {
    // Need separate signals for external and internal components?
    tickSignal.write(currentTick);
    tick.write(currentTick);

    currentTick++;
  }

//============================================================================//
// Local state
//============================================================================//

private:

  LokiVector2D<Multiplier<T>> multipliers;
  LokiVector<AdderTree<T>> adders;
  LokiVector<sc_signal<T>> signals;

  sc_signal<uint> tickSignal;
  uint currentTick;

};

#endif /* SRC_TILE_ACCELERATOR_COMPUTEUNIT_H_ */
