/*
 * Parameters.cpp
 *
 * Contains all parameter values.
 *
 * These will eventually be loaded in from an external file, so they can be
 * modified automatically, and without requiring recompilation.
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#include "Parameters.h"
#include "Arguments.h"
#include "Logging.h"
#include "StringManipulation.h"

#include <iostream>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <map>

using std::map;
using std::string;
using std::cout;
using std::cerr;
using std::endl;

typedef parameter (*getFn)(const chip_parameters_t&);
typedef void (*setFn)(chip_parameters_t&, parameter);

typedef struct {
  string name;
  string description;
  getFn  getter;
  setFn  setter;
  parameter defaultValue;
} Parameter;

// Map from command line arguments to Parameters. Multiple arguments may map to
// the same Parameter.
map<string, Parameter> arguments;

// Map from shortened parameter names to full-length ones.
map<string, string> abbreviations;

// Old argument names which have been deprecated.
map<string, string> deprecated;

//============================================================================//
// General parameters
//============================================================================//

// Print information about any interesting activity during execution.
int DEBUG = 0;

// Print a trace of addresses of instructions executed.
int TRACE = 0;

// Count all events which are significant for energy consumption.
int ENERGY_TRACE = 0;

// Print each instruction executed and its context (register file contents).
// The format should be the same as csim's trace.
int CSIM_TRACE = 0;

// Output high-level information about the accelerators, e.g. execution times.
int ACCELERATOR_TRACE = 0;

// Number of bytes in a Loki word.
int BYTES_PER_WORD = 4;

// Number of cycles before terminating execution.
unsigned long TIMEOUT = ULONG_MAX;

//============================================================================//
// Global variables
//============================================================================//

// Value to be returned at the end of execution.
int RETURN_CODE = EXIT_SUCCESS;

// Number of cores in each tile.
parameter CORES_PER_TILE             = 2;

// If set to 1, all memory operations complete instantaneously.
parameter MAGIC_MEMORY               = 0;


//============================================================================//
// Methods
//============================================================================//

// Define standard getter/setter functions.
#define GETTER_SETTER(NAME, LOCATION) \
  parameter get ## NAME(const chip_parameters_t& p) {return p.LOCATION;}\
  void set ## NAME(chip_parameters_t& p, parameter v) {p.LOCATION = v;}

GETTER_SETTER(AcceleratorsPerTile,      tile.numAccelerators);
GETTER_SETTER(MemoriesPerTile,          tile.numMemories);
GETTER_SETTER(ComputeTileRows,          numComputeTiles.height);
GETTER_SETTER(ComputeTileColumns,       numComputeTiles.width);
GETTER_SETTER(NumRegisters,             tile.core.registerFile.size);
GETTER_SETTER(CoreScratchpadSize,       tile.core.scratchpad.size);
GETTER_SETTER(IPKFIFOSize,              tile.core.ipkFIFO.size);
GETTER_SETTER(IPKCacheSize,             tile.core.cache.size);
GETTER_SETTER(IPKCacheNumTags,          tile.core.cache.numTags);
GETTER_SETTER(MaxIPKSize,               tile.core.cache.maxIPKSize);
GETTER_SETTER(ChannelMapTableSize,      tile.core.channelMapTable.size);
GETTER_SETTER(DirectorySize,            tile.directory.size);
GETTER_SETTER(MemoryBankLatency,        tile.memory.latency);
GETTER_SETTER(MemoryBankSize,           tile.memory.size);
GETTER_SETTER(MemoryHitUnderMiss,       tile.memory.hitUnderMiss);
GETTER_SETTER(MainMemoryLatency,        memory.latency);
GETTER_SETTER(MainMemorySize,           memory.size);
GETTER_SETTER(MainMemoryBandwidth,      memory.bandwidth);
GETTER_SETTER(CoreNumInputChannels,     tile.core.numInputChannels);
GETTER_SETTER(CoreInputFIFOSize,        tile.core.inputFIFO.size);
GETTER_SETTER(CoreOutputFIFOSize,       tile.core.outputFIFO.size);
GETTER_SETTER(MemoryBankInputFIFOSize,  tile.memory.inputFIFO.size);
GETTER_SETTER(MemoryBankOutputFIFOSize, tile.memory.outputFIFO.size);
GETTER_SETTER(RouterFIFOSize,           router.fifo.size);
GETTER_SETTER(AcceleratorPERows,        tile.accelerator.numPEs.height);
GETTER_SETTER(AcceleratorPEColumns,     tile.accelerator.numPEs.width);
GETTER_SETTER(AcceleratorAccumulateRow, tile.accelerator.accumulateRows);
GETTER_SETTER(AcceleratorAccumulateCol, tile.accelerator.accumulateCols);
GETTER_SETTER(AcceleratorPELatency,     tile.accelerator.latency);
GETTER_SETTER(AcceleratorPEInitInterval,tile.accelerator.initiationInterval);

// Non-standard getters/setters access location outside of the parameter struct,
// or access more than one location.
parameter getCoresPerTile(const chip_parameters_t& p) {return p.tile.numCores;}
void setCoresPerTile(chip_parameters_t& p, parameter v) {p.tile.numCores = v; CORES_PER_TILE = v;}
parameter getMagicMemory(const chip_parameters_t& p) {return MAGIC_MEMORY;}
void setMagicMemory(chip_parameters_t& p, parameter v) {MAGIC_MEMORY = v;}
parameter getDebug(const chip_parameters_t& p) {return DEBUG;}
void setDebug(chip_parameters_t& p, parameter v) {DEBUG = v;}
parameter getTimeout(const chip_parameters_t& p) {return TIMEOUT;}
void setTimeout(chip_parameters_t& p, parameter v) {TIMEOUT = v;}

// Currently give all networks the same bandwidth.
parameter getNetworkBandwidth(const chip_parameters_t& p) {return p.router.fifo.bandwidth;}
void setNetworkBandwidth(chip_parameters_t& p, parameter v) {
  p.tile.core.cache.bandwidth = v;
  p.tile.core.inputFIFO.bandwidth = v;
  p.tile.core.ipkFIFO.bandwidth = v;
  p.tile.core.outputFIFO.bandwidth = v;
  p.tile.memory.inputFIFO.bandwidth = v;
  p.tile.memory.outputFIFO.bandwidth = v;
  p.router.fifo.bandwidth = v;
}

void addParameter(string argument, string name, string description,
                  getFn getter, setFn setter, parameter defaultValue) {
  Parameter p;
  p.name = name;
  p.description = description;
  p.getter = getter;
  p.setter = setter;
  p.defaultValue = defaultValue;

  arguments[argument] = p;
}

void initialiseParameters() {
  // Exit if already initialised.
  if (arguments.size() > 0)
    return;

  addParameter("cores-per-tile", "Cores per tile", "",
               getCoresPerTile, setCoresPerTile, 2);

  addParameter("accelerators-per-tile", "Accelerators per tile", "",
               getAcceleratorsPerTile, setAcceleratorsPerTile, 1);

  addParameter("memories-per-tile", "Memory banks per tile", "",
               getMemoriesPerTile, setMemoriesPerTile, 8);
  abbreviations["mems-per-tile"] = "memories-per-tile";

  addParameter("compute-tile-rows", "Compute tile rows",
               "Height of compute tile grid. One extra non-compute tile is above and\n\tbelow this grid.",
               getComputeTileRows, setComputeTileRows, 1);
  abbreviations["tile-rows"] = "compute-tile-rows";

  addParameter("compute-tile-columns", "Compute tile columns",
               "Width of compute tile grid. One extra non-compute tile is to the left\n\tand right of this grid.",
               getComputeTileColumns, setComputeTileColumns, 1);
  abbreviations["compute-tile-cols"] = "compute-tile-columns";
  abbreviations["tile-cols"] = "compute-tile-columns";
  abbreviations["tile-columns"] = "compute-tile-columns";

  addParameter("core-registers", "Number of registers",
               "Size of core's register file. Includes fixed-function registers and\n\tregister-mapped input FIFOs.",
               getNumRegisters, setNumRegisters, 32);

  addParameter("core-scratchpad-size", "Core scratchpad size",
               "Core scratchpad size, in words.",
               getCoreScratchpadSize, setCoreScratchpadSize, 256);

  addParameter("core-ipk-fifo-size", "IPK FIFO size",
               "Size of core's instruction packet FIFO, in words.",
               getIPKFIFOSize, setIPKFIFOSize, 24);

  addParameter("core-ipk-cache-size", "IPK cache size",
               "Size of core's instruction packet cache, in words.",
               getIPKCacheSize, setIPKCacheSize, 64);

  addParameter("core-ipk-cache-num-tags", "IPK cache number of tags",
               "Number of tags in the core's instruction packet cache. This limits the\n\tnumber of packets which can be stored.",
               getIPKCacheNumTags, setIPKCacheNumTags, 16);
  abbreviations["ipk-cache-tags"] = "core-ipk-cache-num-tags";

  addParameter("max-ipk-size", "Maximum IPK size",
               "Maximum instruction packet size.",
               getMaxIPKSize, setMaxIPKSize, 8); // Default = cache line size?

  addParameter("core-channel-map-table-size", "Channel map table size",
               "Number of entries in the core's channel map table.",
               getChannelMapTableSize, setChannelMapTableSize, 15); // 1 channel reserved

  addParameter("directory-size", "Directory size",
               "Number of entries in the L1->L2 directory mapping.",
               getDirectorySize, setDirectorySize, 16);

  addParameter("memory-bank-latency", "Memory bank latency",
               "Latency (in cycles) of the on-tile memory banks, including network traversal.",
               getMemoryBankLatency, setMemoryBankLatency, 3);

  addParameter("memory-bank-size", "Memory bank size",
               "Size of on-tile memory banks (in bytes).",
               getMemoryBankSize, setMemoryBankSize, 8 * 1024);

  addParameter("memory-bank-hit-under-miss", "Memory hit-under-miss",
               "Whether memory banks are allowed to start new operations while waiting\n\tfor data for a request which missed.",
               getMemoryHitUnderMiss, setMemoryHitUnderMiss, 1);
  abbreviations["hit-under-miss"] = "memory-bank-hit-under-miss";

  addParameter("main-memory-latency", "Main memory latency", "",
               getMainMemoryLatency, setMainMemoryLatency, 20);

  addParameter("main-memory-size", "Main memory size",
               "Size of off-chip main memory in bytes.",
               getMainMemorySize, setMainMemorySize, 256 * 1024 * 1024);

  addParameter("main-memory-bandwidth", "Main memory bandwidth",
               "Off-chip memory bandwidth in words per cycle. Upper bound is the number\n\tof memory controllers.",
               getMainMemoryBandwidth, setMainMemoryBandwidth, 1);

  addParameter("core-num-input-channels", "Core number of input channels",
               "Total number of input channels, including both instruction and data\n\tinputs.",
               getCoreNumInputChannels, setCoreNumInputChannels, 8);

  addParameter("core-input-fifo-size", "Core input FIFO size",
               "Number of flits which can be stored in each input network FIFO\n\t(excluding instruction inputs which have their own parameters).",
               getCoreInputFIFOSize, setCoreInputFIFOSize, 4);

  addParameter("core-output-fifo-size", "Core output FIFO size",
               "Number of flits which can be stored in each output network FIFO.",
               getCoreOutputFIFOSize, setCoreOutputFIFOSize, 4);

  addParameter("memory-bank-input-fifo-size", "Memory bank input FIFO size",
               "Number of flits which can be stored in each input network FIFO.",
               getMemoryBankInputFIFOSize, setMemoryBankInputFIFOSize, 4);

  addParameter("memory-bank-output-fifo-size", "Memory bank output FIFO size",
               "Number of flits which can be stored in each output network FIFO.",
               getMemoryBankOutputFIFOSize, setMemoryBankOutputFIFOSize, 4);

  addParameter("router-fifo-size", "Router FIFO size",
               "Number of flits which can be stored in each FIFO.",
               getRouterFIFOSize, setRouterFIFOSize, 4);

  addParameter("magic-memory", "Magic memory",
               "When true, all memory operations complete instantly.",
               getMagicMemory, setMagicMemory, 0);
  
  addParameter("accelerator-pe-rows", "Accelerator PE rows",
               "Rows of processing elements in each accelerator",
               getAcceleratorPERows, setAcceleratorPERows, 4);
  abbreviations["accelerator-width"] = "accelerator-pe-rows";
  abbreviations["acc-pe-rows"] = "accelerator-pe-rows";
  abbreviations["acc-width"] = "accelerator-pe-rows";
  
  addParameter("accelerator-pe-columns", "Accelerator PE columns",
               "Columns of processing elements in each accelerator",
               getAcceleratorPEColumns, setAcceleratorPEColumns, 4);
  abbreviations["accelerator-height"] = "accelerator-pe-columns";
  abbreviations["acc-height"] = "accelerator-pe-columns";
  abbreviations["accelerator-pe-cols"] = "accelerator-pe-columns";
  abbreviations["acc-pe-columns"] = "accelerator-pe-columns";
  abbreviations["acc-pe-cols"] = "accelerator-pe-columns";

  addParameter("accelerator-accumulate-rows", "Accelerator accumulate rows",
               "Accumulate results within each row of PEs in each accelerator",
               getAcceleratorAccumulateRow, setAcceleratorAccumulateRow, 1);
  abbreviations["acc-accumulate-rows"] = "accelerator-accumulate-rows";

  addParameter("accelerator-accumulate-columns", "Accelerator accumulate columns",
               "Accumulate results within each column of PEs in each accelerator",
               getAcceleratorAccumulateCol, setAcceleratorAccumulateCol, 1);
  abbreviations["acc-accumulate-cols"] = "accelerator-accumulate-cols";

  addParameter("accelerator-pe-latency", "Accelerator PE latency",
               "Time between PE receiving input and producing output, in cycles",
               getAcceleratorPELatency, setAcceleratorPELatency, 1);
  abbreviations["acc-pe-latency"] = "accelerator-pe-latency";

  addParameter("accelerator-pe-initiation-interval", "Accelerator PE initiation interval",
               "Time between PE receiving input and being ready to receive another input, in cycles",
               getAcceleratorPEInitInterval, setAcceleratorPEInitInterval, 1);
  abbreviations["accelerator-pe-ii"] = "accelerator-pe-initiation-interval";
  abbreviations["acc-pe-initiation-interval"] = "accelerator-pe-initiation-interval";
  abbreviations["acc-pe-ii"] = "accelerator-pe-initiation-interval";

  addParameter("network-bandwidth", "Network bandwidth",
               "Bandwidth of all on-chip networks, in words per cycle.",
               getNetworkBandwidth, setNetworkBandwidth, 1);

  deprecated["CORES_PER_TILE"] = "cores-per-tile";
  deprecated["ACCELERATORS_PER_TILE"] = "accelerators-per-tile";
  deprecated["MEMS_PER_TILE"] = "memories-per-tile";
  deprecated["COMPUTE_TILE_ROWS"] = "compute-tile-rows";
  deprecated["COMPUTE_TILE_COLUMNS"] = "compute-tile-columns";
  deprecated["NUM_ADDRESSABLE_REGISTERS"] = "core-registers";
  deprecated["CORE_SCRATCHPAD_SIZE"] = "core-scratchpad-size";
  deprecated["IPK_FIFO_SIZE"] = "core-ipk-fifo-size";
  deprecated["IPK_CACHE_SIZE"] = "core-ipk-cache-size";
  deprecated["IPK_CACHE_TAGS"] = "core-ipk-cache-num-tags";
  deprecated["CHANNEL_MAP_SIZE"] = "core-channel-map-table-size";
  deprecated["DIRECTORY_SIZE"] = "directory-size";
  deprecated["MEMORY_BANK_LATENCY"] = "memory-bank-latency";
  deprecated["MEMORY_BANK_SIZE"] = "memory-bank-size";
  deprecated["MEMORY_HIT_UNDER_MISS"] = "memory-bank-hit-under-miss";
  deprecated["MAIN_MEMORY_LATENCY"] = "main-memory-latency";
  deprecated["MAIN_MEMORY_SIZE"] = "main-memory-size";
  deprecated["MAIN_MEMORY_BANDWIDTH"] = "main-memory-bandwidth";
  deprecated["CORE_BUFFER_SIZE"] = "core-input-fifo-size";
  deprecated["MEMORY_BUFFER_SIZE"] = "memory-bank-input-fifo-size";
  deprecated["ROUTER_BUFFER_SIZE"] = "router-fifo-size";
  deprecated["MAGIC_MEMORY"] = "magic-memory";
  deprecated["ACCELERATOR_WIDTH"] = "accelerator-pe-columns";
  deprecated["ACCELERATOR_HEIGHT"] = "accelerator-pe-rows";
  deprecated["ACCELERATOR_BCAST_ROWS"] = "accelerator-broadcast-rows";
  deprecated["ACCELERATOR_BCAST_COLS"] = "accelerator-broadcast-columns";
  deprecated["ACCELERATOR_ACC_ROWS"] = "accelerator-accumulate-rows";
  deprecated["ACCELERATOR_ACC_COLS"] = "accelerator-accumulate-columns";
}

size_t core_parameters_t::numOutputChannels() const {
  return channelMapTable.size;
}

// Used to determine how many bits in a memory address map to the same line.
size_t memory_bank_parameters_t::log2CacheLineSize() const {
  size_t result = 0;
  size_t temp = cacheLineSize >> 1;

  while (temp != 0) {
    result++;
    temp >>= 1;
  }

  assert(1UL << result == cacheLineSize);

  return result;
}

size2d_t accelerator_parameters_t::dma1Ports() const {
  size2d_t size;
  size.height = numPEs.height;
  size.width = numPEs.width;

  return size;
}

size2d_t accelerator_parameters_t::dma2Ports() const {
  size2d_t size;
  size.width = numPEs.width;
  size.height = numPEs.height;

  return size;
}

size2d_t accelerator_parameters_t::dma3Ports() const {
  size2d_t size;
  size.height = accumulateCols ? 1 : numPEs.height;
  size.width = accumulateRows ? 1 : numPEs.width;

  return size;
}

// Used to determine how many bits in a memory address map to the same line.
size_t main_memory_parameters_t::log2CacheLineSize() const {
  size_t result = 0;
  size_t temp = cacheLineSize >> 1;

  while (temp != 0) {
    result++;
    temp >>= 1;
  }

  assert(1UL << result == cacheLineSize);

  return result;
}

size_t tile_parameters_t::mcastNetInputs() const {
  return numCores + numAccelerators;
}

size_t tile_parameters_t::mcastNetOutputs() const {
  return numCores * core.numInputChannels + numAccelerators;
}

size_t tile_parameters_t::totalComponents() const {
  return numCores + numMemories + numAccelerators;
}

size2d_t chip_parameters_t::allTiles() const {
  // There is a border of I/O tiles all around the compute tiles.
  size2d_t tiles = numComputeTiles;
  tiles.height += 2;
  tiles.width += 2;
  return tiles;
}

size_t chip_parameters_t::totalCores() const {
  return numComputeTiles.total() * tile.numCores;
}

size_t chip_parameters_t::totalMemories() const {
  return numComputeTiles.total() * tile.numMemories;
}

size_t chip_parameters_t::totalAccelerators() const {
  return numComputeTiles.total() * tile.numAccelerators;
}

size_t chip_parameters_t::totalComponents() const {
  return numComputeTiles.total() * tile.totalComponents();
}

// Change a parameter value. Only valid before the SystemC components have
// been instantiated.
void Parameters::parseParameter(string name, string value,
                                chip_parameters_t& params) {
  initialiseParameters();

  int nValue = StringManipulation::strToInt(value);

  // Convert abbreviated name to a full name, if necessary.
  if (abbreviations.find(name) != abbreviations.end())
    name = abbreviations[name];

  if (deprecated.find(name) != deprecated.end()) {
    LOKI_WARN << "Parameter " << name << " is deprecated. Please use "
        << deprecated[name] << " instead" << endl;
    name = deprecated[name];
  }

  if (arguments.find(name) == arguments.end()) {
    LOKI_ERROR << "Encountered unknown parameter: " << name << endl;
    throw std::exception();
  }

  Parameter& p = arguments[name];
  p.setter(params, nValue);
}

// Print parameters in a human-readable format.
void Parameters::printParameters(const chip_parameters_t& params) {
  for (auto const &p : arguments)
    cout << "Parameter " << p.first << " is " << p.second.getter(params) << endl;
}

void Parameters::printHelp() {
  initialiseParameters();

  for (auto const &p : arguments) {
    cout << p.first << ":\t" << p.second.name << endl;
    if (p.second.description.length() > 0)
      cout << "\t" << p.second.description << endl;
    cout << "\tDefault: " << p.second.defaultValue << "\n" << endl;
  }
}

#define XML_LINE(name, value) "\t<" << name << ">" << value << "</" << name << ">\n"

// Print parameters in an XML format.
void Parameters::printParametersXML(std::ostream& os, const chip_parameters_t& params) {
  os << "<parameters>\n";

  for (auto const &p : arguments)
    os << XML_LINE(p.first, p.second.getter(params));

  os << "</parameters>\n";
}

chip_parameters_t* Parameters::defaultParameters() {
  initialiseParameters();

  chip_parameters_t* p = new chip_parameters_t();

  for (auto const &a : arguments)
    a.second.setter(*p, a.second.defaultValue);

  // Some extra parameters not accessible through the command line yet.
  p->tile.memory.cacheLineSize = 8 * BYTES_PER_WORD;
  p->memory.cacheLineSize = p->tile.memory.cacheLineSize;

  return p;
}
