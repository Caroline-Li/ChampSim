#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <stdint.h>
#include <time.h>

#include "../../inc/trace_instruction.h"

// struct input_instr {
//   // instruction pointer or PC (Program Counter)
//   unsigned long long ip = 0;

//   // branch info
//   unsigned char is_branch = 0;
//   unsigned char branch_taken = 0;

//   unsigned char destination_registers[NUM_INSTR_DESTINATIONS] = {}; // output registers
//   unsigned char source_registers[NUM_INSTR_SOURCES] = {};           // input registers

//   unsigned long long destination_memory[NUM_INSTR_DESTINATIONS] = {}; // output memory
//   unsigned long long source_memory[NUM_INSTR_SOURCES] = {};           // input memory
// };

typedef enum {LOAD, STORE} InsType;

using trace_instr_format_t = input_instr;

uint64_t instrCount = 0;
int cache_size = 0x200000;
int addr_start = 0x12000005;
int addr_range = 0x200000;
int addr_end = addr_start + addr_range;
int addr = addr_start;
int ip = 0x8000;
std::ofstream outfile;

void WriteCurrentInstruction(trace_instr_format_t curr_instr)
{
  typename decltype(outfile)::char_type buf[sizeof(trace_instr_format_t)];
  memcpy(buf, &curr_instr, sizeof(trace_instr_format_t));
  outfile.write(buf, sizeof(trace_instr_format_t));
}

trace_instr_format_t MakeInstruction(InsType ins_type, int trace_type) {
  trace_instr_format_t curr_instr;
  int reg = rand() % 8 + 1;
  curr_instr.ip = ip;
  ip += sizeof(trace_instr_format_t);
  curr_instr.branch_taken = 0;
  curr_instr.is_branch = 0;

  if (ins_type == LOAD) {
    curr_instr.destination_registers[0] = reg;
    curr_instr.source_memory[0] = addr;
    //curr_instr.source_memory[0] = 0x4000;
  }
  else if (ins_type == STORE) {
    curr_instr.source_registers[0] = reg;
    curr_instr.destination_memory[0] = addr;
    //curr_instr.destination_memory[0] = 0x4000;
  }
  // std::cout << "reg: " << reg << std::endl;
  // std::cout << "addr: " << addr << std::endl;

  switch (trace_type) {
    case 0:
    case 1:
      if (addr < addr_end) {
        addr = (addr + 0x40);
      }
      else {
        addr = addr_start;
      }
      break;
    case 2:
      addr = rand() % addr_range + addr_start;
      break;
    case 3:
      break;
    default:
      std::cout << "unrecognized trace type!" << std::endl;
      exit(-1);
  }

  return curr_instr;
}

int main(int argc, char *argv[]) 
{
  srand(time(NULL));
  if (argc != 4) {
    std::cout << "not enough args!" << std::endl;
    exit(-1);
  }
  int num_instr = std::stoi(argv[1]);
  std::string outfile_name = argv[2];
  int trace_type = std::stoi(argv[3]);

  switch (trace_type) {
    case 0:
      addr_range = 0x200000;
      break;
    case 1:
      addr_range = 0x201000;
      break;
    case 2:
      addr_range = 0x200000;
      break;
    case 3: 
      addr = 0x123005;
      break;
    default:
      break;
  }

  // open outfile
  outfile.open(outfile_name, std::ios_base::binary | std::ios_base::trunc);

  for (int i = 0; i < num_instr; i++) {
    int ld = rand() % 2;
    trace_instr_format_t instr = MakeInstruction(static_cast<InsType>(ld), trace_type);
    WriteCurrentInstruction(instr);
  }

}