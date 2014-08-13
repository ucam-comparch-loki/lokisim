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
#include "../Communication/loki_ports.h"
#include "../Communication/loki_signal.h"

using sc_core::sc_in;
using sc_core::sc_out;
using sc_core::sc_signal;
using sc_core::sc_buffer;

// Information sent across the network.
typedef Flit<Word>              NetworkData;
typedef Flit<Word>              NetworkCredit;
typedef Flit<Word>              NetworkRequest;
typedef Flit<Word>              NetworkResponse;
typedef bool                    ReadyType;

typedef loki_signal<NetworkData>   DataSignal;
typedef loki_signal<NetworkCredit> CreditSignal;
typedef sc_signal<ReadyType>    ReadySignal;

typedef loki_in<NetworkData>    DataInput;
typedef loki_out<NetworkData>   DataOutput;
typedef loki_in<NetworkCredit>  CreditInput;
typedef loki_out<NetworkCredit> CreditOutput;
typedef sc_in<ReadyType>        ReadyInput;
typedef sc_out<ReadyType>       ReadyOutput;

// Requests and grants used for arbitration.
// Requests currently specify the destination channel so that the relevant
// flow control information can be checked.
typedef ChannelIndex            RequestType;
typedef bool                    GrantType;
typedef int                     SelectType;

typedef sc_signal<RequestType>  RequestSignal;
typedef sc_signal<GrantType>    GrantSignal;
typedef sc_signal<SelectType>   SelectSignal;

typedef sc_in<RequestType>      RequestInput;
typedef sc_out<RequestType>     RequestOutput;
typedef sc_in<GrantType>        GrantInput;
typedef sc_out<GrantType>       GrantOutput;
typedef sc_in<SelectType>       SelectInput;
typedef sc_out<SelectType>      SelectOutput;

const RequestType NO_REQUEST = 255;
const SelectType NO_SELECTION = -1;


// (width, height) of the network, used to determine switching activity.
typedef std::pair<double,double> Dimension;

#endif /* NETWORKTYPEDEFS_H_ */
