#ifndef STMS_H
#define STMS_H

#include <cassert>
#include <map>
#include <set>
#include <stdio.h>

#include "cache.h"
#include "prefetcher.h"

struct GHB_Entry {
  uint64_t phy_addr;
  int last_index;
};

class STMSPrefetcher : public Prefetcher
{
private:
  std::vector<GHB_Entry> GHB;
  std::map<uint64_t, uint64_t> index_table;
  uint64_t last_address = 0;

  // Statistics tracking (For final output)
  struct stats_tracker {
    uint32_t total_access = 0;
    uint32_t predictions = 0;
    uint32_t no_prediction = 0;
  } stats;

  void init_knobs();
  void init_stats();
  void stms_train(uint64_t phy_addr);
  void stms_predict(uint64_t trigger_phy_addr, std::vector<uint64_t>& pref_addr, std::vector<bool>& pref_level);

public:
  STMSPrefetcher(std::string type);
  ~STMSPrefetcher();
  void invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, uint64_t instr_id, uint64_t curr_cycle,
                         std::vector<uint64_t>& pref_addr, std::vector<bool>& pref_level);
  void register_fill(uint64_t address, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint64_t curr_cycle);
  void dump_stats();
  void print_config();
};

#endif /* STMS_H */