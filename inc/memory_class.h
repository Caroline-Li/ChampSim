#ifndef MEMORY_CLASS_H
#define MEMORY_CLASS_H

#include <limits>

#include "block.h"

// CACHE ACCESS TYPE
#define LOAD 0
#define RFO 1
#define PREFETCH 2
#define WRITEBACK 3
#define TRANSLATION 4
#define NUM_TYPES 5

// CACHE BLOCK
class BLOCK
{
public:
  bool valid = false, prefetch = false, dirty = false, prefetch_until_evict = false;

  uint64_t address = 0, v_address = 0, tag = 0, data = 0, ip = 0, cpu = 0, instr_id = 0;
  uint64_t signature = 0; uint64_t evicted_demand_address = 0; bool possible_pollution = false;
  bool pollution = false;
  double dram_availability = 0.0;

  // replacement state
  uint32_t lru = std::numeric_limits<uint32_t>::max() >> 1;
};

class MemoryRequestConsumer
{
public:
  /*
   * add_*q() return values:
   *
   * -2 : queue full
   * -1 : packet value forwarded, returned
   * 0  : packet merged
   * >0 : new queue occupancy
   *
   */
  double dram_availability;
  int dram_occupancy;

  const unsigned fill_level;
  virtual int add_rq(PACKET* packet) = 0;
  virtual int add_wq(PACKET* packet) = 0;
  virtual int add_pq(PACKET* packet) = 0;
  virtual uint32_t get_occupancy(uint8_t queue_type, uint64_t address) = 0;
  virtual uint32_t get_size(uint8_t queue_type, uint64_t address) = 0;
  virtual uint32_t get_bank_occupancy(uint8_t queue_type, uint64_t address) = 0;

  explicit MemoryRequestConsumer(unsigned fill_level) : fill_level(fill_level) {}
};

class MemoryRequestProducer
{
public:
  MemoryRequestConsumer* lower_level;
  virtual void return_data(PACKET* packet) = 0;

protected:
  MemoryRequestProducer() {}
  explicit MemoryRequestProducer(MemoryRequestConsumer* ll) : lower_level(ll) {}
};

#endif
