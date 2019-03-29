/*
 * Bridge.h
 *
 * Module to allow two networks to be connected to each other. Supplies the
 * `sink` interface for one network and the `source` interface for the other.
 *
 *  Created on: 29 Mar 2019
 *      Author: db434
 */

#ifndef SRC_NETWORK_BRIDGE_H_
#define SRC_NETWORK_BRIDGE_H_

// I believe that a 1-entry FIFO does the job without providing any unexpected
// buffering. This is essentially a wire - no new data can be written until the
// previous data has been consumed.
template<class T>
class Bridge : public NetworkFIFO<T> {
public:

  Bridge(sc_module_name name) :
      NetworkFIFO<T>(name, 1) {
    // Nothing
  }
};



#endif /* SRC_NETWORK_BRIDGE_H_ */
