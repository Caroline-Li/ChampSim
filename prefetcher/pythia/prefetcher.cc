#include "pythia.h"
Prefetcher* pref;

void CACHE::prefetcher_initialize() {
    pref = new Pythia("pythia");
}

uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in,
                                      uint64_t curr_cycle)
{
  pref->register_fill(addr, set, way, prefetch, evicted_addr, curr_cycle);

  return metadata_in;
}

void CACHE::prefetcher_cycle_operate()
{
  
}

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in, uint64_t instr_id,
                                         uint64_t curr_cycle)
{
    // Collect prefetch requests
    std::vector<uint64_t> pref_addr;
    std::vector<bool> pref_level;
    std::vector<int> offsets;
    pref->invoke_prefetcher(ip, addr, cache_hit, type, instr_id, curr_cycle, pref_addr, pref_level, offsets);

    // Issue prefetch requests
    assert(pref_addr.size() == pref_level.size());
    for (size_t i = 0; i < pref_addr.size(); i++) {
        prefetch_line(pref_addr[i], pref_level[i], i, ip, addr, offsets[i], 0);
    }

  return metadata_in;
}

void CACHE::prefetcher_final_stats()
{
    pref->dump_stats();
}