/*
 * MulticastController.cpp
 *
 *  Created on: Feb 11, 2020
 *      Author: db434
 */

#include "MulticastController.h"

namespace Eyeriss {

template<typename T>
MulticastController<T>::MulticastController(sc_module_name name) :
    LokiComponent(name),
    in("in"),
    out("out") {

  SC_METHOD(newData);
  sensitive << in;
  dont_initialize();

  SC_METHOD(newFlowControl);
  sensitive << out->stallEvent() << out->unstallEvent();
  dont_initialize();

}

template<typename T>
void MulticastController<T>::newData() {
  loki_assert(out->ready());

  in_t data = in->read();
  uint tag = data.first;
  out_t payload = data.second;

  if (tag == getTag())
    out->write(payload);

  // Default trigger: wait for new data.
}

template<typename T>
void MulticastController<T>::newFlowControl() {
  // Event only triggers when state changes, so no chance for double-stalling or
  // double-unstalling.
  if (out->ready())
    in->unstall();
  else
    in->stall();
}



template<typename T>
MulticastControllerColumn<T>::MulticastControllerColumn(sc_module_name name, uint row, uint column) :
    MulticastController<T>(name),
    control("control"),
    row(row),
    column(column) {
  // Nothing
}

template<typename T>
uint MulticastControllerColumn<T>::getTag() {
  return control.getColumnID(row, column);
}



template<typename T>
MulticastControllerRow<T>::MulticastControllerRow(sc_module_name name, uint row) :
    MulticastController<pair<uint, T>>(name),
    control("control"),
    position(row) {
  // Nothing
}

template<typename T>
uint MulticastControllerRow<T>::getTag() {
  return control->getRowID(position);
}

} /* namespace Eyeriss */
