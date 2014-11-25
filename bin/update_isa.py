#! /usr/bin/env python

import datetime, os

class Operation:
    # Split the string into its individual sections. These should be:
    #  operation[0] = mnemonic      (e.g. NOR)
    #  operation[1] = opcode        (e.g. 0000000)
    #  operation[2] = format        (e.g. 3R(rd,rs,rt))
    #  operation[3] = function      (e.g. 0000)
    #  operation[4] = channel       ([o]ptional, [m]andatory, [-]unused)
    #  operation[5] = immediate     ([u]nsigned, [s]igned, [l]ui, [-]none)
    #  operation[6] = ALU function  (e.g. 00000 - usually the same as function bits)
    def __init__(self, string):
        parts = string.split()
        
        self.mnemonic = parts[0]
        self.opcode = int(parts[1], 2)
        self.fullformat = parts[2]
        self.format = parts[2].split('(')[0]  # e.g. just "3R"
        
        if parts[3] == '-':
            self.function = 0
        else:
            self.function = int(parts[3], 2)
            
        self.channel = parts[4]
        self.immediate = parts[5]
        
        if parts[6] == '-':
            self.alu_function = -1
        else:
            self.alu_function = int(parts[6], 2)
    
    def __str__(self):
        return self.mnemonic.ljust(16) + str(self.opcode).ljust(4) +\
               self.format.ljust(8) + str(self.function).ljust(4) + self.channel.ljust(4) +\
               self.immediate.ljust(4) + str(self.alu_function)
        
    def has_dest_reg(self):
        return self.fullformat.find("rd") >= 0        
    def has_src_reg1(self):
        return self.fullformat.find("rs") >= 0
    def has_src_reg2(self):
        return self.fullformat.find("rt") >= 0
        
    def has_immediate(self):
        # Immediates which don't pass through the ALU aren't recognised in the
        # "immediate" field, so we have to do a more complicated check.
        return (self.fullformat.find("immed") >= 0) or (self.fullformat.find("shamt") >= 0)
        
    def has_channel(self):
        return not self.channel == '-'
        
    def sets_predicate(self):
        return self.asm_name().find(".p") >= 0
        
    def is_alu_operation(self):
        return self.alu_function >= 0
    
    def signed_immediate(self):
        return self.immediate == 's'
    
    # The name to go in the C++ enumeration of all operations.
    # By convention, it is all capitals, and it cannot contain any punctuation.
    def enum_name(self):
        return "OP_" + self.mnemonic.replace(".", "_").upper()
    
    # The name of the operation which gets printed. By convention, it is all
    # lower case.
    def asm_name(self):
        return self.mnemonic.lower()
    
    # The enumeration name of the ALU function this operation corresponds to.
    # Only valid for ALU operations.
    # TODO: use the actual name, rather than just casting the value.
    def function_name(self):
        return "(function_t)" + str(self.alu_function)
        
    # An example use case of this instruction, e.g. nor rd, rs, rt (-> ch)
    def usage(self):
        result = self.asm_name() + " "
        operands = self.fullformat.split('(')[1].split(')')[0] # Leave contents of brackets
        result += operands.replace(",", ", ")   # Add spacing
        
        if(self.channel == "m"):
            result += " -> ch"
        elif(self.channel == "o"):
            result += " (-> ch)"
        
        return result.replace(", unused", "").replace(" unused", "") # Remove all instances of "unused"
        
        
################################################################################        

# The file containing all of the information.
definition_file = "/homes/rdm34/ltmp/opcodes.isa"

with open(definition_file) as openfile:
    operations = openfile.readlines()

# Filter out comments and blank lines
operations = filter(lambda x: x[0] != '%' and not x[0].isspace(), operations)

# Generate an Operation type for each line of text remaining.
operations = [Operation(operation) for operation in operations]

# The files we are going to write into.
directory = os.path.dirname(os.path.realpath(__file__))
target_header = directory + "../src/Utility/InstructionMap.h"
target_source = directory + "../src/Utility/InstructionMap.cpp"

# Return a string representing a comma-separated list of booleans showing which 
# operations satisfy the filter function.
def opcode_list(filter_function):  
  d = dict()
  for i in range(128):
    d[i] = "0"
  for operation in filter(filter_function, operations):
    d[operation.opcode] = "1"
  return ", ".join(d.values())
  
# Return a string representing a comma-separated list of values.
# There may be gaps (e.g. an opcode which isn't used) which are filled with the
# supplied default value. The two functions given access the appropriate fields
# of an Operation: one gets the key, and the other gets the value. 
# Also add a function which can filter out operations which shouldn't join the map.
def create_map(length, key_function, value_function, default, include_function=lambda x:True):
  d = dict()
  for i in range(length):
    d[i] = default
  for operation in filter(include_function, operations):
    d[key_function(operation)] = value_function(operation)
  return ", ".join(d.values())

# Generate C++ code which initialises a map of the given types with all operations.
# The functions provided access the keys and values within each operation.
# Also add a function which can filter out operations which shouldn't join the map.  
def create_c_map(name, key_type, value_type, key_function, value_function, include_function=lambda x:True):
  code = """\
  static std::map<"""+key_type+","+value_type+"> "+name+""";
  static bool initialised = false;
  
  if(!initialised) {
""";
  
  room_on_line = True;
  for operation in filter(include_function, operations):
    single_op = "    "+name+"["+str(key_function(operation))+"] = "+str(value_function(operation))+";"
    code += single_op.ljust(50)
    if not room_on_line:
      code += "\n"
    room_on_line = not room_on_line
  return code + "\n    initialised = true;\n  }\n"

################################################################################

header_text = """\
/*
 * InstructionMap.h
 *
 * Listing of all instructions, their formats, and other associated information.
 *
 * Automatically generated by """ + os.path.basename(__file__) + " on " + str(datetime.date.today()) + """.
 *
 */
 
#ifndef INSTRUCTIONMAP_H_
#define INSTRUCTIONMAP_H_

#include <string>

typedef std::string inst_name_t;

class InstructionMap {

public:

  enum Opcode {
\n"""

# Add a line for each operation to the enumeration.
for i in range(len(operations)):
    operation = operations[i]
    name = operation.enum_name() + " = " + str(operation.opcode)
    if i < len(operations) - 1:
        name += ","
    comment = "// " + operation.usage()
    # strings are immutable - don't want to keep adding to them
    # Use a bytearray instead, or a list of all lines to write to the file
    header_text += "    " + name.ljust(20) + comment + "\n"

header_text += """
  };
  
  enum Function {"""

functions = []

# Go through all operations, checking for their ALU function codes. Assumes that
# the first operation found will have the most useful name, and that function
# codes appear in order.
for operation in operations:
    if operation.alu_function not in functions:
        header_text += "\n    FN_" + operation.mnemonic.replace(".", "_").upper() + " = " + str(operation.alu_function) + ","
        functions.append(operation.alu_function)

header_text += """
  };
  
  enum Format {
    FMT_FF,     // Fetch format                  (rs,immed)
    FMT_0R,     // Zero registers                (unused)        (immed)
    FMT_0Rnc,   // Zero registers, no channel    (immed)
    FMT_1R,     // One register                  (rd,immed)      (rs,immed)
    FMT_1Rnc,   // One register, no channel      (rd,immed)      (rs,immed)       (rd,unused)
    FMT_2R,     // Two registers                 (rd,rs,immed)   (rs,rt,unused)
    FMT_2Rnc,   // Two registers, no channel     (rs,rt,unused)
    FMT_2Rs,    // Two registers, shift amount   (rd,rs,shamt)
    FMT_3R      // Three registers               (rd,rs,rt)
  };
  
  typedef InstructionMap::Opcode opcode_t;
  typedef InstructionMap::Function function_t;
  typedef InstructionMap::Format format_t;
  
  // Simple true/false questions to ask of each operation.
  static bool storesResult(opcode_t opcode);
  static bool hasDestReg(opcode_t opcode);
  static bool hasSrcReg1(opcode_t opcode);
  static bool hasSrcReg2(opcode_t opcode);
  static bool hasImmediate(opcode_t opcode);
  static bool hasRemoteChannel(opcode_t opcode);
  static bool setsPredicate(opcode_t opcode);
  static bool isALUOperation(opcode_t opcode);
  static bool hasSignedImmediate(opcode_t opcode);
  
  // The total number of instructions currently supported.
  static int numInstructions();
  
  // Convert back and forth between names and opcodes.
  static opcode_t opcode(const inst_name_t& name);
  static const inst_name_t& name(opcode_t opcode, function_t function = (function_t)0);
  
  static function_t function(const inst_name_t& name);
  static function_t function(opcode_t opcode);
  static format_t format(opcode_t opcode);

};

typedef InstructionMap::Opcode opcode_t;
typedef InstructionMap::Function function_t;
typedef InstructionMap::Format format_t;

#endif /* INSTRUCTIONMAP_H_ */\n"""

# Write file
with open(target_header, "w") as header_file:
    header_file.write(header_text)
    print "Wrote", os.path.abspath(target_header)

################################################################################

# A function which returns the C++ implementation of an opcode_t -> bool method.
# Parameters:
#   name        = name of function
#   lambda_func = way of getting a boolean value from the Operation class
def func_implementation(name, lambda_func):
    return "bool InstructionMap::"+name+"(opcode_t opcode) {\n" +\
           "  static const bool _"+name+"[] = {"+opcode_list(lambda_func)+"};\n\n" +\
           "  return _"+name+"[opcode];\n" +\
           "}\n\n"

source_text = """\
/*
 * InstructionMap.cpp
 *
 * Automatically generated by """ + os.path.basename(__file__) + " on " + str(datetime.date.today()) + """.
 *
 */

#include "InstructionMap.h"
#include <assert.h>
#include <iostream>
#include <map>

bool InstructionMap::storesResult(opcode_t opcode) {return hasDestReg(opcode);} // remove?

""" + \
func_implementation("hasDestReg",         lambda x: x.has_dest_reg()    ) + \
func_implementation("hasSrcReg1",         lambda x: x.has_src_reg1()    ) + \
func_implementation("hasSrcReg2",         lambda x: x.has_src_reg2()    ) + \
func_implementation("hasImmediate",       lambda x: x.has_immediate()   ) + \
func_implementation("hasRemoteChannel",   lambda x: x.has_channel()     ) + \
func_implementation("setsPredicate",      lambda x: x.sets_predicate()  ) + \
func_implementation("isALUOperation",     lambda x: x.is_alu_operation()) + \
func_implementation("hasSignedImmediate", lambda x: x.signed_immediate()) + \
"""\
int  InstructionMap::numInstructions() {return """ + str(len(operations)) + """;} // 128?

const inst_name_t& InstructionMap::name(opcode_t opcode, function_t function) {
  static const inst_name_t opcode_to_name[] = {"""
  
source_text += \
  create_map(128,                                   # 128 opcodes
             lambda x: x.opcode,                    # mapping from opcodes...
             lambda x: '"' + x.asm_name() + '"',    # ... to names
             "\"\"")                                # if opcode unused, give empty string
             
source_text += """};
  
  static const inst_name_t function_to_name[] = {"""
  
source_text += \
  create_map(16,                                    # 16 function codes
             lambda x: x.function,                  # mapping from functions...
             lambda x: '"' + x.asm_name() + '"',    # ... to names
             "\"\"",                                # if unused, give empty string
             lambda x: x.opcode == 0)               # only instructions with opcode == 0 have function codes

source_text += """};
  
  static const inst_name_t function_to_name_p[] = {"""
  
source_text += \
  create_map(16,                                    # 16 function codes
             lambda x: x.function,                  # mapping from functions...
             lambda x: '"' + x.asm_name() + '"',    # ... to names
             "\"\"",                                # if unused, give empty string
             lambda x: x.opcode == 1)               # only instructions with opcode == 0 have function codes

source_text += """};

  if(opcode == 0)      return function_to_name[function];
  else if(opcode == 1) return function_to_name_p[function];
  else                 return opcode_to_name[opcode];
}

format_t InstructionMap::format(opcode_t opcode) {
  static const format_t opcode_to_format[] = {"""+create_map(128, lambda x: x.opcode, lambda x: "FMT_" + x.format, "(format_t)0")+"""};

  return opcode_to_format[opcode];
}

opcode_t InstructionMap::opcode(const inst_name_t& name) {\n"""

source_text += \
  create_c_map("name_to_opcode",                      # Name of map
               "inst_name_t",                         # Key type
               "opcode_t",                            # Value type
               lambda x: '"' + x.asm_name() + '"',    # Access key (and add quote marks)
               lambda x: x.enum_name())               # Access value

source_text += """
  if(name_to_opcode.find(name) == name_to_opcode.end()) {
    std::cerr << "Error: unknown instruction: " << name << std::endl;
    throw std::exception();
  }
  else return name_to_opcode[name];
}

function_t InstructionMap::function(const inst_name_t& name) {\n"""

source_text += \
  create_c_map("name_to_function",                    # Name of map
               "inst_name_t",                         # Key type
               "function_t",                          # Value type
               lambda x: '"' + x.asm_name() + '"',    # Access key (and add quote marks)
               lambda x: x.function_name(),           # Access value
               lambda x: x.opcode < 2)                # only certain instructions have function codes
  
source_text += """
  assert(name_to_function.find(name) != name_to_function.end());
  return name_to_function[name];
}

function_t InstructionMap::function(opcode_t opcode) {
  static const function_t opcode_to_function[] = {"""
  
source_text += \
  create_map(128,
             lambda x: x.opcode,
             lambda x: "(function_t)" + str(x.alu_function),
             "(function_t)0")

source_text += """};

  return opcode_to_function[opcode];
}
"""

# Write file 
with open(target_source, "w") as source_file:
    source_file.write(source_text)
    print "Wrote", os.path.abspath(target_source)
