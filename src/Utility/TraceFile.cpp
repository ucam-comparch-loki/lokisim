/*
 * TraceFile.cpp
 *
 *  Created on: 5 Mar 2010
 *      Author: db434
 */

#include "TraceFile.h"

static const string header = "VCD trace file generated by Loki Simulator\n\n";

/* Open a trace file with the given name, and insert a comment containing
 * the current parameter values. */
sc_trace_file* TraceFile::initialiseTraceFile(string& name) {

  string fullName = "trace_files/";
  fullName.append(name);

  sc_trace_file *trace = sc_core::sc_create_vcd_trace_file(fullName.c_str());
  trace->set_time_unit(100, sc_core::SC_PS);

  sc_core::sc_write_comment(trace, header + parameterSummary());

  return trace;

}

/* Generate a string to report the settings of the parameters when this
 * trace file is created. */
string TraceFile::parameterSummary() {

  // Move this to Parameters?
  std::stringstream s;

  using std::endl;

  s << "PARAMETERS:" << "\n";
  s << "Number of tiles: " << (NUM_TILE_ROWS * NUM_TILE_COLUMNS) << "\n";
  s << "  Clusters per tile:      " << CLUSTERS_PER_TILE << "\n";
  s << "  Memories per tile:      " << MEMS_PER_TILE << "\n";
  s << "  Channels between tiles: " << NUM_CHANNELS_BETWEEN_TILES << "\n";
  s << "Storage:\n";
  s << "  Memory size:                   " << MEMORY_SIZE << "\n";
  s << "  Instruction packet cache size: " << IPK_CACHE_SIZE << "\n";
  s << "  Instruction packet FIFO size:  " << IPK_FIFO_SIZE << "\n";
  s << "  Register file size:            " << NUM_REGISTERS << "\n";
  s << "Channel ends:\n";
  s << "  Input:       " << NUM_RECEIVE_CHANNELS << "\n";
  s << "  Output:      " << NUM_SEND_CHANNELS << "\n";
  s << "  Buffer size: " << CHANNEL_END_BUFFER_SIZE << "\n";
  s << "Flow control buffer size: " << FLOW_CONTROL_BUFFER_SIZE << "\n";
  s << "Maximum instruction packet size: " << MAX_IPK_SIZE;

  return s.str();

}
