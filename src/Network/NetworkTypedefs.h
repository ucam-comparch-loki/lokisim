/*
 * NetworkTypedefs.h
 *
 *  Created on: 27 Apr 2011
 *      Author: db434
 */

#ifndef NETWORKTYPEDEFS_H_
#define NETWORKTYPEDEFS_H_

#include "systemc"
#include "../Datatype/AddressedWord.h"

using sc_core::sc_in;
using sc_core::sc_out;
using sc_core::sc_signal;
using sc_core::sc_buffer;

typedef AddressedWord       DataType;
typedef AddressedWord       CreditType;
typedef bool                ReadyType;

typedef sc_buffer<CreditType> CreditSignal;
typedef sc_buffer<DataType>   DataSignal;
typedef sc_signal<ReadyType>  ReadySignal;

typedef sc_in<DataType>     DataInput;
typedef sc_out<DataType>    DataOutput;
typedef sc_in<ReadyType>    ReadyInput;
typedef sc_out<ReadyType>   ReadyOutput;
typedef sc_in<CreditType>   CreditInput;
typedef sc_out<CreditType>  CreditOutput;

// (width, height) of the network, used to determine switching activity.
typedef std::pair<double,double> Dimension;

#endif /* NETWORKTYPEDEFS_H_ */