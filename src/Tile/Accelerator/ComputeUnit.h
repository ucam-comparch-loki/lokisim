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
#include "AdderTree.h"
#include "Configuration.h"
#include "Multiplier.h"

template<typename T>
class ComputeUnit: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  LokiVector2D<sc_in<T>> in1;
  LokiVector2D<sc_in<T>> in2;
  LokiVector2D<sc_out<T>> out;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  ComputeUnit(sc_module_name name, const Configuration& config) :
      LokiComponent(name),
      in1(config.dma1Ports().width, config.dma1Ports().height, "in1"),
      in2(config.dma2Ports().width, config.dma2Ports().height, "in2"),
      out(config.dma3Ports().width, config.dma3Ports().height, "out"),
      multipliers(config.peArraySize().width,
                  vector<Multiplier<T>*>(config.peArraySize().height, NULL)) {

    // Use this a lot of times, so create a shorter name.
    size2d_t PEs = config.peArraySize();

    for (uint col=0; col<PEs.width; col++) {
      for (uint row=0; row<PEs.height; row++) {
        Multiplier<T>* mul = new Multiplier<T>(sc_gen_unique_name("mul"), 1, 1);
        multipliers[col][row] = mul;
      }
    }

    // Connect in1.
    for (uint col=0; col<PEs.width; col++) {
      if (config.broadcastRows()) {
        loki_assert(config.dma1Ports().width == 1);

        for (uint row=0; row<PEs.height; row++)
          multipliers[col][row]->in1(in1[0][row]);
      }
      else {
        for (uint row=0; row<PEs.height; row++)
          multipliers[col][row]->in1(in1[col][row]);
      }
    }

    // Connect in2.
    for (uint col=0; col<PEs.width; col++) {
      if (config.broadcastCols()) {
        loki_assert(config.dma2Ports().height == 1);

        for (uint row=0; row<PEs.height; row++)
          multipliers[col][row]->in2(in2[col][0]);
      }
      else {
        for (uint row=0; row<PEs.height; row++)
          multipliers[col][row]->in2(in2[col][row]);
      }
    }

    // Connect out.
    // Direct connection from each PE.
    if (!config.accumulateCols() && !config.accumulateRows())
      for (uint col=0; col<PEs.width; col++)
        for (uint row=0; row<PEs.height; row++)
          multipliers[col][row]->out(out[col][row]);

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
          multipliers[col][row]->out(*signal);
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
          multipliers[col][row]->out(*signal);
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
          multipliers[col][row]->out(*signal);
          signals.push_back(signal);
        }
      }

      AdderTree<T>* adder =
          new AdderTree<T>(sc_gen_unique_name("adders"), adders.size(), 1, 1);

      for (uint i=0; i<adders.size(); i++) {
        sc_signal<T>* signal = new sc_signal<T>(sc_gen_unique_name("sig"));
        adder->in[i](*signal);
        adders[i]->out(*signal);
        signals.push_back(signal);
      }

      adders.push_back(adder);
      adder->out(out[0][0]);
    }
  }

  ~ComputeUnit() {
    for (uint i=0; i<multipliers.size(); i++)
      for (uint j=0; j<multipliers[i].size(); j++)
        delete multipliers[i][j];

    for (uint i=0; i<adders.size(); i++)
      delete adders[i];

    for (uint i=0; i<signals.size(); i++)
      delete signals[i];
  }

//============================================================================//
// Local state
//============================================================================//

private:

  vector<vector<Multiplier<T>*>> multipliers;
  vector<AdderTree<T>*> adders;
  vector<sc_signal<T>*> signals;

};

#endif /* SRC_TILE_ACCELERATOR_COMPUTEUNIT_H_ */
