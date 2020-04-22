/*
 * Configuration.h
 *
 * Interconnect allowing configuration of the PE array.
 *
 * Information includes:
 *  - row IDs for each row
 *  - column IDs for each PE
 *  - computation parameters
 *    - number of filters
 *    - number of channels
 *    - fixed point normalisation
 *
 * The original architecture implements this as a scan chain. Here, I allow
 * instant updates, but only notify the connected components after a
 * representative delay.
 *
 *  Created on: Feb 13, 2020
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_EYERISS_CONFIGURATION_H_
#define SRC_TILE_ACCELERATOR_EYERISS_CONFIGURATION_H_

#include "../../../LokiComponent.h"

using sc_core::sc_event;
using sc_core::sc_interface;

namespace Eyeriss {

class configuration_slave_ifc : virtual public sc_interface {
public:
  // Event triggered whenever the configuration changes.
  virtual const sc_event& default_event() const = 0;
};

class configuration_pe_ifc : virtual public configuration_slave_ifc {
public:
  // The number of filters in this computation.
  virtual uint getNumFilters() const = 0;

  // The number of channels in this computation.
  virtual uint getNumChannels() const = 0;

  // The amount to right-shift results by in order to normalise them.
  virtual uint getOutputShift() const = 0;
};

class configuration_row_filter_ifc : virtual public configuration_slave_ifc {
public:
  // Get the tag that this row is associated with for this computation.
  // Multiple row may share the same tag.
  virtual uint getRowID(uint row) const = 0;
};

class configuration_column_filter_ifc : virtual public configuration_slave_ifc {
public:
  // Get the tag that this PE is associated with for this computation.
  // Multiple PEs may share the same tag.
  virtual uint getColumnID(uint row, uint column) const = 0;
};

class configuration_master_ifc : virtual public sc_interface {
public:
  // Update the configuration for a new computation.
  virtual void update(uint numFilters, uint numChannels, uint outputShift) = 0;
};


class ControlFabric : public LokiComponent,
                      virtual public configuration_pe_ifc,
                      virtual public configuration_row_filter_ifc,
                      virtual public configuration_column_filter_ifc,
                      virtual public configuration_master_ifc {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  ControlFabric(sc_module_name name, uint rows, uint columns);

//============================================================================//
// Methods
//============================================================================//

public:

  // Event triggered whenever the configuration changes.
  virtual const sc_event& default_event() const;

  // The number of filters in this computation.
  virtual uint getNumFilters() const;

  // The number of channels in this computation.
  virtual uint getNumChannels() const;

  // The amount to right-shift results by in order to normalise them.
  virtual uint getOutputShift() const;

  // Get the tag that this row is associated with for this computation.
  // Multiple row may share the same tag.
  virtual uint getRowID(uint row) const;

  // Get the tag that this PE is associated with for this computation.
  // Multiple PEs may share the same tag.
  virtual uint getColumnID(uint row, uint column) const;

  // Update the configuration for a new computation.
  // These updates are usually computed offline and compiled into the program.
  // I compute them at runtime (and add fake latency) to simplify the flow.
  virtual void update(uint numFilters, uint numChannels, uint outputShift);

//============================================================================//
// Local state
//============================================================================//

private:

  // If this were implemented as a scan chain, how many cycles would it take for
  // data to reach the end?
  const uint latency;

  uint numFilters;
  uint numChannels;
  uint outputShift;

  vector<uint> rowIDs;
  vector<vector<uint>> columnIDs;

  sc_event newConfiguration;

};

} // end Eyeriss namespace

#endif /* SRC_TILE_ACCELERATOR_EYERISS_CONFIGURATION_H_ */
