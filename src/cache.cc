#include "cache.h"

#include <algorithm>
#include <iterator>

#include "champsim.h"
#include "champsim_constants.h"
#include "util.h"
#include "vmem.h"
#include "fdp_control.h"
#include "prefetch_ip_suppress_filter.h"
#include "perceptron_filter_feature_test.h"
#include "decision_tree.h"

#ifndef SANITY_CHECK
#define NDEBUG
#endif

Tracer tracer;
extern VirtualMemory vmem;
extern uint8_t warmup_complete[NUM_CPUS];
HAWKEYE_PC_PREDICTOR* prefetch_predictor;
FDP_Control fdp_control;
Prefetch_Filter pf_filter;
Prefetch_Feature_Filter pf_feature_filter;
HAWKEYE_REUSE_PREDICTOR* prefetch_reuse_predictor;
extern bool training_complete;
double accuracy_threshold = 0.95;
vector<uint64_t> history[NUM_CPUS];
Control_Response control_response;
Decision_Tree_Predictor decision_tree_predictor;
// PERCEPTRON_ PERC_;
#define HISTORY_LEN 3
void CACHE::handle_fill()
{
  while (writes_available_this_cycle > 0) {
    auto fill_mshr = MSHR.begin();
    if (fill_mshr == std::end(MSHR) || fill_mshr->event_cycle > current_cycle)
      return;

    // find victim
    uint32_t set = get_set(fill_mshr->address);

    auto set_begin = std::next(std::begin(block), set * NUM_WAY);
    auto set_end = std::next(set_begin, NUM_WAY);
    auto first_inv = std::find_if_not(set_begin, set_end, is_valid<BLOCK>());
    uint32_t way = std::distance(set_begin, first_inv);
    if (way == NUM_WAY) {
      way = impl_replacement_find_victim(fill_mshr->cpu, fill_mshr->instr_id, set, &block.data()[set * NUM_WAY], fill_mshr->ip, fill_mshr->address,
                                         fill_mshr->type);
      if (PERCEPTRON_IP_FILTER) {
        if (fill_mshr->type == PREFETCH) {
          pf_filter.degree_pollution_tracker_map[CACHE_LINE(block.data()[set * NUM_WAY + way].address)] = {fill_mshr->prefetch_degree, true};
        }
        else {
          pf_filter.degree_pollution_tracker_map[CACHE_LINE(block.data()[set * NUM_WAY + way].address)] = {fill_mshr->prefetch_degree, false};
        }
        if (NAME == L2_NAME && !block.data()[set*NUM_WAY + way].prefetch_until_evict && fill_mshr->type == PREFETCH) {
            pf_filter.in_cache_pollution_map[CACHE_LINE(block.data()[set * NUM_WAY + way].address)] = {CACHE_LINE(fill_mshr->address), true};        
        }
      }
      if (FEATURE_TEST) {
        if (fill_mshr->type == PREFETCH) {
          pf_feature_filter.pollution_map[CACHE_LINE(block.data()[set * NUM_WAY + way].address)] = true;
          pf_feature_filter.ip_pollution_map_tracker[CACHE_LINE(block.data()[set * NUM_WAY + way].address)] = {fill_mshr->ip, true};
        }
        else {
          pf_feature_filter.pollution_map[CACHE_LINE(block.data()[set * NUM_WAY + way].address)] = false;
          pf_feature_filter.ip_pollution_map_tracker[CACHE_LINE(block.data()[set * NUM_WAY + way].address)] = {fill_mshr->ip, true};
        }
        pf_feature_filter.num_useful_blocks_evicted++;
      }
    }
    if (PERCEPTRON_REJECT_CACHE) {
      if (PERCEPTRON_REJECT_CACHE_LOGGING) {
          cout << "evicting from reject cache" << endl;
          pf_filter.print_reject_cache(set);
      }
      pf_filter.evict_victim_reject_cache(set);
      if (PERCEPTRON_REJECT_CACHE_LOGGING) {
          pf_filter.print_reject_cache(set);
      }
      
    }

    bool success = filllike_miss(set, way, *fill_mshr);
    if (!success)
      return;

    if (way != NUM_WAY) {
      // update processed packets
      fill_mshr->data = block[set * NUM_WAY + way].data;

      for (auto ret : fill_mshr->to_return)
        ret->return_data(&(*fill_mshr));
    }

    MSHR.erase(fill_mshr);
    writes_available_this_cycle--;
  }
}

void CACHE::handle_writeback()
{
  while (writes_available_this_cycle > 0) {
    if (!WQ.has_ready())
      return;

    // handle the oldest entry
    PACKET& handle_pkt = WQ.front();

    // access cache
    uint32_t set = get_set(handle_pkt.address);
    uint32_t way = get_way(handle_pkt.address, set);

    BLOCK& fill_block = block[set * NUM_WAY + way];

    if (way < NUM_WAY) // HIT
    {
      impl_replacement_update_state(handle_pkt.cpu, set, way, fill_block.address, handle_pkt.ip, 0, handle_pkt.type, 1);

      // COLLECT STATS
      sim_hit[handle_pkt.cpu][handle_pkt.type]++;
      sim_access[handle_pkt.cpu][handle_pkt.type]++;

      // mark dirty
      fill_block.dirty = 1;
    } else // MISS
    {
      bool success;
      if (handle_pkt.type == RFO && handle_pkt.to_return.empty()) {
        success = readlike_miss(handle_pkt);
      } else {
        // find victim
        auto set_begin = std::next(std::begin(block), set * NUM_WAY);
        auto set_end = std::next(set_begin, NUM_WAY);
        auto first_inv = std::find_if_not(set_begin, set_end, is_valid<BLOCK>());
        way = std::distance(set_begin, first_inv);
        if (way == NUM_WAY) {
          way = impl_replacement_find_victim(handle_pkt.cpu, handle_pkt.instr_id, set, &block.data()[set * NUM_WAY], handle_pkt.ip, handle_pkt.address,
                                             handle_pkt.type);
          if (PERCEPTRON_IP_FILTER) {
            if (handle_pkt.type == PREFETCH) {
              pf_filter.degree_pollution_tracker_map[CACHE_LINE(block.data()[set * NUM_WAY + way].address)] = {handle_pkt.prefetch_degree, true};
            }
            else {
              pf_filter.degree_pollution_tracker_map[CACHE_LINE(block.data()[set * NUM_WAY + way].address)] = {handle_pkt.prefetch_degree, false};
            }
            if (NAME == L2_NAME && !block.data()[set*NUM_WAY + way].prefetch_until_evict && handle_pkt.type == PREFETCH) {
              pf_filter.in_cache_pollution_map[CACHE_LINE(block.data()[set * NUM_WAY + way].address)] = {CACHE_LINE(handle_pkt.address), true};    
            }
          }

          if (FEATURE_TEST) {
            if (handle_pkt.type == PREFETCH) {
              pf_feature_filter.pollution_map[CACHE_LINE(block.data()[set * NUM_WAY + way].address)] = true;
              pf_feature_filter.ip_pollution_map_tracker[CACHE_LINE(block.data()[set * NUM_WAY + way].address)] = {handle_pkt.ip, true};
            }
            else {
              pf_feature_filter.pollution_map[CACHE_LINE(block.data()[set * NUM_WAY + way].address)] = false;
              pf_feature_filter.ip_pollution_map_tracker[CACHE_LINE(block.data()[set * NUM_WAY + way].address)] = {handle_pkt.ip, true};
            }
            pf_feature_filter.num_useful_blocks_evicted++;
          }
        }

        success = filllike_miss(set, way, handle_pkt);
      }

      if (!success)
        return;
    }

    // remove this entry from WQ
    writes_available_this_cycle--;
    WQ.pop_front();
  }
}

void CACHE::handle_read()
{
  while (reads_available_this_cycle > 0) {

    if (!RQ.has_ready())
      return;

    // handle the oldest entry
    PACKET& handle_pkt = RQ.front();

    // A (hopefully temporary) hack to know whether to send the evicted paddr or
    // vaddr to the prefetcher
    ever_seen_data |= (handle_pkt.v_address != handle_pkt.ip);

    uint32_t set = get_set(handle_pkt.address);
    uint32_t way = get_way(handle_pkt.address, set);

    BLOCK& fill_block = block[set * NUM_WAY + way];

    if (way < NUM_WAY) // HIT
    {
      readlike_hit(set, way, handle_pkt);
    } else {
      bool success = readlike_miss(handle_pkt);
      if (!success) {
        return;
      }
      else {

        // check for cache pollution
        // cout << "fill block evicted demand address: " << (fill_block.evicted_demand_address) << endl;
        // cout << "handle pkt address: " << CACHE_LINE(handle_pkt.address) << endl;
        if (NAME == L2_NAME && PERCEPTRON_IP_FILTER && 
          pf_filter.in_cache_pollution_map.find(CACHE_LINE(handle_pkt.address)) != pf_filter.in_cache_pollution_map.end() &&
          pf_filter.in_cache_pollution_map[CACHE_LINE(handle_pkt.address)].second) {
            uint64_t polluting_pf_addr = pf_filter.in_cache_pollution_map[CACHE_LINE(handle_pkt.address)].first;
            pf_filter.pf_addr_pollution_map[polluting_pf_addr] = true;
          // pf_filter.train(pf_filter.in_cache_pollution_map[CACHE_LINE(handle_pkt.address)].first, false, false, false, true, 1);
          pf_filter.in_cache_pollution_map[CACHE_LINE(handle_pkt.address)] = {0, false};
        }

        //  if (NAME == "cpu0_L2C" && PERCEPTRON_IP_FILTER && 
        //   fill_block.evicted_demand_address == CACHE_LINE(handle_pkt.address)) {
        //   cout << "pollution detected!" << endl;
        //   fill_block.pollution = true;
        // }

        // reject table
        if (PERCEPTRON_IP_FILTER && (PERCEPTRON_HISTORY || PERCEPTRON_DRAM_AVAILABILITY)) {
          if (PERCEPTRON_REJECT_TABLE) {
            if (pf_filter.check_reject_table(CACHE_LINE(handle_pkt.address))) {
              if (PERCEPTRON_DRAM_AVAILABILITY) {
                if (PERCEPTRON_REJECT_CACHE_LOGGING) {
                  cout << "training from reject table" << endl;
                }
                pf_filter.train(handle_pkt.ip, true, CACHE_LINE(handle_pkt.address));

              }
              else if (PERCEPTRON_HISTORY) {
                pf_filter.train(CACHE_LINE(handle_pkt.address), true, true, false, false, false, true, 1);
              }
              pf_filter.erase_reject_table(CACHE_LINE(handle_pkt.address));
            }
          }
          else if (PERCEPTRON_DIRECT_MAPPED_REJECT_TABLE) {
            if (pf_filter.check_direct_mapped_reject_table(CACHE_LINE(handle_pkt.address))) {
              if (PERCEPTRON_DRAM_AVAILABILITY) {
                pf_filter.train(CACHE_LINE(handle_pkt.address), true, CACHE_LINE(handle_pkt.address));
              }
              else if (PERCEPTRON_HISTORY) {
                pf_filter.train(CACHE_LINE(handle_pkt.address), true, true, false, false, false, true, 1);
              }
              pf_filter.erase_direct_mapped_reject_table(CACHE_LINE(handle_pkt.address));
            }
          }
          else if (PERCEPTRON_REJECT_CACHE) {
            if (pf_filter.check_reject_cache(get_set(handle_pkt.address), CACHE_LINE(handle_pkt.address))) {
              if (PERCEPTRON_REJECT_CACHE_LOGGING) {
                cout << "set: " << get_set(handle_pkt.address) << " address: " << handle_pkt.address << endl;
                pf_filter.print_reject_cache(get_set(handle_pkt.address));
              }
              if (REJECT_CACHE_TRAIN_LOGGING) {
                cout << "training from reject cache!" << endl;
                cout << "set: " << get_set(handle_pkt.address) << " address: " << handle_pkt.address << endl;
                pf_filter.print_reject_cache(get_set(handle_pkt.address));
              }
              pf_filter.train(CACHE_LINE(handle_pkt.address), true, true, false, false, false, true, 1);
              // pf_filter.train(handle_pkt.address, true);
              pf_filter.erase_reject_cache(get_set(handle_pkt.address), CACHE_LINE(handle_pkt.address));
            }
          }

          pf_filter.demand_miss_interval_counter++;
          pf_filter.demand_miss_total++;


          
        }
        if (FEATURE_TEST) {
          if (pf_feature_filter.pollution_map.find(CACHE_LINE(handle_pkt.address)) != pf_feature_filter.pollution_map.end() 
                && pf_feature_filter.pollution_map[CACHE_LINE(handle_pkt.address)]) {
                pf_feature_filter.pollution_interval_counter++;
                pf_feature_filter.pollution_map[CACHE_LINE(handle_pkt.address)] = false;
          }
          if (pf_feature_filter.ip_pollution_map_tracker.find(CACHE_LINE(handle_pkt.address)) != pf_feature_filter.ip_pollution_map_tracker.end() && pf_feature_filter.ip_pollution_map_tracker[CACHE_LINE(handle_pkt.address)].second) {
                pf_feature_filter.ip_pollution_map[pf_feature_filter.ip_pollution_map_tracker[CACHE_LINE(handle_pkt.address)].first]++;
                pf_feature_filter.ip_pollution_map_tracker[CACHE_LINE(handle_pkt.address)] = {pf_feature_filter.ip_pollution_map_tracker[CACHE_LINE(handle_pkt.address)].first, false};
          }
          pf_feature_filter.demand_miss_interval_counter++;
          pf_feature_filter.ip_demand_miss_interval_map[handle_pkt.ip]++;
          pf_feature_filter.demand_miss_total++;

          if (pf_feature_filter.check_direct_mapped_reject_table(CACHE_LINE(handle_pkt.address))) {
            pf_feature_filter.train(CACHE_LINE(handle_pkt.address), true, true, false);
            pf_feature_filter.erase_direct_mapped_reject_table(CACHE_LINE(handle_pkt.address));
          }
        }
        // if (PPF) {
        //   if (PERC_.check_reject_table(handle_pkt.address)) {
        //     PERC_.perc_train_from_reject_table(handle_pkt.address, true);
        //   }
        // }

      }

      if (FDP && NAME == "LLC") {
        fdp_control.num_demand_LLC_misses_total++;
      }
      else if (FDP_INTERVAL && NAME == "LLC") {
        fdp_control.num_demand_LLC_misses_interval_counter++;
      }
}

    // remove this entry from RQ
    RQ.pop_front();
    reads_available_this_cycle--;
  }
}

void CACHE::handle_prefetch()
{
  while (reads_available_this_cycle > 0) {
    if (!PQ.has_ready())
      return;

    // handle the oldest entry
    PACKET& handle_pkt = PQ.front();

    uint32_t set = get_set(handle_pkt.address);
    uint32_t way = get_way(handle_pkt.address, set);

    if (way < NUM_WAY) // HIT
    {
      readlike_hit(set, way, handle_pkt);

      if (NAME == L2_NAME && PERCEPTRON_IP_FILTER) {
        if (PERCEPTRON_HISTORY) {
          pf_filter.get_prediction(handle_pkt.ip, CACHE_LINE(handle_pkt.address), handle_pkt.pf_metadata, 0, get_set(handle_pkt.address), handle_pkt.pf_trigger_addr, handle_pkt.pf_offset, lower_level->dram_occupancy, false, 0, false, false, true, history[cpu]);
        }
      }

      if (NAME == L2_NAME && FEATURE_TEST) {
        pf_feature_filter.get_prediction(handle_pkt.ip, CACHE_LINE(handle_pkt.address), handle_pkt.pf_metadata, 0, get_set(handle_pkt.address), handle_pkt.pf_trigger_addr, handle_pkt.pf_offset, lower_level->dram_availability, lower_level->get_bank_occupancy(1, handle_pkt.address), false, 0, false, history[cpu]);
      }
    } else {

      if (NAME == L2_NAME && FEATURE_TEST) {
        bool pred = pf_feature_filter.get_prediction(handle_pkt.ip, CACHE_LINE(handle_pkt.address), handle_pkt.pf_metadata, 0, get_set(handle_pkt.address), handle_pkt.pf_trigger_addr, handle_pkt.pf_offset, lower_level->dram_availability, lower_level->get_bank_occupancy(1, handle_pkt.address), false, 0, true, history[cpu]);
        if (!pred) {
          pf_suppressed++;
          PQ.pop_front();
          return;
        }
      }

      if (NAME == L2_NAME && DECISION_TREE_FILTERING) {
        uint64_t ip_1 = history[cpu].size() > 0 ? history[cpu][0] : 0;
        uint64_t ip_2 = history[cpu].size() > 1 ? history[cpu][1] : 0;
        uint64_t ip_3 = history[cpu].size() > 2 ? history[cpu][2] : 0;
        uint64_t hashed_history = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
        bool pred = decision_tree_predictor.predict(handle_pkt.ip, handle_pkt.pf_metadata, handle_pkt.pf_offset, hashed_history, lower_level->dram_occupancy);
        if (!pred) {
          pf_suppressed++;
          PQ.pop_front();
          return;
        }
      }
      

      if (NAME == L2_NAME && PERCEPTRON_IP_FILTER) {
          bool pred = false;
          // double dram_availability = (lower_level->get_size(1, handle_pkt.address) - lower_level->get_occupancy(1, handle_pkt.address)) / (double) lower_level->get_size(1, handle_pkt.address);
          if (NO_DRAM_OCCUPANCY) {
            pred = pf_filter.get_prediction(handle_pkt.ip);
          }
          else if (PERCEPTRON_DRAM_AVAILABILITY) {
            pred = pf_filter.get_prediction(handle_pkt.ip, (lower_level->get_size(1, handle_pkt.address) - lower_level->get_occupancy(1, handle_pkt.address)) / (double) lower_level->get_size(1, handle_pkt.address));
          }
          else if (PERCEPTRON_HISTORY) {
            bool harmony_pred = false;
            int rrpv = 0;

            if (PERCEPTRON_WITH_HARMONY) {
              harmony_pred = prefetch_predictor->get_prediction(handle_pkt.ip);
            }
            if (!harmony_pred) {
              rrpv = 7;
            }
            PPF_PRED prefetch_action = pf_filter.get_prediction(handle_pkt.ip, CACHE_LINE(handle_pkt.address), handle_pkt.pf_metadata, rrpv, get_set(handle_pkt.address), handle_pkt.pf_trigger_addr, handle_pkt.pf_offset, lower_level->dram_occupancy, harmony_pred, 0, true, false, true, history[cpu]);
            if (prefetch_action == SUPPRESS) {
              pf_suppressed++;
              PQ.pop_front();
              return;
            }
          }
          pf_filter.addr_to_dram_availability_map[handle_pkt.address] = dram_availability;
          // if (!pred) {
          //   pf_suppressed++;
          //   PQ.pop_front();
          //   return;
          // }
        }


      bool success = readlike_miss(handle_pkt);
      if (!success) {
        return;
      }
      else {
        tracer.prefetch_fill_interval_counter++;
        tracer.prefetch_miss_total++;

        // dram occupancy here?
        if (FDP && DRAM_AVAILABILITY) {
          fdp_control.dram_availability_at_prefetch_issue += lower_level->get_size(1, handle_pkt.address) 
                                                            - lower_level->get_occupancy(1, handle_pkt.address);
          fdp_control.dram_occupancy_at_prefetch_issue += lower_level->get_occupancy(1, handle_pkt.address);
          if (!fdp_control.get_dram_rq_size) {
            fdp_control.dram_rq_size = lower_level->get_size(1, handle_pkt.address);
            fdp_control.get_dram_rq_size = true;
            cout << "dram rq size: " << fdp_control.dram_rq_size << endl;
          }
        }
        else if (FDP_INTERVAL && DRAM_AVAILABILITY) {
          fdp_control.dram_availability_at_prefetch_issue_interval += lower_level->get_size(1, handle_pkt.address) 
                                                            - lower_level->get_occupancy(1, handle_pkt.address);
          fdp_control.dram_occupancy_at_prefetch_issue_interval += lower_level->get_occupancy(1, handle_pkt.address);
          if (!fdp_control.get_dram_rq_size) {
            fdp_control.dram_rq_size = lower_level->get_size(1, handle_pkt.address);
            fdp_control.get_dram_rq_size = true;
            cout << "dram rq size: " << fdp_control.dram_rq_size << endl;
          }
        }
        if (FDP) {
            fdp_control.prefetch_fill_total++;
        }
        if (FDP_INTERVAL) {
            fdp_control.prefetch_fill_interval_counter++;
        }
        if (PERCEPTRON_IP_FILTER && NAME == "LLC") {
          pf_filter.prefetch_fill_interval_counter++;
          pf_filter.dram_availability_at_prefetch_issue_interval += lower_level->get_size(1, handle_pkt.address) 
                                                            - lower_level->get_occupancy(1, handle_pkt.address);
          pf_filter.bank_occupancy_at_prefetch_issue_interval += lower_level->get_bank_occupancy(1, handle_pkt.address);
        }
        if (NAME == "LLC") {
          tracer.dram_occupancy_at_prefetch_issue += lower_level->get_occupancy(1, handle_pkt.address);
          tracer.dram_availability_at_prefetch_issue += lower_level->get_size(1, handle_pkt.address) 
                                                            - lower_level->get_occupancy(1, handle_pkt.address);
        }
        if (!tracer.get_dram_rq_size) {
          tracer.dram_rq_size = lower_level->get_size(1, handle_pkt.address);
          tracer.get_dram_rq_size = true;
        }

        if (PERCEPTRON_IP_FILTER) {
          pf_filter.prefetch_miss_interval_counter++;
          pf_filter.prefetch_miss_total++;
          pf_filter.ip_accuracy_map[handle_pkt.ip].second++;
        }

        if (FEATURE_TEST) {
          pf_feature_filter.prefetch_fill_interval_counter++;
          pf_feature_filter.ip_accuracy_map[handle_pkt.ip].second++;
        }



      }
    }

    // remove this entry from PQ
    PQ.pop_front();
    reads_available_this_cycle--;
  }
}

void CACHE::readlike_hit(std::size_t set, std::size_t way, PACKET& handle_pkt)
{
  DP(if (warmup_complete[handle_pkt.cpu]) {
    std::cout << "[" << NAME << "] " << __func__ << " hit";
    std::cout << " instr_id: " << handle_pkt.instr_id << " address: " << std::hex << (handle_pkt.address >> OFFSET_BITS);
    std::cout << " full_addr: " << handle_pkt.address;
    std::cout << " full_v_addr: " << handle_pkt.v_address << std::dec;
    std::cout << " type: " << +handle_pkt.type;
    std::cout << " cycle: " << current_cycle << std::endl;
  });

  BLOCK& hit_block = block[set * NUM_WAY + way];

  handle_pkt.data = hit_block.data;

  // update prefetcher on load instruction
  if (should_activate_prefetcher(handle_pkt.type) && handle_pkt.pf_origin_level < fill_level) {
    cpu = handle_pkt.cpu;
    uint64_t pf_base_addr = (virtual_prefetch ? handle_pkt.v_address : handle_pkt.address) & ~bitmask(match_offset_bits ? 0 : OFFSET_BITS);
    handle_pkt.pf_metadata = impl_prefetcher_cache_operate(pf_base_addr, handle_pkt.ip, 1, handle_pkt.type, handle_pkt.pf_metadata, handle_pkt.instr_id, ooo_cpu[cpu]->current_cycle);

    if (PERCEPTRON_HISTORY || FEATURE_TEST) {
    auto iter = find(history[cpu].begin(), history[cpu].end(), handle_pkt.ip);
    if (iter != history[cpu].end()) {
      history[cpu].erase(iter);
    }
    history[cpu].push_back(handle_pkt.ip);
    if (history[cpu].size() > HISTORY_LEN) {
      history[cpu].erase(history[cpu].begin());
    }
  }

  }

  // update replacement policy
  impl_replacement_update_state(handle_pkt.cpu, set, way, hit_block.address, handle_pkt.ip, 0, handle_pkt.type, 1);

  // COLLECT STATS
  sim_hit[handle_pkt.cpu][handle_pkt.type]++;
  sim_access[handle_pkt.cpu][handle_pkt.type]++;

  for (auto ret : handle_pkt.to_return)
    ret->return_data(&handle_pkt);

  // update prefetch stats and reset prefetch bit
  if (hit_block.prefetch && handle_pkt.type != PREFETCH) {
    pf_useful++;
    tracer.useful_prefetch_total++;
    tracer.useful_prefetch_interval_counter++;
    if (PERCEPTRON_IP_FILTER) {
      // cout << "perceptron ip filter useful prefetch" << endl;
      pf_filter.ip_accuracy_map[hit_block.ip].first++;
      pf_filter.useful_prefetch_interval_counter++;
      pf_filter.useful_prefetch_total++;
      
      if (NO_DRAM_OCCUPANCY) {
        pf_filter.train(hit_block.ip, true, false);
      }
      else if (PERCEPTRON_DRAM_AVAILABILITY) {
        pf_filter.train(hit_block.ip, true, CACHE_LINE(hit_block.address));
      }
      else if (PERCEPTRON_HISTORY) {
        // pf_filter.train(hit_block.ip, hit_block.address, true, history[cpu]);
        // useful prefetch but caused cache pollution
        // if (pf_filter.pf_addr_pollution_map[CACHE_LINE(hit_block.address)]) {
        //   pf_filter.train(CACHE_LINE(hit_block.address), false, false, true, false, false, 1);
        //   pf_filter.pf_addr_pollution_map[CACHE_LINE(hit_block.address)] = false;
        // }
      // if (pf_filter.pf_addr_pollution_map[CACHE_LINE(hit_block.address)]) {
      //   pf_filter.train(CACHE_LINE(hit_block.address), false, false, true, true, false, true, 1);
      // }
      // else {
      //   pf_filter.train(CACHE_LINE(hit_block.address), true, false, true, false, false, true, 1);
      // }

      pf_filter.train(CACHE_LINE(hit_block.address), true, false, true, false, false, true, 1);
      }
      // pf_filter.train(hit_block.ip, true, lower_level->get_occupancy(1, handle_pkt.address));
    }
    if (DECISION_TREE_FILTERING) {
      decision_tree_predictor.update_result(true);
    }

    // if (PPF) {
    //   PERC_.perc_train_from_prefetch_table(hit_block.address, true);
    // }

    if (FDP) {
      fdp_control.useful_prefetch_total++;
      if (handle_pkt.type == LOAD) {
        fdp_control.coverage_total++;
      }
    }
    if (FDP_INTERVAL) {
      fdp_control.useful_prefetch_interval_counter++;
      if (handle_pkt.type == LOAD) {
        fdp_control.coverage_interval_counter++;
      }
    }
    if (HARMONY_PREDICTION) {
      if (prefetch_reuse_predictor->get_mark(CACHE_LINE(hit_block.address))) {
        prefetch_reuse_predictor->set_reuse(CACHE_LINE(hit_block.address));
      }
    }

    if (FEATURE_TEST) {
      pf_feature_filter.ip_accuracy_map[hit_block.ip].first++;
      pf_feature_filter.useful_prefetch_interval_counter++;
      pf_feature_filter.train(CACHE_LINE(hit_block.address), true, false, true);
    }

    hit_block.prefetch = 0;
  }
  // if (hit_block.prefetch) {
  // }
  if (hit_block.prefetch_until_evict) {
      tracer.cycle_coverage += (double) total_miss_latency / (double) total_miss;
      tracer.cycle_coverage_interval += (double) total_miss_latency / (double) total_miss;
      fdp_control.demand_hit_prefetched_block(2.0);
  }
}

bool CACHE::readlike_miss(PACKET& handle_pkt)
{
  DP(if (warmup_complete[handle_pkt.cpu]) {
    std::cout << "[" << NAME << "] " << __func__ << " miss";
    std::cout << " instr_id: " << handle_pkt.instr_id << " address: " << std::hex << (handle_pkt.address >> OFFSET_BITS);
    std::cout << " full_addr: " << handle_pkt.address;
    std::cout << " full_v_addr: " << handle_pkt.v_address << std::dec;
    std::cout << " type: " << +handle_pkt.type;
    std::cout << " cycle: " << current_cycle << std::endl;
  });

  // check mshr
  auto mshr_entry = std::find_if(MSHR.begin(), MSHR.end(), eq_addr<PACKET>(handle_pkt.address, OFFSET_BITS));
  bool mshr_full = (MSHR.size() == MSHR_SIZE);

  if (mshr_entry != MSHR.end()) // miss already inflight
  {
    // update fill location
    mshr_entry->fill_level = std::min(mshr_entry->fill_level, handle_pkt.fill_level);

    packet_dep_merge(mshr_entry->lq_index_depend_on_me, handle_pkt.lq_index_depend_on_me);
    packet_dep_merge(mshr_entry->sq_index_depend_on_me, handle_pkt.sq_index_depend_on_me);
    packet_dep_merge(mshr_entry->instr_depend_on_me, handle_pkt.instr_depend_on_me);
    packet_dep_merge(mshr_entry->to_return, handle_pkt.to_return);

    if (mshr_entry->type == PREFETCH && handle_pkt.type != PREFETCH) {
      // Mark the prefetch as useful
      if (mshr_entry->pf_origin_level == fill_level) {
        pf_useful++;
        

        // fdp increment accurate count
        if (FDP) {
          fdp_control.useful_prefetch_total++;
        }
        if (FDP_INTERVAL) {
          fdp_control.useful_prefetch_interval_counter++;
        }
        // fdp increment late count
        if (FDP) {
          fdp_control.late_prefetch_total++;
        }
        if (FDP_INTERVAL) {
          fdp_control.late_prefetch_interval_counter++;
        }
        if (PERCEPTRON_IP_FILTER) {
          // cout << "perceptron ip filter useful prefetch" << endl;
          pf_filter.ip_accuracy_map[mshr_entry->ip].first++;
          pf_filter.ip_late_map[mshr_entry->ip]++;
          pf_filter.late_prefetch_interval_counter++;
          pf_filter.useful_prefetch_interval_counter++;

          // train on late but useful prefetches?
          if (NO_DRAM_OCCUPANCY) {
            pf_filter.train(mshr_entry->ip, true, false);
          }
          else if (PERCEPTRON_DRAM_AVAILABILITY) {
            pf_filter.train(mshr_entry->ip, true, CACHE_LINE(mshr_entry->address));
          }
          else if (PERCEPTRON_HISTORY) {
            // pf_filter.train(mshr_entry->ip, mshr_entry->address, true, history[cpu]);
            // cout << "late prefetch cycles saved: " << current_cycle - mshr_entry->cycle_enqueued << endl;
            int cycles_saved = current_cycle - mshr_entry->cycle_enqueued;
            // if (cycles_saved)
            pf_filter.train(CACHE_LINE(mshr_entry->address), true, false, true, false, true, true, 1);
          }
        }

        // if (PPF) {
        //   PERC_.perc_train_from_prefetch_table(mshr_entry->address, true);
        // }

        if (DECISION_TREE_FILTERING) {
          decision_tree_predictor.update_result(true);
        }

        if (FEATURE_TEST) {
          pf_feature_filter.ip_accuracy_map[mshr_entry->ip].first++;
          pf_feature_filter.ip_late_map[mshr_entry->ip]++;
          pf_feature_filter.late_prefetch_interval_counter++;
          pf_feature_filter.useful_prefetch_interval_counter++;
          pf_feature_filter.train(CACHE_LINE(mshr_entry->address), true, false, true);
          
        }

        // mark the useful prefetch
        // tracer.ip_accuracy_map[mshr_entry->ip].first++;
        // mark the lateness
        tracer.late_total++;
        tracer.useful_prefetch_total++;
        tracer.late_interval_counter++;
        tracer.useful_prefetch_interval_counter++;
        // tracer.ip_late_map[mshr_entry->ip]++

        // fdp_control.late_prefetch(current_cycle - mshr_entry->cycle_enqueued);
        tracer.cycle_coverage += current_cycle - mshr_entry->cycle_enqueued;
        tracer.cycle_coverage_interval += current_cycle - mshr_entry->cycle_enqueued;
        fdp_control.late_prefetch(1.0);
        
      }

      uint64_t prior_event_cycle = mshr_entry->event_cycle;
      *mshr_entry = handle_pkt;

      // in case request is already returned, we should keep event_cycle
      mshr_entry->event_cycle = prior_event_cycle;
    }
  } else {
    if (mshr_full)  // not enough MSHR resource
      return false; // TODO should we allow prefetches anyway if they will not
                    // be filled to this level?

    bool is_read = prefetch_as_load || (handle_pkt.type != PREFETCH);

    // check to make sure the lower level queue has room for this read miss
    int queue_type = (is_read) ? 1 : 3;
    if (lower_level->get_occupancy(queue_type, handle_pkt.address) == lower_level->get_size(queue_type, handle_pkt.address))
      return false;

    // Allocate an MSHR
    if (handle_pkt.fill_level <= fill_level) {
      auto it = MSHR.insert(std::end(MSHR), handle_pkt);
      it->cycle_enqueued = current_cycle;
      it->event_cycle = std::numeric_limits<uint64_t>::max();
    }

    if (handle_pkt.fill_level <= fill_level)
      handle_pkt.to_return = {this};
    else
      handle_pkt.to_return.clear();

    if (!is_read)
      lower_level->add_pq(&handle_pkt);
    else
      lower_level->add_rq(&handle_pkt);

    if (NAME == "LLC") {
          dram_availability = (double) (lower_level->get_size(1, handle_pkt.address) - lower_level->get_occupancy(1, handle_pkt.address)) / (double) lower_level->get_size(1, handle_pkt.address);
          dram_occupancy = lower_level->get_occupancy(1, handle_pkt.address);
          // cout << "LLC dram availability: " << dram_availability << endl;
          // cout << "LLC occupancy: " << lower_level->get_occupancy(1, handle_pkt.address) << endl;
          // cout << "LLC dram size: " << lower_level->get_size(1, handle_pkt.address) << endl;
    }
  }

  // update prefetcher on load instructions and prefetches from upper levels
  if (should_activate_prefetcher(handle_pkt.type) && handle_pkt.pf_origin_level < fill_level) {
    cpu = handle_pkt.cpu;
    uint64_t pf_base_addr = (virtual_prefetch ? handle_pkt.v_address : handle_pkt.address) & ~bitmask(match_offset_bits ? 0 : OFFSET_BITS);
    handle_pkt.pf_metadata = impl_prefetcher_cache_operate(pf_base_addr, handle_pkt.ip, 0, handle_pkt.type, handle_pkt.pf_metadata, handle_pkt.instr_id, ooo_cpu[cpu]->current_cycle);

    if (PERCEPTRON_HISTORY || FEATURE_TEST) {
      auto iter = find(history[cpu].begin(), history[cpu].end(), handle_pkt.ip);
      if (iter != history[cpu].end()) {
        history[cpu].erase(iter);
      }
      history[cpu].push_back(handle_pkt.ip);
      if (history[cpu].size() > HISTORY_LEN) {
        history[cpu].erase(history[cpu].begin());
      }

      if (PERCEPTRON_REJECT_CACHE) {
        if (PERCEPTRON_REJECT_CACHE_LOGGING) {
          cout << "updating reject cache" << endl;
          pf_filter.print_reject_cache(get_set(handle_pkt.address));
        }
        pf_filter.update_reject_cache(get_set(handle_pkt.address));
        if (PERCEPTRON_REJECT_CACHE_LOGGING) {
          pf_filter.print_reject_cache(get_set(handle_pkt.address));
        }

      }
    }
    
  }

  return true;
}

bool CACHE::filllike_miss(std::size_t set, std::size_t way, PACKET& handle_pkt)
{
  DP(if (warmup_complete[handle_pkt.cpu]) {
    std::cout << "[" << NAME << "] " << __func__ << " miss";
    std::cout << " instr_id: " << handle_pkt.instr_id << " address: " << std::hex << (handle_pkt.address >> OFFSET_BITS);
    std::cout << " full_addr: " << handle_pkt.address;
    std::cout << " full_v_addr: " << handle_pkt.v_address << std::dec;
    std::cout << " type: " << +handle_pkt.type;
    std::cout << " cycle: " << current_cycle << std::endl;
  });

  bool bypass = (way == NUM_WAY);
#ifndef LLC_BYPASS
  assert(!bypass);
#endif
  assert(handle_pkt.type != WRITEBACK || !bypass);

  BLOCK& fill_block = block[set * NUM_WAY + way];
  bool evicting_dirty = !bypass && (lower_level != NULL) && fill_block.dirty;
  uint64_t evicting_address = 0;
  bool fill_is_prefetch = false;

  if (!bypass) {
    if (evicting_dirty) {
      PACKET writeback_packet;

      writeback_packet.fill_level = lower_level->fill_level;
      writeback_packet.cpu = handle_pkt.cpu;
      writeback_packet.address = fill_block.address;
      writeback_packet.data = fill_block.data;
      writeback_packet.instr_id = handle_pkt.instr_id;
      writeback_packet.ip = 0;
      writeback_packet.type = WRITEBACK;

      auto result = lower_level->add_wq(&writeback_packet);
      if (result == -2)
        return false;
    }

    if (ever_seen_data)
      evicting_address = fill_block.address & ~bitmask(match_offset_bits ? 0 : OFFSET_BITS);
    else
      evicting_address = fill_block.v_address & ~bitmask(match_offset_bits ? 0 : OFFSET_BITS);

    if (fill_block.prefetch) {
      pf_useless++;
      // negatively train on useless prefetches
      if (PERCEPTRON_IP_FILTER) {
        if (NO_DRAM_OCCUPANCY) {
          pf_filter.train(fill_block.ip, false, false);
        }
        else if (PERCEPTRON_DRAM_AVAILABILITY) {
          pf_filter.train(fill_block.ip, false, CACHE_LINE(fill_block.address));
        }
        else if (PERCEPTRON_HISTORY) {
          // double prefetch_fraction = (pf_filter.demand_miss_interval_counter > 0) ? (double) pf_filter.prefetch_miss_interval_counter / (double) pf_filter.demand_miss_interval_counter : 0.0;
          // if ((fill_block.possible_pollution && handle_pkt.type == LOAD) 
          //   || (
          //     pf_filter.addr_to_dram_availability_map.find(CACHE_LINE(fill_block.address)) != pf_filter.addr_to_dram_availability_map.end() && pf_filter.addr_to_dram_availability_map[CACHE_LINE(fill_block.address)] < 0.90
          //     // pf_filter.prefetch_miss_interval_counter > pf_filter.demand_miss_interval_counter
          //     // prefetch_fraction > 0.95
          //     )
          //     ) {
          //   pf_filter.train(CACHE_LINE(fill_block.address), false, false);
          //   pf_filter.addr_to_dram_availability_map.erase(CACHE_LINE(fill_block.address));
          // }

          // if (pf_filter.addr_to_dram_availability_map.find(CACHE_LINE(fill_block.address)) != pf_filter.addr_to_dram_availability_map.end() && pf_filter.addr_to_dram_availability_map[CACHE_LINE(fill_block.address)] < 0.5) {
          //   pf_filter.train(CACHE_LINE(fill_block.address), false, false, false);
          // }
          // cout << "training on address: " << fill_block.address << endl;
            // else {
            //   if (pf_filter.training_type == Training_Type::NORMAL) {
            //     pf_filter.train(CACHE_LINE(fill_block.address), false, false, false, false, false, true, 1);
            //   }
            // }

        // if (pf_filter.normal_false_training) {
        //     pf_filter.train(CACHE_LINE(fill_block.address), false, false, false, false, false, true, 1);
        // }

          pf_filter.total_false++;
          if (pf_filter.pf_addr_pollution_map[CACHE_LINE(fill_block.address)]) {
            pf_filter.num_false_with_negative_impact++;
            pf_filter.train(CACHE_LINE(fill_block.address), false, false, false, true, false, false, 1);
            pf_filter.pf_addr_pollution_map[CACHE_LINE(fill_block.address)] = false;
          }
          else if (pf_filter.pf_addr_blocking_demand_map[CACHE_LINE(fill_block.address)]) {
            pf_filter.num_false_with_negative_impact++;
            pf_filter.train(CACHE_LINE(fill_block.address), false, false, false, false, false, false, 1);
            pf_filter.pf_addr_blocking_demand_map[CACHE_LINE(fill_block.address)] = false;
          }
          else {
            pf_filter.train(CACHE_LINE(fill_block.address), false, false, false, false, false, true, 1);
          }
        
          // if (pf_filter.training_type == Training_Type::NORMAL) {
          //   pf_filter.train(CACHE_LINE(fill_block.address), false, false, false, false, false, true, 1);
          // }
          // else if (pf_filter.training_type == Training_Type::HIGH) {
          //   pf_filter.train(CACHE_LINE(fill_block.address), false, false, false, false, false, true, 2);
          // }
          // else {
          // }

          // if (fill_block.pollution) {
          //   pf_filter.train(CACHE_LINE(fill_block.address), false, false, false, 3);
          // }
        }
      }

      if (FEATURE_TEST) {
        pf_feature_filter.train(CACHE_LINE(fill_block.address), false, false, false);
      }

      if (DECISION_TREE_FILTERING) {
        decision_tree_predictor.update_result(false);
      }

      // if (PPF) {
      //   PERC_.perc_train_from_prefetch_table(fill_block.address, false);
      // }
      double dram_occupancy = (double) lower_level->get_occupancy(1, fill_block.address) / (double) lower_level->get_size(1, fill_block.address);
      fdp_control.useless_prefetch(2*dram_occupancy);
    }

    if (handle_pkt.type == PREFETCH)
      pf_fill++;
    
    if (NAME == L2_NAME && !fill_block.prefetch_until_evict && handle_pkt.type == PREFETCH) {
      fill_block.evicted_demand_address = CACHE_LINE(fill_block.address);
      fill_block.possible_pollution = true;
      // cout << "setting evicted demand address: " << CACHE_LINE(fill_block.address) << endl;
      // if (ADAPTIVE_PREFETCH_FILTER) {
      //   pf_filter.add_prefetch_victim_addr_pair(CACHE_LINE(handle_pkt.address), CACHE_LINE(fill_block.address));
      // }
    }
    else {
      fill_block.possible_pollution = false;
    }

    pf_filter.pf_addr_blocking_demand_map[CACHE_LINE(fill_block.address)] = false;

    if (ADAPTIVE_PREFETCH_FILTER) {
      pf_filter.update_apf_with_victim_cache_miss(CACHE_LINE(fill_block.address));
    }


    fill_is_prefetch = fill_block.prefetch_until_evict;

    fill_block.valid = true;
    fill_block.prefetch = (handle_pkt.type == PREFETCH && handle_pkt.pf_origin_level == fill_level);
    fill_block.prefetch_until_evict = (handle_pkt.type == PREFETCH && handle_pkt.pf_origin_level == fill_level);
    fill_block.dirty = (handle_pkt.type == WRITEBACK || (handle_pkt.type == RFO && handle_pkt.to_return.empty()));
    fill_block.address = handle_pkt.address;
    fill_block.v_address = handle_pkt.v_address;
    fill_block.data = handle_pkt.data;
    fill_block.ip = handle_pkt.ip;
    fill_block.cpu = handle_pkt.cpu;
    fill_block.instr_id = handle_pkt.instr_id;
    fill_block.pollution = false;
  }

  if (warmup_complete[handle_pkt.cpu] && (handle_pkt.cycle_enqueued != 0)) {
    total_miss_latency += current_cycle - handle_pkt.cycle_enqueued;
    total_miss++;
    miss_latency_vector.push_back(current_cycle - handle_pkt.cycle_enqueued);

    if (FDP_INTERVAL) {
      if (!fill_is_prefetch) {
        fdp_control.demand_miss_latency_interval_counter += current_cycle - handle_pkt.cycle_enqueued;
      }
      else {
        fdp_control.prefetch_miss_latency_interval_counter += current_cycle - handle_pkt.cycle_enqueued;
        fdp_control.prefetch_miss_interval_counter++;
      }
    }
    if (PERCEPTRON_IP_FILTER) {
       if (!fill_is_prefetch) {
        pf_filter.demand_miss_latency_interval_counter += current_cycle - handle_pkt.cycle_enqueued;
      }
      else {
        pf_filter.prefetch_miss_latency_interval_counter += current_cycle - handle_pkt.cycle_enqueued;
      }
    }
  }


  // update prefetcher
  cpu = handle_pkt.cpu;
  handle_pkt.pf_metadata =
      impl_prefetcher_cache_fill((virtual_prefetch ? handle_pkt.v_address : handle_pkt.address) & ~bitmask(match_offset_bits ? 0 : OFFSET_BITS), set, way,
                                 handle_pkt.type == PREFETCH, evicting_address, handle_pkt.pf_metadata, ooo_cpu[cpu]->current_cycle);
  // if (PPF) {
  //   PERC_.update_prefetch_and_reject_table(evicting_address);
  // }

  // update replacement policy
  impl_replacement_update_state(handle_pkt.cpu, set, way, handle_pkt.address, handle_pkt.ip, 0, handle_pkt.type, 0);

  if (PERCEPTRON_REJECT_CACHE) {
    if (PERCEPTRON_REJECT_CACHE_LOGGING) {
      cout << "updating reject cache" << endl;
      pf_filter.print_reject_cache(set);
    }
    pf_filter.update_reject_cache(set);
    if (PERCEPTRON_REJECT_CACHE_LOGGING) {
      pf_filter.print_reject_cache(set);
    }
  }

  // COLLECT STATS
  sim_miss[handle_pkt.cpu][handle_pkt.type]++;
  sim_access[handle_pkt.cpu][handle_pkt.type]++;

  return true;
}

void CACHE::operate()
{
  operate_writes();
  operate_reads();

  if (NAME == "LLC" && (FDP || FDP_INTERVAL || COST_MODEL || COST_MODEL_INTERVAL) && fdp_control.is_interval_complete()) {
    int prev_pref_degree = get_prefetcher_degree(cpu);
    Control_Response cr = fdp_control.prefetcher_control(this->current_cycle, pf_filter.degree_accuracy_map[prev_pref_degree].first, pf_filter.degree_accuracy_map[prev_pref_degree].second);
    switch(cr) {
      case INCREMENT: {
        int new_degree = min(prev_pref_degree+1, 6);
        set_prefetcher_degree(new_degree, cpu);
        fdp_control.fdp_interval_vec.push_back({INCREMENT, new_degree});
        control_response = INCREMENT;
        if (FDP_LOGGING) {
          cout << "INCREMENT" << endl;
        }
        break;
      }
      case DECREMENT: {
        int new_degree = max(prev_pref_degree-1, 4);
        set_prefetcher_degree(new_degree, cpu);
        fdp_control.fdp_interval_vec.push_back({DECREMENT, new_degree});
        control_response = DECREMENT;
        if (FDP_LOGGING) {
          cout << "DECREMENT" << endl;
        }
        break;
      }
      case NO_CHANGE: {
        fdp_control.fdp_interval_vec.push_back({NO_CHANGE, prev_pref_degree});
        control_response = NO_CHANGE;
        if (FDP_LOGGING) {
          cout << "NO CHANGE" << endl;
        }
      }
      default: {
        break;
      }
    }
    fdp_control.reset_interval(this->current_cycle);
  }

  if ((PERCEPTRON_IP_FILTER) && pf_filter.is_interval_complete()) {
    // cout << "NAME: " << NAME << endl;
    pf_filter.reset_interval(this->current_cycle, get_prefetcher_degree(cpu));
    // set_prefetcher_degree(new_degree, cpu);
    // pf_filter.verify_stats(tracer.useful_prefetch_total, tracer.num_total_prefetch_fills, tracer.late_total, tracer.pollution_total);
  }
  if (FEATURE_TEST && pf_feature_filter.is_interval_complete()) {
    pf_feature_filter.reset_interval(this->current_cycle, get_prefetcher_degree(cpu));
  }
  impl_prefetcher_cycle_operate();
}

void CACHE::operate_writes()
{
  // perform all writes
  writes_available_this_cycle = MAX_WRITE;
  handle_fill();
  handle_writeback();

  WQ.operate();
}

void CACHE::operate_reads()
{
  // perform all reads
  reads_available_this_cycle = MAX_READ;
  handle_read();
  va_translate_prefetches();
  handle_prefetch();

  RQ.operate();
  PQ.operate();
  VAPQ.operate();
}

uint32_t CACHE::get_set(uint64_t address) { return ((address >> OFFSET_BITS) & bitmask(lg2(NUM_SET))); }

uint32_t CACHE::get_way(uint64_t address, uint32_t set)
{
  auto begin = std::next(block.begin(), set * NUM_WAY);
  auto end = std::next(begin, NUM_WAY);
  return std::distance(begin, std::find_if(begin, end, eq_addr<BLOCK>(address, OFFSET_BITS)));
}

int CACHE::invalidate_entry(uint64_t inval_addr)
{
  uint32_t set = get_set(inval_addr);
  uint32_t way = get_way(inval_addr, set);

  if (way < NUM_WAY) {
    block[set * NUM_WAY + way].valid = 0;
  }

  return way;
}

int CACHE::add_rq(PACKET* packet)
{
  assert(packet->address != 0);
  RQ_ACCESS++;

  DP(if (warmup_complete[packet->cpu]) {
    std::cout << "[" << NAME << "_RQ] " << __func__ << " instr_id: " << packet->instr_id << " address: " << std::hex << (packet->address >> OFFSET_BITS);
    std::cout << " full_addr: " << packet->address << " v_address: " << packet->v_address << std::dec << " type: " << +packet->type
              << " occupancy: " << RQ.occupancy();
  })

  // check for the latest writebacks in the write queue
  champsim::delay_queue<PACKET>::iterator found_wq = std::find_if(WQ.begin(), WQ.end(), eq_addr<PACKET>(packet->address, match_offset_bits ? 0 : OFFSET_BITS));

  if (found_wq != WQ.end()) {

    DP(if (warmup_complete[packet->cpu]) std::cout << " MERGED_WQ" << std::endl;)

    packet->data = found_wq->data;
    for (auto ret : packet->to_return)
      ret->return_data(packet);

    WQ_FORWARD++;
    return -1;
  }

  // check for duplicates in the read queue
  auto found_rq = std::find_if(RQ.begin(), RQ.end(), eq_addr<PACKET>(packet->address, OFFSET_BITS));
  if (found_rq != RQ.end()) {

    DP(if (warmup_complete[packet->cpu]) std::cout << " MERGED_RQ" << std::endl;)

    packet_dep_merge(found_rq->lq_index_depend_on_me, packet->lq_index_depend_on_me);
    packet_dep_merge(found_rq->sq_index_depend_on_me, packet->sq_index_depend_on_me);
    packet_dep_merge(found_rq->instr_depend_on_me, packet->instr_depend_on_me);
    packet_dep_merge(found_rq->to_return, packet->to_return);

    RQ_MERGED++;

    return 0; // merged index
  }

  // check occupancy
  if (RQ.full()) {
    RQ_FULL++;

    DP(if (warmup_complete[packet->cpu]) std::cout << " FULL" << std::endl;)

    return -2; // cannot handle this request
  }

  // if there is no duplicate, add it to RQ
  if (warmup_complete[cpu])
    RQ.push_back(*packet);
  else
    RQ.push_back_ready(*packet);

  DP(if (warmup_complete[packet->cpu]) std::cout << " ADDED" << std::endl;)

  RQ_TO_CACHE++;
  return RQ.occupancy();
}

int CACHE::add_wq(PACKET* packet)
{
  WQ_ACCESS++;

  DP(if (warmup_complete[packet->cpu]) {
    std::cout << "[" << NAME << "_WQ] " << __func__ << " instr_id: " << packet->instr_id << " address: " << std::hex << (packet->address >> OFFSET_BITS);
    std::cout << " full_addr: " << packet->address << " v_address: " << packet->v_address << std::dec << " type: " << +packet->type
              << " occupancy: " << RQ.occupancy();
  })

  // check for duplicates in the write queue
  champsim::delay_queue<PACKET>::iterator found_wq = std::find_if(WQ.begin(), WQ.end(), eq_addr<PACKET>(packet->address, match_offset_bits ? 0 : OFFSET_BITS));

  if (found_wq != WQ.end()) {

    DP(if (warmup_complete[packet->cpu]) std::cout << " MERGED" << std::endl;)

    WQ_MERGED++;
    return 0; // merged index
  }

  // Check for room in the queue
  if (WQ.full()) {
    DP(if (warmup_complete[packet->cpu]) std::cout << " FULL" << std::endl;)

    ++WQ_FULL;
    return -2;
  }

  // if there is no duplicate, add it to the write queue
  if (warmup_complete[cpu])
    WQ.push_back(*packet);
  else
    WQ.push_back_ready(*packet);

  DP(if (warmup_complete[packet->cpu]) std::cout << " ADDED" << std::endl;)

  WQ_TO_CACHE++;
  WQ_ACCESS++;

  return WQ.occupancy();
}

int CACHE::prefetch_line(uint64_t pf_addr, bool fill_this_level, uint32_t prefetch_metadata, uint64_t ip, uint64_t prefetch_trigger_addr, uint64_t prefetch_offset, uint64_t score)
{
  pf_requested++;

  if (HARMONY_PREDICTION) {
    bool pred = prefetch_predictor->get_prediction(ip);
    if (!pred) {
      if (!prefetch_reuse_predictor->get_prediction(CACHE_LINE(pf_addr))) {
        // cout << "suppressed prefetch for ip: " << ip << endl;
        // prefetch_reuse_predictor->print_SHCT_ip(ip);
        pf_suppressed++;
        return 0;
      }
      
    }

  }

  if (ACCURACY_THRESHOLD) {
    int accurate = tracer.get_accurate_counter_ip(ip);
    int total = tracer.get_total_counter_ip(ip);

    if (training_complete && total > 0) {
        double accuracy = (double) accurate/(double)total;
        double impact = (double) total / (double) tracer.num_total_prefetch_fills;

        if (accuracy <= accuracy_threshold) {
            pf_suppressed++; 
            return 0;

        }
    }
  }

  if (COMBINED_HARMONY_ACCURACY) {
    bool pred = prefetch_predictor->get_prediction(ip);
    int accurate = tracer.get_accurate_counter_ip(ip);
    int total = tracer.get_total_counter_ip(ip);

    double accuracy = (double) accurate/(double)total;
    if (training_complete && !pred && accuracy < 0.50) {
      pf_suppressed++;
      return 0;
    }
  } 

  // if (NAME == L2_NAME && PERCEPTRON_IP_FILTER && PERCEPTRON_HISTORY) {
  //       PPF_PRED prefetch_action = pf_filter.get_prediction(ip, CACHE_LINE(pf_addr), prefetch_metadata, 0, get_set(pf_addr), prefetch_trigger_addr, prefetch_offset, lower_level->dram_occupancy, false, 0, true, false, warmup_complete[cpu], history[cpu]);
  //       if (prefetch_action == SUPPRESS) {
  //         pf_suppressed++;
  //         return 0;
  //       }
  // }

  // if (PERCEPTRON_IP_FILTER) {
  //   bool pred = false;
  //   double dram_availability = (double) (lower_level->get_size(1, pf_addr) - lower_level->get_occupancy(1, pf_addr)) / (double) lower_level->get_size(1, pf_addr);
  //   if (NO_DRAM_OCCUPANCY) {
  //     pred = pf_filter.get_prediction(ip);
  //   }
  //   else if (PERCEPTRON_DRAM_AVAILABILITY) {
  //     pred = pf_filter.get_prediction(ip, dram_availability, CACHE_LINE(pf_addr), prefetch_metadata, prefetch_offset);
  //   }
  //   else if (PERCEPTRON_HISTORY) {
  //     bool harmony_pred = false;
  //     int rrpv = 0;
  //     if (PERCEPTRON_WITH_HARMONY) {
  //       harmony_pred = prefetch_predictor->get_prediction(ip);
  //       if (!harmony_pred) {
  //         rrpv = 7;
  //       }
  //     }
  //     PPF_PRED prefetch_action = pf_filter.get_prediction(ip, CACHE_LINE(pf_addr), prefetch_metadata, rrpv, get_set(pf_addr), prefetch_trigger_addr, prefetch_offset, dram_availability, harmony_pred, score, false, history[cpu]);
  //     if (prefetch_action == SUPPRESS) {
  //       pf_suppressed++;
  //       return 0;
  //     }
  //     // else if (prefetch_action == PREFETCH_LLC) {
  //     //   fill_this_level = false;
  //     // }
  //   }
  //   pf_filter.addr_to_dram_availability_map[CACHE_LINE(pf_addr)] = dram_availability;
  //   // if (!pred) {
  //   //   pf_suppressed++;
  //   //   return 0;
  //   // }
  // }

  if (ADAPTIVE_PREFETCH_FILTER) {
    bool pred = pf_filter.get_apf_prediction(CACHE_LINE(pf_addr));
    if (pred) {
      pf_suppressed++;
      return 0;
    }
  }

  // if (PPF) {
  //   uint64_t ip_1 = (history[cpu].size() > 0) ? history[cpu][0] : 0;
  //   uint64_t ip_2 = (history[cpu].size() > 1) ? history[cpu][1] : 0;
  //   uint64_t ip_3 = (history[cpu].size() > 1) ? history[cpu][2] : 0;
  //     PPF_PRED pred_action = PERC_.get_perc_prediction(prefetch_trigger_addr, pf_addr, ip, ip_1, ip_2, ip_3, prefetch_offset, prefetch_metadata);
  //     if (pred_action == SUPPRESS) {
  //       pf_suppressed++;
  //       return 0;
  //     }
  //     else if (pred_action == PREFETCH_LLC) {
  //       fill_this_level = false;
  //     }
  // }

  // if (FEATURE_TEST) {
  //   bool pred = pf_feature_filter.get_prediction(ip, CACHE_LINE(pf_addr), prefetch_metadata, 0, get_set(pf_addr), prefetch_trigger_addr, prefetch_offset, lower_level->dram_availability, lower_level->dram_occupancy, false, score, history[cpu]);
  //   if (!pred) {
  //     pf_suppressed++;
  //     return 0;
  //   }
  // }

  // tracer.dram_occupancy_at_prefetch_issue += lower_level->get_occupancy(1, pf_addr);
  // tracer.num_issued_prefetches_interval++;



  PACKET pf_packet;
  pf_packet.type = PREFETCH;
  pf_packet.fill_level = (fill_this_level ? fill_level : lower_level->fill_level);
  pf_packet.pf_origin_level = fill_level;
  pf_packet.pf_metadata = prefetch_metadata;
  pf_packet.prefetch_degree = prefetch_metadata;
  pf_packet.cpu = cpu;
  pf_packet.address = pf_addr;
  pf_packet.v_address = virtual_prefetch ? pf_addr : 0;
  pf_packet.ip = ip;
  pf_packet.pf_trigger_addr = prefetch_trigger_addr;
  pf_packet.pf_offset = prefetch_offset;

  if (virtual_prefetch) {
    if (!VAPQ.full()) {
      VAPQ.push_back(pf_packet);
      return 1;
    }
  } else {
    int result = add_pq(&pf_packet);
    if (result != -2) {
      if (result > 0)
        pf_issued++;
      return 1;
    }
  }
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
  return 0;
}

int CACHE::prefetch_line(uint64_t ip, uint64_t base_addr, uint64_t pf_addr, bool fill_this_level, uint32_t prefetch_metadata, uint64_t prefetch_trigger_addr, uint64_t prefetch_offset, uint64_t score)
{
  static bool deprecate_printed = false;
  if (!deprecate_printed) {
    std::cout << "WARNING: The extended signature CACHE::prefetch_line(ip, "
                 "base_addr, pf_addr, fill_this_level, prefetch_metadata) is "
                 "deprecated."
              << std::endl;
    std::cout << "WARNING: Use CACHE::prefetch_line(pf_addr, fill_this_level, "
                 "prefetch_metadata) instead."
              << std::endl;
    deprecate_printed = true;
  }
  return prefetch_line(pf_addr, fill_this_level, prefetch_metadata, ip, prefetch_trigger_addr, prefetch_offset, score);
}

void CACHE::va_translate_prefetches()
{
  // TEMPORARY SOLUTION: mark prefetches as translated after a fixed latency
  if (VAPQ.has_ready()) {
    VAPQ.front().address = vmem.va_to_pa(cpu, VAPQ.front().v_address).first;

    // move the translated prefetch over to the regular PQ
    int result = add_pq(&VAPQ.front());

    // remove the prefetch from the VAPQ
    if (result != -2)
      VAPQ.pop_front();

    if (result > 0)
      pf_issued++;
  }
}

int CACHE::add_pq(PACKET* packet)
{
  assert(packet->address != 0);
  PQ_ACCESS++;

  DP(if (warmup_complete[packet->cpu]) {
    std::cout << "[" << NAME << "_WQ] " << __func__ << " instr_id: " << packet->instr_id << " address: " << std::hex << (packet->address >> OFFSET_BITS);
    std::cout << " full_addr: " << packet->address << " v_address: " << packet->v_address << std::dec << " type: " << +packet->type
              << " occupancy: " << RQ.occupancy();
  })

  // check for the latest wirtebacks in the write queue
  champsim::delay_queue<PACKET>::iterator found_wq = std::find_if(WQ.begin(), WQ.end(), eq_addr<PACKET>(packet->address, match_offset_bits ? 0 : OFFSET_BITS));

  if (found_wq != WQ.end()) {

    DP(if (warmup_complete[packet->cpu]) std::cout << " MERGED_WQ" << std::endl;)

    packet->data = found_wq->data;
    for (auto ret : packet->to_return)
      ret->return_data(packet);

    WQ_FORWARD++;
    return -1;
  }

  // check for duplicates in the PQ
  auto found = std::find_if(PQ.begin(), PQ.end(), eq_addr<PACKET>(packet->address, OFFSET_BITS));
  if (found != PQ.end()) {
    DP(if (warmup_complete[packet->cpu]) std::cout << " MERGED_PQ" << std::endl;)

    found->fill_level = std::min(found->fill_level, packet->fill_level);
    packet_dep_merge(found->to_return, packet->to_return);

    PQ_MERGED++;
    return 0;
  }

  // check occupancy
  if (PQ.full()) {

    DP(if (warmup_complete[packet->cpu]) std::cout << " FULL" << std::endl;)

    PQ_FULL++;
    return -2; // cannot handle this request
  }

  // if there is no duplicate, add it to PQ
  if (warmup_complete[cpu])
    PQ.push_back(*packet);
  else
    PQ.push_back_ready(*packet);

  DP(if (warmup_complete[packet->cpu]) std::cout << " ADDED" << std::endl;)

  PQ_TO_CACHE++;
  return PQ.occupancy();
}

void CACHE::return_data(PACKET* packet)
{
  // check MSHR information
  auto mshr_entry = std::find_if(MSHR.begin(), MSHR.end(), eq_addr<PACKET>(packet->address, OFFSET_BITS));
  auto first_unreturned = std::find_if(MSHR.begin(), MSHR.end(), [](auto x) { return x.event_cycle == std::numeric_limits<uint64_t>::max(); });

  // sanity check
  if (mshr_entry == MSHR.end()) {
    std::cerr << "[" << NAME << "_MSHR] " << __func__ << " instr_id: " << packet->instr_id << " cannot find a matching entry!";
    std::cerr << " address: " << std::hex << packet->address;
    std::cerr << " v_address: " << packet->v_address;
    std::cerr << " address: " << (packet->address >> OFFSET_BITS) << std::dec;
    std::cerr << " event: " << packet->event_cycle << " current: " << current_cycle << std::endl;
    assert(0);
  }

  // MSHR holds the most updated information about this request
  mshr_entry->data = packet->data;
  mshr_entry->pf_metadata = packet->pf_metadata;
  mshr_entry->event_cycle = current_cycle + (warmup_complete[cpu] ? FILL_LATENCY : 0);

  DP(if (warmup_complete[packet->cpu]) {
    std::cout << "[" << NAME << "_MSHR] " << __func__ << " instr_id: " << mshr_entry->instr_id;
    std::cout << " address: " << std::hex << (mshr_entry->address >> OFFSET_BITS) << " full_addr: " << mshr_entry->address;
    std::cout << " data: " << mshr_entry->data << std::dec;
    std::cout << " index: " << std::distance(MSHR.begin(), mshr_entry) << " occupancy: " << get_occupancy(0, 0);
    std::cout << " event: " << mshr_entry->event_cycle << " current: " << current_cycle << std::endl;
  });

  // Order this entry after previously-returned entries, but before non-returned
  // entries
  std::iter_swap(mshr_entry, first_unreturned);
}

uint32_t CACHE::get_occupancy(uint8_t queue_type, uint64_t address)
{
  if (queue_type == 0)
    return std::count_if(MSHR.begin(), MSHR.end(), is_valid<PACKET>());
  else if (queue_type == 1)
    return RQ.occupancy();
  else if (queue_type == 2)
    return WQ.occupancy();
  else if (queue_type == 3)
    return PQ.occupancy();

  return 0;
}

uint32_t CACHE::get_size(uint8_t queue_type, uint64_t address)
{
  if (queue_type == 0)
    return MSHR_SIZE;
  else if (queue_type == 1)
    return RQ.size();
  else if (queue_type == 2)
    return WQ.size();
  else if (queue_type == 3)
    return PQ.size();

  return 0;
}

uint32_t CACHE::get_bank_occupancy(uint8_t queue_type, uint64_t address) {
  return 0;
}

bool CACHE::should_activate_prefetcher(int type) { return (1 << static_cast<int>(type)) & pref_activate_mask; }

void CACHE::print_deadlock()
{
  if (!std::empty(MSHR)) {
    std::cout << NAME << " MSHR Entry" << std::endl;
    std::size_t j = 0;
    for (PACKET entry : MSHR) {
      std::cout << "[" << NAME << " MSHR] entry: " << j++ << " instr_id: " << entry.instr_id;
      std::cout << " address: " << std::hex << (entry.address >> LOG2_BLOCK_SIZE) << " full_addr: " << entry.address << std::dec << " type: " << +entry.type;
      std::cout << " fill_level: " << +entry.fill_level << " event_cycle: " << entry.event_cycle << std::endl;
    }
  } else {
    std::cout << NAME << " MSHR empty" << std::endl;
  }
}

// int CACHE::get_prefetcher_degree(int cpu) {
//   return 0;
// }

// void CACHE::set_prefetcher_degree(int cpu, int degree) {
//   return;
// } 
