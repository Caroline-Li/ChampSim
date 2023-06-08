/*
 * An idealized version of the STMS prefetcher.
 */
#include "stms.h"

#include <cassert>
#include <map>
#include <set>
#include <stdio.h>

namespace knob
{
extern uint32_t stms_pref_degree;
}

void STMSPrefetcher::init_knobs() {}

void STMSPrefetcher::init_stats() {}

STMSPrefetcher::STMSPrefetcher(std::string type) : Prefetcher(type)
{
  init_knobs();
  init_stats();
}

STMSPrefetcher::~STMSPrefetcher() {}

void STMSPrefetcher::invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, uint64_t instr_id, uint64_t curr_cycle,
                                       std::vector<uint64_t>& pref_addr, std::vector<bool>& pref_level)
{
  if (type != LOAD)
    return;

  if (cache_hit)
    return;

  uint64_t addr_B = (address >> 6) << 6;

  if (addr_B == last_address)
    return;
  last_address = addr_B;

  stats.total_access++;

  // Predict before training
  stms_predict(addr_B, pref_addr, pref_level);

  // unsigned int num_prefetched = 0;
  // for(unsigned int i=0; i<candidates.size(); i++)
  // {
  //     int ret = prefetch_line(ip, addr, candidates[i], FILL_LLC);
  //     if (ret == 1)
  //     {
  //         predictions++;
  //         num_prefetched++;
  //     }
  //     if(num_prefetched >= DEGREE)
  //         break;
  // }
  stms_train(addr_B);
}

void STMSPrefetcher::stms_train(uint64_t phy_addr)
{
  GHB_Entry new_entry;
  new_entry.phy_addr = phy_addr;
  if (index_table.find(phy_addr) != index_table.end())
    new_entry.last_index = index_table[phy_addr];
  else
    new_entry.last_index = -1;

  GHB.push_back(new_entry);
  assert(GHB.size() >= 1);

  index_table[phy_addr] = (GHB.size() - 1);
}

void STMSPrefetcher::stms_predict(uint64_t trigger_phy_addr, std::vector<uint64_t>& pref_addr, std::vector<bool>& pref_level)
{
  if (index_table.find(trigger_phy_addr) != index_table.end()) {
    uint64_t index = index_table[trigger_phy_addr];
    assert(GHB[index].phy_addr == trigger_phy_addr);
    for (unsigned int i = 1; i <= 32; i++) {
      if ((index + i) >= GHB.size() || pref_addr.size() >= knob::stms_pref_degree)
        break;
      uint64_t candidate_phy_addr = GHB[index + i].phy_addr;
      pref_addr.push_back(candidate_phy_addr);
      pref_level.push_back(true);

      stats.predictions++;
    }
  } else
    stats.no_prediction++;
}

void STMSPrefetcher::register_fill(uint64_t address, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint64_t curr_cycle) {}

void STMSPrefetcher::dump_stats()
{
  std::cout << "stms.total_access " << stats.total_access << std::endl
            << "stms.predictions " << stats.predictions << std::endl
            << "stms.no_prediction " << stats.no_prediction << std::endl
            << "stms.index_table_size " << index_table.size() << std::endl
            << "stms.ghb_size " << GHB.size() << std::endl;
}

void STMSPrefetcher::print_config() { std::cout << "stms_pref_degree " << knob::stms_pref_degree << std::endl; }