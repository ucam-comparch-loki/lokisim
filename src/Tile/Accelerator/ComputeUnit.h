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
#include "AcceleratorTypes.h"
#include "AdderTree.h"
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
  // We have separate signals for inputs and outputs because the computation
  // may be pipelined, leaving the output a few ticks behind the input.
  sc_out<tick_t> inputTick, outputTick;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(ComputeUnit<T>);

  ComputeUnit(sc_module_name name, const accelerator_parameters_t& params) :
      LokiComponent(name),
      iClock("clock"),
      in1("in1", params.dma1Ports().width, params.dma1Ports().height),
      in2("in2", params.dma2Ports().width, params.dma2Ports().height),
      out("out", params.dma3Ports().width, params.dma3Ports().height),
      in1Ready("in1Ready"),
      in2Ready("in2Ready"),
      outReady("outReady"),
      inputTick("inputTick"),
      outputTick("outputTick") {

    currentTick = 0;
    inputTick.initialize(currentTick);

    // Use this a lot of times, so create a shorter name.
    size2d_t PEs = params.numPEs;

    multipliers.init(PEs.width);
    for (uint col=0; col<PEs.width; col++) {
      for (uint row=0; row<PEs.height; row++) {
        Multiplier<T>* mul = new Multiplier<T>(sc_gen_unique_name("mul"), 1, 1);
        multipliers[col].push_back(mul);
      }
    }

    // Connect in1.
    for (uint col=0; col<PEs.width; col++) {
      if (params.broadcastRows) {
        loki_assert(params.dma1Ports().width == 1);

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
      if (params.broadcastCols) {
        loki_assert(params.dma2Ports().height == 1);

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
    if (!params.accumulateCols && !params.accumulateRows)
      for (uint col=0; col<PEs.width; col++)
        for (uint row=0; row<PEs.height; row++)
          multipliers[col][row].out(out[col][row]);

    // Adders along columns.
    if (params.accumulateCols && !params.accumulateRows) {
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
    if (!params.accumulateCols && params.accumulateRows) {
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
    if (params.accumulateCols && params.accumulateRows) {
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

    SC_METHOD(newOutputTick);
    sensitive << outputReadyEvent;
    dont_initialize();
  }

//============================================================================//
// Methods
//============================================================================//

private:

  void newTick() {
    // If output can't receive data, wait for it
    if (!outReady.read()) {
      LOKI_LOG << this->name() << ": waiting for output to become free" << endl;
      next_trigger(outReady.posedge_event());
    }

    // Else if input doesn't have new data, wait for it
    else if (!in1Ready.read()) {
      LOKI_LOG << this->name() << ": waiting for input 1 to supply data" << endl;
      next_trigger(in1Ready.posedge_event());
    }
    else if (!in2Ready.read()) {
      LOKI_LOG << this->name() << ": waiting for input 2 to supply data" << endl;
      next_trigger(in2Ready.posedge_event());
    }

    // Else compute
    else {
      LOKI_LOG << this->name() << ": executing tick " << currentTick << endl;
      triggerCompute();
    }

  }

  void triggerCompute() {
    // Need separate signals for external and internal components?
    inputTick.write(currentTick);

    // TODO: account for latency of multipliers and adder trees.
    // TODO: ensure that this approach works for long latencies, when multiple
    // notifications can be pending simultaneously.
    outputReadyEvent.notify(0.1, sc_core::SC_NS);

    currentTick++;
  }

  // Signal that the output for the given tick is available on the `out` ports.
  void newOutputTick() {
    // Can't use currentTick because it may have moved on.
    outputTick.write(outputTick.read() + 1);
  }

//============================================================================//
// Local state
//============================================================================//

private:

  LokiVector2D<Multiplier<T>> multipliers;
  LokiVector<AdderTree<T>> adders;
  LokiVector<sc_signal<T>> signals;

  tick_t currentTick;

  sc_event outputReadyEvent;

};

#endif /* SRC_TILE_ACCELERATOR_COMPUTEUNIT_H_ */
