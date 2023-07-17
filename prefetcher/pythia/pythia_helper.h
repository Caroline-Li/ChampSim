#ifndef PYTHIA_HELPER_H
#define PYTHIA_HELPER_H

#include <deque>
#include <string>
#include <vector>

#include "bitmap.h"
#include <unordered_map>
#include <unordered_set>

using namespace std;

#define MAX_HOP_COUNT 16

typedef enum {
  PC = 0,
  Offset,
  Delta,
  PC_path,
  Offset_path,
  Delta_path,
  Address,
  Page,

  NumFeatures // Make sure to add the name to pythia_helper.cc MapFeatureString!
} Feature;

const char* getFeatureString(Feature feature);

typedef enum {
  none = 0,
  incorrect,
  correct_untimely,
  correct_timely,
  out_of_bounds,
  tracker_hit,
  incorrect_lowconf,
  correct_untimely_lowconf,
  correct_timely_lowconf,

  num_rewards // Make sure to add the name to pythia_helper.cc MapRewardTypeString!
} RewardType;

const char* getRewardTypeString(RewardType type);

inline bool isRewardCorrect(RewardType type) { return (type == correct_timely || type == correct_untimely); }
inline bool isRewardIncorrect(RewardType type) { return type == incorrect; }

class State
{
public:
  uint64_t pc;
  uint64_t address;
  uint64_t page;
  uint32_t offset;
  int32_t delta;
  uint32_t local_delta_sig;
  uint32_t local_delta_sig2;
  uint32_t local_pc_sig;
  uint32_t local_offset_sig;
  uint8_t bw_level;
  bool is_high_bw;
  uint32_t acc_level;

  /*
   * Add more states here
   */

  void reset()
  {
    pc = 0xdeadbeef;
    address = 0xdeadbeef;
    page = 0xdeadbeef;
    offset = 0;
    delta = 0;
    local_delta_sig = 0;
    local_delta_sig2 = 0;
    local_pc_sig = 0;
    local_offset_sig = 0;
    bw_level = 0;
    is_high_bw = false;
    acc_level = 0;
  }
  State() { reset(); }
  ~State() {}
  uint32_t value();                  /* apply as many state types as you want */
  uint32_t get_hash(uint64_t value); /* play wild with hashes */
  std::string to_string();
};

class ActionTracker
{
public:
  int32_t action;
  int32_t conf;
  ActionTracker(int32_t act, int32_t c) : action(act), conf(c) {}
  ~ActionTracker() {}
};

class Pythia_STEntry
{
public:
  uint64_t page;
  deque<uint64_t> pcs;
  deque<uint32_t> offsets;
  deque<int32_t> deltas;
  Bitmap bmp_real;
  Bitmap bmp_pred;
  unordered_set<uint64_t> unique_pcs;
  unordered_set<int32_t> unique_deltas;
  uint64_t trigger_pc;
  uint32_t trigger_offset;
  bool streaming;

  /* tracks last n actions on a page to determine degree */
  deque<ActionTracker*> action_tracker;
  unordered_set<int32_t> action_with_max_degree;
  unordered_set<int32_t> afterburning_actions;

  uint32_t total_prefetches;

public:
  Pythia_STEntry(uint64_t p, uint64_t pc, uint32_t offset) : page(p)
  {
    pcs.clear();
    offsets.clear();
    deltas.clear();
    bmp_real.reset();
    unique_pcs.clear();
    unique_deltas.clear();
    trigger_pc = pc;
    trigger_offset = offset;
    streaming = false;

    pcs.push_back(pc);
    offsets.push_back(offset);
    unique_pcs.insert(pc);
    bmp_real[offset] = 1;
  }
  ~Pythia_STEntry() {}
  uint32_t get_delta_sig();
  uint32_t get_delta_sig2();
  uint32_t get_pc_sig();
  uint32_t get_offset_sig();
  void update(uint64_t page, uint64_t pc, uint32_t offset, uint64_t address);
  void track_prefetch(uint32_t offset, int32_t pref_offset);
  void insert_action_tracker(int32_t pref_offset);
  bool search_action_tracker(int32_t action, int32_t& conf);
};

class Pythia_PTEntry
{
public:
  uint64_t address;
  State* state;
  uint32_t action_index;
  bool high_confidence;
  /* set when prefetched line is filled into cache
   * check during reward to measure timeliness */
  bool is_filled;
  /* set when prefetched line is alredy found in cache
   * donotes extreme untimely prefetch */
  bool pf_cache_hit;
  int32_t reward;
  RewardType reward_type;
  bool has_reward;
  vector<bool> consensus_vec; // only used in featurewise engine

  Pythia_PTEntry(uint64_t ad, State* st, uint32_t ac, bool hc) : address(ad), state(st), action_index(ac), high_confidence(hc)
  {
    is_filled = false;
    pf_cache_hit = false;
    reward = 0;
    reward_type = RewardType::none;
    has_reward = false;
  }
  ~Pythia_PTEntry() {}
};

/* some data structures to mine information from workloads */
class PythiaRecorder
{
public:
  unordered_set<uint64_t> unique_pcs;
  unordered_set<uint64_t> unique_trigger_pcs;
  unordered_set<uint64_t> unique_pages;
  unordered_map<uint64_t, uint64_t> access_bitmap_dist;
  // vector<unordered_map<int32_t, uint64_t>> hop_delta_dist;
  uint64_t hop_delta_dist[MAX_HOP_COUNT + 1][127];
  uint64_t total_bitmaps_seen;
  uint64_t unique_bitmaps_seen;
  unordered_map<uint64_t, vector<uint64_t>> pc_bw_dist;

  PythiaRecorder() {}
  ~PythiaRecorder() {}
  void record_access(uint64_t pc, uint64_t address, uint64_t page, uint32_t offset, uint8_t bw_level);
  void record_trigger_access(uint64_t page, uint64_t pc, uint32_t offset);
  void record_access_knowledge(Pythia_STEntry* stentry);
  void dump_stats();
};

/* auxiliary functions */
void print_access_debug(Pythia_STEntry* stentry);
string print_active_features(vector<int32_t> active_features);
string print_active_features2(vector<int32_t> active_features);
string print_weight_type(uint32_t weight_type);
string print_pooling_type(uint32_t pooling_type);

#endif /* PYTHIA_HELPER_H */