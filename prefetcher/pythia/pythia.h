#ifndef PYTHIA_H
#define PYTHIA_H

#include <vector>

#include "cache.h"
#include "champsim.h"
#include "champsim_constants.h"
#include "learning_engine_basic.h"
#include "learning_engine_featurewise.h"
#include "prefetcher.h"
#include "pythia_helper.h"
#include <unordered_map>

using namespace std;

#define MAX_ACTIONS 64
#define MAX_REWARDS 16
#define MAX_PYTHIA_DEGREE 16
#define PYTHIA_MAX_IPC_LEVEL 4

/* forward declaration */
class LearningEngine;

class Pythia : public Prefetcher
{
private:
  deque<Pythia_STEntry*> signature_table;
  LearningEngineBasic* brain;
  LearningEngineFeaturewise* brain_featurewise;
  deque<Pythia_PTEntry*> prefetch_tracker;
  deque<Pythia_PTEntry*> prefetch_tracker_lowconf; // Only used if pythia_separate_lowconf_pt = true
  Pythia_PTEntry* last_evicted_tracker;
  Pythia_PTEntry* last_evicted_tracker_lowconf; // Only used if pythia_separate_lowconf_pt = true
  uint8_t bw_level;
  uint8_t core_ipc;
  uint32_t acc_level;

  /* for workload insights only
   * has nothing to do with prefetching */
  PythiaRecorder* recorder;

  /* Data structures for debugging */
  unordered_map<string, uint64_t> target_action_state;

  struct {
    struct {
      uint64_t lookup;
      uint64_t hit;
      uint64_t evict;
      uint64_t insert;
      uint64_t streaming;
    } st;

    struct {
      uint64_t called;
      uint64_t out_of_bounds;
      vector<uint64_t> action_dist;
      vector<uint64_t> issue_dist;
      vector<uint64_t> pred_hit;
      vector<uint64_t> out_of_bounds_dist;
      uint64_t predicted;
      uint64_t multi_deg;
      uint64_t multi_deg_called;
      uint64_t multi_deg_histogram[MAX_PYTHIA_DEGREE + 1];
      uint64_t deg_histogram[MAX_PYTHIA_DEGREE + 1];
    } predict;

    struct {
      uint64_t called;
      uint64_t same_address;
      uint64_t evict;
    } track;

    struct {
      struct {
        uint64_t called;
        uint64_t pt_not_found;
        uint64_t pt_found;
        uint64_t pt_found_total;
        uint64_t has_reward;
      } demand;

      struct {
        uint64_t called;
      } train;

      struct {
        uint64_t called;
      } assign_reward;

      struct {
        uint64_t dist[MAX_REWARDS][2];
      } compute_reward;

      uint64_t correct_timely;
      uint64_t correct_untimely;
      uint64_t no_pref;
      uint64_t incorrect;
      uint64_t out_of_bounds;
      uint64_t tracker_hit;
      uint64_t correct_timely_lowconf;
      uint64_t correct_untimely_lowconf;
      uint64_t incorrect_lowconf;
      uint64_t dist[MAX_ACTIONS][MAX_REWARDS];
    } reward;

    struct {
      uint64_t called;
      uint64_t compute_reward;
    } train;

    struct {
      uint64_t called;
      uint64_t set;
      uint64_t set_total;
      uint64_t set_actions[MAX_ACTIONS];
      uint64_t set_conf[2];
    } register_fill;

    struct {
      uint64_t called;
      uint64_t set;
      uint64_t set_total;
      uint64_t set_actions[MAX_ACTIONS];
      uint64_t set_conf[2];
    } register_llc_fill;

    struct {
      uint64_t called;
      uint64_t set;
      uint64_t set_total;
    } register_prefetch_hit;

    struct {
      uint64_t pythia;
    } pref_issue;

    struct {
      uint64_t low_conf;  // Prefetched to higher level
      uint64_t high_conf; // Prefetched to lower level
    } confidence;

    struct {
      uint64_t epochs;
      uint64_t histogram[DRAM_BW_LEVELS];
    } bandwidth;

    struct {
      uint64_t epochs;
      uint64_t histogram[PYTHIA_MAX_IPC_LEVEL];
    } ipc;

    struct {
      uint64_t epochs;
      uint64_t histogram[CACHE_ACC_LEVELS];
    } cache_acc;
  } stats;

  unordered_map<uint32_t, vector<uint64_t>> state_action_dist;
  unordered_map<std::string, vector<uint64_t>> state_action_dist2;
  unordered_map<int32_t, vector<uint64_t>> action_deg_dist;

private:
  void init_knobs();
  void init_stats();

  void update_global_state(uint64_t pc, uint64_t page, uint32_t offset, uint64_t address);
  Pythia_STEntry* update_local_state(uint64_t pc, uint64_t page, uint32_t offset, uint64_t address);
  uint32_t predict(uint64_t address, uint64_t page, uint32_t offset, State* state, vector<uint64_t>& pref_addr, vector<bool>& pref_level, vector<int>& offsets);
  bool track(uint64_t address, State* state, uint32_t action_index, float action_value, Pythia_PTEntry** tracker);
  void reward(uint64_t address);
  void reward(Pythia_PTEntry* ptentry);
  void assign_reward(Pythia_PTEntry* ptentry, RewardType type);
  int32_t compute_reward(Pythia_PTEntry* ptentry, RewardType type);
  void train(Pythia_PTEntry* curr_evicted, Pythia_PTEntry* last_evicted);
  vector<Pythia_PTEntry*> search_pt(uint64_t address, bool search_all, const deque<Pythia_PTEntry*>& pt);
  vector<Pythia_PTEntry*> search_all_pts(uint64_t address, bool search_all);
  void update_stats(uint32_t state, uint32_t action_index, uint32_t pref_degree = 1);
  void update_stats(State* state, uint32_t action_index, uint32_t degree = 1);
  void track_in_st(uint64_t page, uint32_t pred_offset, int32_t pref_offset);
  void gen_multi_degree_pref(uint64_t page, uint32_t offset, int32_t action, float action_value, uint32_t pref_degree, vector<uint64_t>& pref_addr,
                             vector<bool>& pref_level, vector<int>& offsets);
  uint32_t get_dyn_pref_degree(float max_to_avg_q_ratio, uint64_t page = 0xdeadbeef, int32_t action = 0); /* only implemented for CMAC engine 2.0 */
  bool is_high_bw();

public:
  Pythia(string type);
  ~Pythia();
  void invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, uint64_t instr_id, uint64_t curr_cycle, vector<uint64_t>& pref_addr,
                         vector<bool>& pref_level, vector<int>& offsets);
  void register_fill(uint64_t address, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint64_t curr_cycle);
  void register_llc_fill(uint64_t address);
  void register_prefetch_hit(uint64_t address);
  void dump_stats();
  void print_config();
  bool is_high_confidence(float action_value);
  int32_t getAction(uint32_t action_index);
  void update_bw(uint8_t bw_level);
  void update_ipc(uint8_t ipc);
  void update_acc(uint32_t acc_level);
};

#endif /* PYTHIA_H */