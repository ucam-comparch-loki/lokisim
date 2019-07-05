/*
 * ComputeUnit.cpp
 *
 *  Created on: 24 May 2019
 *      Author: db434
 */

#include "ComputeUnit.h"
#include "../../Utility/Instrumentation/Operations.h"
#include "Accelerator.h"
#include "AccumulateStage.h"
#include "MultiplierStage.h"

// Need to list all options for templated components.
template class ComputeUnit<int>;
template class ComputeUnit<float>;

template<typename T>
ComputeUnit<T>::ComputeUnit(sc_module_name name, const accelerator_parameters_t& params) :
    LokiComponent(name),
    clock("clock"),
    in1("in1"),
    in2("in2"),
    out("out") {

  // The number of values flowing through the pipeline simultaneously.
  size2d_t datapath = params.numPEs;

  // Multiplier stage.
  MultiplierStage<T>* multipliers =
      new MultiplierStage<T>("multiply", datapath, 1, 1);
  multipliers->in[0](in1);
  multipliers->in[1](in2);
  pipeline.push_back(multipliers);

  // Accumulation stage (optional).
  if (params.accumulateCols) {
    AccumulateStage<T>* accumulator =
        new AccumulateStage<T>("acc_cols", AccumulateStage<T>::SUM_COLUMNS, 1, 1);
    addNewStage(accumulator, datapath);

    datapath.height = 1;
  }

  // Accumulation stage (optional).
  if (params.accumulateRows) {
    AccumulateStage<T>* accumulator =
        new AccumulateStage<T>("acc_rows", AccumulateStage<T>::SUM_ROWS, 1, 1);
    addNewStage(accumulator, datapath);

    datapath.width = 1;
  }

  pipeline[pipeline.size() - 1].out(out);
}

template<typename T>
void ComputeUnit<T>::addNewStage(ComputeStage<T>* stage, size2d_t inputSize) {
  AcceleratorChannel<T>* channel = new AcceleratorChannel<T>(inputSize);

  stage->in[0](*channel);
  pipeline[pipeline.size() - 1].out(*channel);

  channels.push_back(channel);
  pipeline.push_back(stage);
}

template<typename T>
Accelerator& ComputeUnit<T>::parent() const {
  return *(static_cast<Accelerator*>(this->get_parent_object()));
}

template<typename T>
const ComponentID& ComputeUnit<T>::id() const {
  return parent().id;
}

