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
      multipliers(config.peArraySize().width, vector<Multiplier<T>*>(config.peArraySize().height, NULL)) {

    // Use this a lot of times, so create a shorter name.
    size2d_t PEs = config.peArraySize();

    for (uint col=0; col<PEs.width; col++) {
      for (uint row=0; row<PEs.height; row++) {
        Multiplier<T>* mul = new Multiplier<T>(1, 1);
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
          multipliers[col][row]->in1(in1[col][0]);
      }
      else {
        for (uint row=0; row<PEs.height; row++)
          multipliers[col][row]->in1(in1[col][row]);
      }
    }

    // Connect out.
    // Direct connection from each PE.
    if (!config.accumulateCols() && !config.accumulateRows())
      for (uint col=0; col<PEs.width; col++)
        for (uint row=0; row<PEs.height; row++)
          out[col][row](multipliers[col][row]->out);

    // Adders along columns.
    if (config.accumulateCols() && !config.accumulateRows()) {
      for (uint col=0; col<PEs.width; col++) {
        AdderTree<T>* adder = new AdderTree<T>(PEs.height, 1, 1);
        adders.push_back(adder);
        out[col][0](adder->out);

        for (uint row=0; row<PEs.height; row++)
          adder->in[row](multipliers[col][row]->out);
      }
    }

    // Adders along rows.
    if (!config.accumulateCols() && config.accumulateRows()) {
      for (uint row=0; row<PEs.height; row++) {
        AdderTree<T>* adder = new AdderTree<T>(PEs.width, 1, 1);
        adders.push_back(adder);
        out[0][row](adder->out);

        for (uint col=0; col<PEs.width; col++)
          adder->in[col](multipliers[col][row]->out);
      }
    }

    // Adders along both columns and rows.
    if (config.accumulateCols() && config.accumulateRows()) {
      for (uint col=0; col<PEs.width; col++) {
        AdderTree<T>* adder = new AdderTree<T>(PEs.height, 1, 1);
        adders.push_back(adder);

        for (uint row=0; row<PEs.height; row++)
          adder->in[row](multipliers[col][row]->out);
      }

      AdderTree<T>* adder = new AdderTree<T>(adders.size(), 1, 1);
      for (uint i=0; i<adders.size(); i++)
        adder->in[i](adders[i]->out);
      adders.push_back(adder);
      out[0][0](adder->out);
    }
  }

  ~ComputeUnit() {
    for (uint i=0; i<multipliers.size(); i++)
      for (uint j=0; j<multipliers[i].size(); j++)
        delete multipliers[i][j];

    for (uint i=0; i<adders.size(); i++)
      delete adders[i];
  }

//============================================================================//
// Local state
//============================================================================//

private:

  vector<vector<Multiplier<T>*>> multipliers;
  vector<AdderTree<T>*> adders;

};

#endif /* SRC_TILE_ACCELERATOR_COMPUTEUNIT_H_ */
