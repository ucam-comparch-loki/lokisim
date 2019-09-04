/*
 * NetworkTypedefs.h
 *
 *  Created on: 27 Apr 2011
 *      Author: db434
 */

#ifndef NETWORKTYPEDEFS_H_
#define NETWORKTYPEDEFS_H_

#include "systemc"
#include "../Datatype/Flit.h"
#include "../Datatype/Word.h"
#include "../Communication/loki_ports.h"
#include "../Communication/loki_signal.h"

using sc_core::sc_in;
using sc_core::sc_out;
using sc_core::sc_signal;
using sc_core::sc_buffer;

// Information sent across the network.
typedef Flit<Word>                    NetworkData;
typedef Flit<Word>                    NetworkCredit;
typedef Flit<Word>                    NetworkRequest;
typedef Flit<Word>                    NetworkResponse;
typedef bool                          ReadyType;

typedef loki_signal<NetworkData>      DataSignal;
typedef loki_signal<NetworkCredit>    CreditSignal;
typedef loki_signal<NetworkRequest>   RequestSignal;
typedef loki_signal<NetworkResponse>  ResponseSignal;
typedef sc_signal<ReadyType>          ReadySignal;

typedef loki_in<NetworkData>          DataInput;
typedef loki_out<NetworkData>         DataOutput;
typedef loki_in<NetworkCredit>        CreditInput;
typedef loki_out<NetworkCredit>       CreditOutput;
typedef loki_in<NetworkRequest>       RequestInput;
typedef loki_out<NetworkRequest>      RequestOutput;
typedef loki_in<NetworkResponse>      ResponseInput;
typedef loki_out<NetworkResponse>     ResponseOutput;
typedef sc_in<ReadyType>              ReadyInput;
typedef sc_out<ReadyType>             ReadyOutput;

enum Direction {NORTH, EAST, SOUTH, WEST, LOCAL};

#endif /* NETWORKTYPEDEFS_H_ */
