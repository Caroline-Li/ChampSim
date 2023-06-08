#ifndef SPP_DEV2_H
#define SPP_DEV2_H

#include <string>
#include <vector>

#include "cache.h"
#include "prefetcher.h"
#include "spp_dev2_helper.h"
#include <unordered_map>
// using namespace spp;

/* SPP Prefetcher */
class SPP_dev2 : public Prefetcher
{
private:
  CACHE* m_parent_cache;
  SIGNATURE_TABLE ST;
  PATTERN_TABLE PT;
  PREFETCH_FILTER FILTER;
  GLOBAL_REGISTER GHR;

  /* stats by rbera */
  struct {
    struct {
      uint64_t total;
      uint64_t at_L2;
      uint64_t at_LLC;
    } pref;

    struct {
      uint64_t count;
      uint64_t total;
      uint64_t max;
      uint64_t min;
    } depth;

    struct {
      uint64_t count;
      uint64_t total;
      uint64_t max;
      uint64_t min;
    } breadth;
  } stats;

  unordered_map<int32_t, uint64_t> delta_histogram;
  unordered_map<uint32_t, unordered_map<int32_t, uint64_t>> depth_delta_histogram;

private:
  void init_knobs();
  void init_stats();

public:
  SPP_dev2(std::string type, CACHE* cache);
  ~SPP_dev2();
  void invoke_prefetcher(uint64_t ip, uint64_t addr, uint8_t cache_hit, uint8_t type, uint64_t instr_id, uint64_t curr_cycle, std::vector<uint64_t>& pref_addr,
                         std::vector<bool>& pref_level, std::vector<int>& offsets);
  void dump_stats();
  void print_config();
  void register_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint64_t curr_cycle);
};

#endif /* SPP_DEV2_H */