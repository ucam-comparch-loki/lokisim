/*
 * MulticastController.h
 *
 * Implementation of Eyeriss's multicast controller based on:
 *   https://ieeexplore.ieee.org/document/7738524 fig 10
 *
 * Its purpose is to filter out data which matches its given tag.
 *
 *  Created on: Feb 11, 2020
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_EYERISS_MULTICASTCONTROLLER_H_
#define SRC_TILE_ACCELERATOR_EYERISS_MULTICASTCONTROLLER_H_

#include <utility>
#include "../../../LokiComponent.h"
#include "../../../Network/NetworkTypes.h"
#include "Interconnect.h"

using std::pair;

namespace Eyeriss {

template<typename T>
class MulticastController: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  // Receive a (tag, data) tuple, and output the data if the tag matches.
  typedef T out_t;
  typedef pair<uint, out_t> in_t;

  sc_port<interconnect_read_ifc<in_t>> in;
  sc_port<interconnect_write_ifc<out_t>> out;

//============================================================================//
// Constructors and destructors
//============================================================================//

protected:

  SC_HAS_PROCESS(MulticastController);
  MulticastController(sc_module_name name);

//============================================================================//
// Methods
//============================================================================//

protected:

  // Get the tag assigned to this unit.
  virtual uint getTag() = 0;

private:

  void newData();
  void newFlowControl();

};


// Filter for an individual PE.
template<typename T>
class MulticastControllerColumn: public MulticastController<T> {
public:
  sc_port<configuration_column_filter_ifc> control;

  MulticastControllerColumn(sc_module_name name, uint row, uint column);

protected:
  virtual uint getTag();

private:
  const uint row;
  const uint column;
};


// Filter for a row of PEs.
template<typename T>
class MulticastControllerRow: public MulticastController<pair<uint, T>> {
public:
  sc_port<configuration_row_filter_ifc> control;

  MulticastControllerRow(sc_module_name name, uint row);

protected:
  virtual uint getTag();

private:
  // The physical row index of this filter.
  const uint position;
};

} /* namespace Eyeriss */

#endif /* SRC_TILE_ACCELERATOR_EYERISS_MULTICASTCONTROLLER_H_ */
