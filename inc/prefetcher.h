#ifndef PREFETCHER_H
#define PREFETCHER_H

#include <string>
#include <vector>

class Prefetcher
{
protected:
  std::string type;

public:
  Prefetcher(std::string _type) { type = _type; }
  ~Prefetcher() {}
  std::string get_type() { return type; }
  virtual void invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, uint64_t instr_id, uint64_t curr_cycle,
                                 std::vector<uint64_t>& pref_addr, std::vector<bool>& pref_level, std::vector<int>& offsets) = 0;
  virtual void register_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint64_t curr_cycle) = 0;
  virtual void dump_stats() = 0;
  virtual void print_config() = 0;
};

#endif /* PREFETCHER_H */
