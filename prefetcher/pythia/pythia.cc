#include "pythia.h"

#include <algorithm>
#include <assert.h>
#include <iomanip>

#include "cache.h"
#include "champsim.h"
#include "champsim_constants.h"
#include "memory_class.h"
#include "util.h"

#if 0
#define LOCKED(...) \
  {                 \
    fflush(stdout); \
    __VA_ARGS__;    \
    fflush(stdout); \
  }
#define LOGID() fprintf(stdout, "[%25s@%3u] ", __FUNCTION__, __LINE__);
#define MYLOG(...) LOCKED(LOGID(); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n");)
#else
#define MYLOG(...) \
  {                \
  }
#endif

/* Action array
 * Basically a set of deltas to evaluate
 * Similar to the concept of BOP */
std::vector<int32_t> Actions;

namespace knob
{
extern float pythia_alpha;
extern float pythia_gamma;
extern float pythia_epsilon;
extern uint32_t pythia_state_num_bits;
extern uint32_t pythia_max_states;
extern uint32_t pythia_seed;
extern string pythia_policy;
extern string pythia_learning_type;
extern vector<int32_t> pythia_actions;
extern uint32_t pythia_max_actions;
extern uint32_t pythia_pt_size;
extern uint32_t pythia_st_size;
extern uint32_t pythia_max_pcs;
extern uint32_t pythia_max_offsets;
extern uint32_t pythia_max_deltas;
extern int32_t pythia_reward_none; // Default / low bandwidth rewards
extern int32_t pythia_reward_incorrect;
// extern int32_t  pythia_reward_incorrect_lowconf;
extern int32_t pythia_reward_correct_untimely;
// extern int32_t  pythia_reward_correct_untimely_lowconf;
extern int32_t pythia_reward_correct_timely;
// extern int32_t  pythia_reward_correct_timely_lowconf;
extern bool pythia_brain_zero_init;
extern bool pythia_enable_reward_all;
extern bool pythia_enable_track_multiple;
extern bool pythia_enable_reward_out_of_bounds;
extern int32_t pythia_reward_out_of_bounds;
extern uint32_t pythia_state_type;
extern bool pythia_access_debug;
extern bool pythia_print_access_debug;
extern uint64_t pythia_print_access_debug_pc;
extern uint32_t pythia_print_access_debug_pc_count;
extern bool pythia_print_trace;
extern bool pythia_enable_state_action_stats;
extern bool pythia_enable_reward_tracker_hit;
extern int32_t pythia_reward_tracker_hit;
extern uint32_t pythia_state_hash_type;
extern bool pythia_enable_featurewise_engine;
extern uint32_t pythia_pref_degree;
extern bool pythia_enable_dyn_degree;
extern vector<float> pythia_max_to_avg_q_thresholds;
extern vector<int32_t> pythia_dyn_degrees;
extern uint64_t pythia_early_exploration_window;
extern uint32_t pythia_pt_address_hash_type;
extern uint32_t pythia_pt_address_hashed_bits;
extern uint32_t pythia_bloom_filter_size;
extern uint32_t pythia_multi_deg_select_type;
extern vector<int32_t> pythia_last_pref_offset_conf_thresholds;
extern vector<int32_t> pythia_dyn_degrees_type2;
extern uint32_t pythia_action_tracker_size;
extern uint32_t pythia_high_bw_thresh;
extern bool pythia_enable_hbw_reward; // High bandwidth rewards
extern int32_t pythia_reward_hbw_correct_timely;
// extern int32_t  pythia_reward_hbw_correct_timely_lowconf;
extern int32_t pythia_reward_hbw_correct_untimely;
// extern int32_t  pythia_reward_hbw_correct_untimely_lowconf;
extern int32_t pythia_reward_hbw_incorrect;
// extern int32_t  pythia_reward_hbw_incorrect_lowconf;
extern int32_t pythia_reward_hbw_none;
extern int32_t pythia_reward_hbw_out_of_bounds;
extern int32_t pythia_reward_hbw_tracker_hit;
extern vector<int32_t> pythia_last_pref_offset_conf_thresholds_hbw;
extern vector<int32_t> pythia_dyn_degrees_type2_hbw;
extern bool pythia_enable_dyn_level; // Prefetch in L2 if high-confidence, LLC if low-confidence
extern float pythia_dyn_level_threshold;
extern bool pythia_separate_lowconf_pt; // If true, use a separate EQ for low-confidence prefetches.
extern uint32_t pythia_lowconf_pt_size;
extern bool pythia_enable_page_boundaries;

/* Learning Engine knobs */
extern bool le_enable_trace;
extern uint32_t le_trace_interval;
extern std::string le_trace_file_name;
extern uint32_t le_trace_state;
extern bool le_enable_score_plot;
extern std::vector<int32_t> le_plot_actions;
extern std::string le_plot_file_name;
extern bool le_enable_action_trace;
extern uint32_t le_action_trace_interval;
extern std::string le_action_trace_name;
extern bool le_enable_action_plot;

/* Featurewise Engine knobs */
extern vector<int32_t> le_featurewise_active_features;
extern vector<int32_t> le_featurewise_num_tilings;
extern vector<int32_t> le_featurewise_num_tiles;
extern vector<int32_t> le_featurewise_hash_types;
extern vector<int32_t> le_featurewise_enable_tiling_offset;
extern float le_featurewise_max_q_thresh;
extern bool le_featurewise_enable_action_fallback;
extern vector<float> le_featurewise_feature_weights;
extern bool le_featurewise_enable_dynamic_weight;
extern uint32_t le_featurewise_weight_type;
extern float le_featurewise_weight_gradient;
extern float le_featurewise_weight_minimum;
extern bool le_featurewise_disable_adjust_weight_all_features_align;
extern bool le_featurewise_enable_weight_normalization;
extern bool le_featurewise_enable_action_bias;
extern bool le_featurewise_selective_update;
extern uint32_t le_featurewise_pooling_type;
extern bool le_featurewise_enable_dyn_action_fallback;
extern uint32_t le_featurewise_bw_acc_check_level;
extern uint32_t le_featurewise_acc_thresh;
extern bool le_featurewise_enable_trace;
extern uint32_t le_featurewise_trace_feature_type;
extern string le_featurewise_trace_feature;
extern uint32_t le_featurewise_trace_interval;
extern uint32_t le_featurewise_trace_record_count;
extern std::string le_featurewise_trace_file_name;
extern bool le_featurewise_enable_score_plot;
extern vector<int32_t> le_featurewise_plot_actions;
extern std::string le_featurewise_plot_file_name;
extern bool le_featurewise_remove_plot_script;
} // namespace knob

void Pythia::init_knobs()
{
  Actions.resize(knob::pythia_max_actions, 0);
  std::copy(knob::pythia_actions.begin(), knob::pythia_actions.end(), Actions.begin());
  assert(Actions.size() == knob::pythia_max_actions);
  assert(Actions.size() <= MAX_ACTIONS);
  if (knob::pythia_access_debug) {
    cout << "***WARNING*** setting knob::pythia_max_pcs, knob::pythia_max_offsets, and knob::pythia_max_deltas to large value as knob::pythia_access_debug is "
            "true"
         << endl;
    knob::pythia_max_pcs = 1024;
    knob::pythia_max_offsets = 1024;
    knob::pythia_max_deltas = 1024;
  }
  cout << "pythia asserts!" << endl;
  assert(knob::pythia_pref_degree >= 0 && (knob::pythia_pref_degree == 0 || !knob::pythia_enable_dyn_degree));
  // assert(knob::pythia_max_to_avg_q_thresholds.size() == knob::pythia_dyn_degrees.size() - 1);
  // assert(knob::pythia_last_pref_offset_conf_thresholds.size() == knob::pythia_dyn_degrees_type2.size() - 1);
}

void Pythia::init_stats()
{
  bzero(&stats, sizeof(stats));
  stats.predict.action_dist.resize(knob::pythia_max_actions, 0);
  stats.predict.issue_dist.resize(knob::pythia_max_actions, 0);
  stats.predict.pred_hit.resize(knob::pythia_max_actions, 0);
  stats.predict.out_of_bounds_dist.resize(knob::pythia_max_actions, 0);
  state_action_dist.clear();
}

Pythia::Pythia(string type) : Prefetcher(type)
{
  init_knobs();
  init_stats();

  recorder = new PythiaRecorder();

  last_evicted_tracker = NULL;
  last_evicted_tracker_lowconf = NULL;

  /* init learning engine */
  brain_featurewise = NULL;
  brain = NULL;

  if (knob::pythia_enable_featurewise_engine) {
    brain_featurewise = new LearningEngineFeaturewise(this, knob::pythia_alpha, knob::pythia_gamma, knob::pythia_epsilon, knob::pythia_max_actions,
                                                      knob::pythia_seed, knob::pythia_policy, knob::pythia_learning_type, knob::pythia_brain_zero_init);
  } else {
    brain = new LearningEngineBasic(this, knob::pythia_alpha, knob::pythia_gamma, knob::pythia_epsilon, knob::pythia_max_actions, knob::pythia_max_states,
                                    knob::pythia_seed, knob::pythia_policy, knob::pythia_learning_type, knob::pythia_brain_zero_init,
                                    knob::pythia_early_exploration_window);
  }

  bw_level = 0;
  core_ipc = 0;
}

Pythia::~Pythia()
{
  if (brain_featurewise)
    delete brain_featurewise;
  if (brain)
    delete brain;
}

void Pythia::print_config()
{
  cout << "pythia_alpha " << knob::pythia_alpha << endl
       << "pythia_gamma " << knob::pythia_gamma << endl
       << "pythia_epsilon " << knob::pythia_epsilon << endl
       << "pythia_state_num_bits " << knob::pythia_state_num_bits << endl
       << "pythia_max_states " << knob::pythia_max_states << endl
       << "pythia_seed " << knob::pythia_seed << endl
       << "pythia_policy " << knob::pythia_policy << endl
       << "pythia_learning_type " << knob::pythia_learning_type << endl
       << "pythia_actions " << array_to_string(Actions) << endl
       << "pythia_max_actions " << knob::pythia_max_actions << endl
       << "pythia_pt_size " << knob::pythia_pt_size << endl
       << "pythia_st_size " << knob::pythia_st_size << endl
       << "pythia_max_pcs " << knob::pythia_max_pcs << endl
       << "pythia_max_offsets " << knob::pythia_max_offsets << endl
       << "pythia_max_deltas " << knob::pythia_max_deltas << endl
       << "pythia_reward_none " << knob::pythia_reward_none << endl
       << "pythia_reward_incorrect " << knob::pythia_reward_incorrect << endl
       << "pythia_reward_correct_untimely " << knob::pythia_reward_correct_untimely << endl
       << "pythia_reward_correct_timely " << knob::pythia_reward_correct_timely
       << endl
       //<< "pythia_reward_incorrect_lowconf " << knob::pythia_reward_incorrect_lowconf << endl
       //<< "pythia_reward_correct_untimely_lowconf " << knob::pythia_reward_correct_untimely_lowconf << endl
       //<< "pythia_reward_correct_timely_lowconf " << knob::pythia_reward_correct_timely_lowconf << endl
       << "pythia_brain_zero_init " << knob::pythia_brain_zero_init << endl
       << "pythia_enable_reward_all " << knob::pythia_enable_reward_all << endl
       << "pythia_enable_track_multiple " << knob::pythia_enable_track_multiple << endl
       << "pythia_enable_reward_out_of_bounds " << knob::pythia_enable_reward_out_of_bounds << endl
       << "pythia_reward_out_of_bounds " << knob::pythia_reward_out_of_bounds << endl
       << "pythia_state_type " << knob::pythia_state_type << endl
       << "pythia_state_hash_type " << knob::pythia_state_hash_type << endl
       << "pythia_access_debug " << knob::pythia_access_debug << endl
       << "pythia_print_access_debug " << knob::pythia_print_access_debug << endl
       << "pythia_print_access_debug_pc " << hex << knob::pythia_print_access_debug_pc << dec << endl
       << "pythia_print_access_debug_pc_count " << knob::pythia_print_access_debug_pc_count << endl
       << "pythia_print_trace " << knob::pythia_print_trace << endl
       << "pythia_enable_state_action_stats " << knob::pythia_enable_state_action_stats << endl
       << "pythia_enable_reward_tracker_hit " << knob::pythia_enable_reward_tracker_hit << endl
       << "pythia_reward_tracker_hit " << knob::pythia_reward_tracker_hit << endl
       << "pythia_enable_featurewise_engine " << knob::pythia_enable_featurewise_engine << endl
       << "pythia_pref_degree " << knob::pythia_pref_degree << endl
       << "pythia_enable_dyn_degree " << knob::pythia_enable_dyn_degree << endl
       << "pythia_max_to_avg_q_thresholds " << array_to_string(knob::pythia_max_to_avg_q_thresholds) << endl
       << "pythia_dyn_degrees " << array_to_string(knob::pythia_dyn_degrees) << endl
       << "pythia_multi_deg_select_type " << knob::pythia_multi_deg_select_type << endl
       << "pythia_last_pref_offset_conf_thresholds " << array_to_string(knob::pythia_last_pref_offset_conf_thresholds) << endl
       << "pythia_dyn_degrees_type2 " << array_to_string(knob::pythia_dyn_degrees_type2) << endl
       << "pythia_action_tracker_size " << knob::pythia_action_tracker_size << endl
       << "pythia_high_bw_thresh " << knob::pythia_high_bw_thresh << endl
       << "pythia_enable_hbw_reward " << knob::pythia_enable_hbw_reward << endl
       << "pythia_reward_hbw_correct_timely " << knob::pythia_reward_hbw_correct_timely << endl
       << "pythia_reward_hbw_correct_untimely " << knob::pythia_reward_hbw_correct_untimely << endl
       << "pythia_reward_hbw_incorrect " << knob::pythia_reward_hbw_incorrect
       << endl
       //<< "pythia_reward_hbw_correct_timely_lowconf " << knob::pythia_reward_hbw_correct_timely_lowconf << endl
       //<< "pythia_reward_hbw_correct_untimely_lowconf " << knob::pythia_reward_hbw_correct_untimely_lowconf << endl
       //<< "pythia_reward_hbw_incorrect_lowconf " << knob::pythia_reward_hbw_incorrect_lowconf << endl
       << "pythia_reward_hbw_none " << knob::pythia_reward_hbw_none << endl
       << "pythia_reward_hbw_out_of_bounds " << knob::pythia_reward_hbw_out_of_bounds << endl
       << "pythia_reward_hbw_tracker_hit " << knob::pythia_reward_hbw_tracker_hit << endl
       << "pythia_last_pref_offset_conf_thresholds_hbw " << array_to_string(knob::pythia_last_pref_offset_conf_thresholds_hbw) << endl
       << "pythia_dyn_degrees_type2_hbw " << array_to_string(knob::pythia_dyn_degrees_type2_hbw) << endl
       << "pythia_enable_dyn_level " << knob::pythia_enable_dyn_level << endl
       << "pythia_dyn_level_threshold " << knob::pythia_dyn_level_threshold << endl
       << "pythia_separate_lowconf_pt " << knob::pythia_separate_lowconf_pt << endl
       << "pythia_lowconf_pt_size " << knob::pythia_lowconf_pt_size << endl
       << endl
       << "le_enable_trace " << knob::le_enable_trace << endl
       << "le_trace_interval " << knob::le_trace_interval << endl
       << "le_trace_file_name " << knob::le_trace_file_name << endl
       << "le_trace_state " << hex << knob::le_trace_state << dec << endl
       << "le_enable_score_plot " << knob::le_enable_score_plot << endl
       << "le_plot_file_name " << knob::le_plot_file_name << endl
       << "le_plot_actions " << array_to_string(knob::le_plot_actions) << endl
       << "le_enable_action_trace " << knob::le_enable_action_trace << endl
       << "le_action_trace_interval " << knob::le_action_trace_interval << endl
       << "le_action_trace_name " << knob::le_action_trace_name << endl
       << "le_enable_action_plot " << knob::le_enable_action_plot << endl
       << endl
       << "le_featurewise_active_features " << print_active_features2(knob::le_featurewise_active_features) << endl
       << "le_featurewise_num_tilings " << array_to_string(knob::le_featurewise_num_tilings) << endl
       << "le_featurewise_num_tiles " << array_to_string(knob::le_featurewise_num_tiles) << endl
       << "le_featurewise_hash_types " << array_to_string(knob::le_featurewise_hash_types) << endl
       << "le_featurewise_enable_tiling_offset " << array_to_string(knob::le_featurewise_enable_tiling_offset) << endl
       << "le_featurewise_max_q_thresh " << knob::le_featurewise_max_q_thresh << endl
       << "le_featurewise_enable_action_fallback " << knob::le_featurewise_enable_action_fallback << endl
       << "le_featurewise_feature_weights " << array_to_string(knob::le_featurewise_feature_weights) << endl
       << "le_featurewise_enable_dynamic_weight " << knob::le_featurewise_enable_dynamic_weight << endl
       << "le_featurewise_weight_type " << print_weight_type(knob::le_featurewise_weight_type) << endl
       << "le_featurewise_weight_gradient " << knob::le_featurewise_weight_gradient << endl
       << "le_featurewise_weight_minimum " << knob::le_featurewise_weight_minimum << endl
       << "le_featurewise_disable_adjust_weight_all_features_align " << knob::le_featurewise_disable_adjust_weight_all_features_align << endl
       << "le_featurewise_enable_weight_normalization " << knob::le_featurewise_enable_weight_normalization << endl
       << "le_featurewise_enable_action_bias " << knob::le_featurewise_enable_action_bias << endl
       << "le_featurewise_selective_update " << knob::le_featurewise_selective_update << endl
       << "le_featurewise_pooling_type " << print_pooling_type(knob::le_featurewise_pooling_type) << endl
       << "le_featurewise_enable_dyn_action_fallback " << knob::le_featurewise_enable_dyn_action_fallback << endl
       << "le_featurewise_bw_acc_check_level " << knob::le_featurewise_bw_acc_check_level << endl
       << "le_featurewise_acc_thresh " << knob::le_featurewise_acc_thresh << endl
       << "le_featurewise_enable_trace " << knob::le_featurewise_enable_trace << endl
       << "le_featurewise_trace_feature_type " << knob::le_featurewise_trace_feature_type << endl
       << "le_featurewise_trace_feature " << knob::le_featurewise_trace_feature << endl
       << "le_featurewise_trace_interval " << knob::le_featurewise_trace_interval << endl
       << "le_featurewise_trace_record_count " << knob::le_featurewise_trace_record_count << endl
       << "le_featurewise_trace_file_name " << knob::le_featurewise_trace_file_name << endl
       << "le_featurewise_enable_score_plot " << knob::le_featurewise_enable_score_plot << endl
       << "le_featurewise_plot_actions " << array_to_string(knob::le_featurewise_plot_actions) << endl
       << "le_featurewise_plot_file_name " << knob::le_featurewise_plot_file_name << endl
       << endl;
}

void Pythia::invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, uint64_t instr_id, uint64_t curr_cycle,
                               vector<uint64_t>& pref_addr, vector<bool>& pref_level, vector<int>& offsets)
{
  uint64_t page = address >> LOG2_PAGE_SIZE;
  uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

  MYLOG("---------------------------------------------------------------------");
  MYLOG("%s %lx pc %lx page %lx off %u", GetAccessType(type), address, pc, page, offset);

  /* compute reward on demand */
  reward(address);

  /* record the access: just to gain some insights from the workload
   * defined in pythia_helper.h(cc) */
  recorder->record_access(pc, address, page, offset, bw_level);

  /* global state tracking */
  update_global_state(pc, page, offset, address);
  /* per page state tracking */
  Pythia_STEntry* stentry = update_local_state(pc, page, offset, address);

  /* Measure state.
   * state can contain per page local information like delta signature, pc signature etc.
   * it can also contain global signatures like last three branch PCs etc.
   */
  State* state = new State();
  state->pc = pc;
  state->address = address;
  state->page = page;
  state->offset = offset;
  state->delta = !stentry->deltas.empty() ? stentry->deltas.back() : 0;
  state->local_delta_sig = stentry->get_delta_sig();
  state->local_delta_sig2 = stentry->get_delta_sig2();
  state->local_pc_sig = stentry->get_pc_sig();
  state->local_offset_sig = stentry->get_offset_sig();
  state->bw_level = bw_level;
  state->is_high_bw = is_high_bw();
  state->acc_level = acc_level;

  uint32_t count = pref_addr.size();
  predict(address, page, offset, state, pref_addr, pref_level, offsets);
  stats.pref_issue.pythia += (pref_addr.size() - count);
}

void Pythia::update_global_state(uint64_t pc, uint64_t page, uint32_t offset, uint64_t address)
{ /* @rbera TODO: implement */
}

Pythia_STEntry* Pythia::update_local_state(uint64_t pc, uint64_t page, uint32_t offset, uint64_t address)
{
  stats.st.lookup++;
  Pythia_STEntry* stentry = NULL;
  auto st_index = find_if(signature_table.begin(), signature_table.end(), [page](Pythia_STEntry* stentry) { return stentry->page == page; });
  if (st_index != signature_table.end()) {
    stats.st.hit++;
    stentry = (*st_index);
    stentry->update(page, pc, offset, address);
    signature_table.erase(st_index);
    signature_table.push_back(stentry);
    return stentry;
  } else {
    if (signature_table.size() >= knob::pythia_st_size) {
      stats.st.evict++;
      stentry = signature_table.front();
      signature_table.pop_front();
      if (knob::pythia_access_debug) {
        recorder->record_access_knowledge(stentry);
        if (knob::pythia_print_access_debug) {
          print_access_debug(stentry);
        }
      }
      delete stentry;
    }

    stats.st.insert++;
    stentry = new Pythia_STEntry(page, pc, offset);
    recorder->record_trigger_access(page, pc, offset);
    signature_table.push_back(stentry);
    return stentry;
  }
}

bool Pythia::is_high_confidence(float action_value)
{
  /* Return true if <action_value> is large enough to be a high_confidence action,
   * Return false otherwise */
  if (!knob::pythia_enable_dyn_level)
    return true;
  return (action_value >= knob::pythia_dyn_level_threshold);
}

uint32_t Pythia::predict(uint64_t base_address, uint64_t page, uint32_t offset, State* state, vector<uint64_t>& pref_addr, vector<bool>& pref_level, vector<int>& offsets)
{
  MYLOG("addr@%lx page %lx off %u state %x", base_address, page, offset, state->value());

  stats.predict.called++;

  /* query learning engine to get the next prediction */
  uint32_t action_index = 0;
  float action_value = 0.0;
  uint32_t pref_degree = knob::pythia_pref_degree;
  vector<bool> consensus_vec; // only required for featurewise engine

  if (knob::pythia_enable_featurewise_engine) {
    float max_to_avg_q_ratio = 1.0;
    action_index = brain_featurewise->chooseAction(state, action_value, max_to_avg_q_ratio, consensus_vec);
    if (knob::pythia_enable_dyn_degree) {
      pref_degree = get_dyn_pref_degree(max_to_avg_q_ratio, page, Actions[action_index]);
    }
    if (knob::pythia_enable_state_action_stats) {
      update_stats(state, action_index, pref_degree);
    }
  } else {
    uint32_t state_index = state->value();
    assert(state_index < knob::pythia_max_states);
    action_index = brain->chooseAction(state_index, action_value);
    if (knob::pythia_enable_state_action_stats) {
      update_stats(state_index, action_index, pref_degree);
    }
  }
  assert(action_index < knob::pythia_max_actions);

  MYLOG("act_idx %u act %d", action_index, Actions[action_index]);

  uint64_t addr = 0xdeadbeef;
  Pythia_PTEntry* ptentry = NULL;
  int32_t predicted_offset = 0;
  if (Actions[action_index] != 0) {
    predicted_offset = (int32_t)offset + Actions[action_index];
    if ((predicted_offset >= 0 && predicted_offset < 64) || !knob::pythia_enable_page_boundaries) /* falls within the page */
    {
      addr = (page << LOG2_PAGE_SIZE) + (predicted_offset << LOG2_BLOCK_SIZE);
      MYLOG("pred_off %d pred_addr %lx", predicted_offset, addr);
      /* track prefetch */
      bool new_addr = track(addr, state, action_index, action_value, &ptentry);
      if (new_addr) {
        // Prefetch address
        pref_addr.push_back(addr);

        // Prefetch level (0 = default, if knob disabled, otherwise  or default (high confidence) or LLC (low confidence.)
        // Always high confidence, if pythia_enable_dyn_level is false.
        pref_level.push_back(is_high_confidence(action_value));

        offsets.push_back(Actions[action_index]);

        if (is_high_confidence(action_value))
          stats.confidence.high_conf++;
        else
          stats.confidence.low_conf++;

        track_in_st(page, predicted_offset, Actions[action_index]);
        stats.predict.issue_dist[action_index]++;
        if (pref_degree > 1) {
          gen_multi_degree_pref(page, offset, Actions[action_index], action_value, pref_degree, pref_addr, pref_level, offsets);
        }
        stats.predict.deg_histogram[pref_degree]++;
        ptentry->consensus_vec = consensus_vec;
      } else {
        MYLOG("pred_off %d tracker_hit", predicted_offset);
        stats.predict.pred_hit[action_index]++;
        if (knob::pythia_enable_reward_tracker_hit) {
          addr = 0xdeadbeef;
          track(addr, state, action_index, action_value, &ptentry);
          assert(ptentry);
          assign_reward(ptentry, RewardType::tracker_hit);
          ptentry->consensus_vec = consensus_vec;
        }
      }
      stats.predict.action_dist[action_index]++;
    } else {
      MYLOG("pred_off %d out_of_bounds", predicted_offset);
      stats.predict.out_of_bounds++;
      stats.predict.out_of_bounds_dist[action_index]++;
      if (knob::pythia_enable_reward_out_of_bounds) {
        addr = 0xdeadbeef;
        track(addr, state, action_index, action_value, &ptentry);
        assert(ptentry);
        assign_reward(ptentry, RewardType::out_of_bounds);
        ptentry->consensus_vec = consensus_vec;
      }
    }
  } else {
    MYLOG("no prefecth");
    /* agent decided not to prefetch */
    addr = 0xdeadbeef;
    /* track no prefetch */
    track(addr, state, action_index, action_value, &ptentry);
    stats.predict.action_dist[action_index]++;
    ptentry->consensus_vec = consensus_vec;
  }

  stats.predict.predicted += pref_addr.size();
  MYLOG("end@%lx", base_address);

  return pref_addr.size();
}

/* Returns true if the address is not already present in prefetch_tracker
 * false otherwise */
bool Pythia::track(uint64_t address, State* state, uint32_t action_index, float action_value, Pythia_PTEntry** tracker)
{
  MYLOG("addr@%lx state %x act_idx %u act %d confidence %d", address, state->value(), action_index, Actions[action_index], is_high_confidence(action_value));
  stats.track.called++;

  deque<Pythia_PTEntry*>* pt = NULL;
  Pythia_PTEntry** let = NULL;
  uint32_t pt_size = 0;
  if (knob::pythia_separate_lowconf_pt && !is_high_confidence(action_value)) {
    pt = &prefetch_tracker_lowconf;
    let = &last_evicted_tracker_lowconf;
    pt_size = knob::pythia_lowconf_pt_size;
  } else {
    pt = &prefetch_tracker;
    let = &last_evicted_tracker;
    pt_size = knob::pythia_pt_size;
  }

  vector<Pythia_PTEntry*> ptentries = search_pt(address, false, *pt);
  bool new_addr = ptentries.empty() ? true : false;

  if (!new_addr && address != 0xdeadbeef && !knob::pythia_enable_track_multiple) {
    stats.track.same_address++;
    tracker = NULL;
    return new_addr;
  }

  /* new prefetched address that hasn't been seen before */
  Pythia_PTEntry* ptentry = NULL;
  if (pt->size() >= pt_size) {
    stats.track.evict++;
    ptentry = pt->front();
    pt->pop_front();
    // if((ptentry->address >> LOG2_BLOCK_SIZE) == 0x50c0cd6) {
    //   std::cout << "[DEBUG] evicted 0x50c0cd6 last_evicted 0x" << std::hex << ((*let)->address >> LOG2_BLOCK_SIZE) << std::endl;
    // }
    MYLOG("victim_state %x victim_act_idx %u victim_act %d confidence %d", ptentry->state->value(), ptentry->action_index, Actions[ptentry->action_index],
          ptentry->high_confidence);
    if (*let) {
      MYLOG("last_victim_state %x last_victim_act_idx %u last_victim_act %d confidence %d", (*let)->state->value(), (*let)->action_index,
            Actions[(*let)->action_index], (*let)->high_confidence);
      /* train the agent */
      train(ptentry, *let);

      delete (*let)->state;
      delete *let;
    }

    *let = ptentry;
  }

  ptentry = new Pythia_PTEntry(address, state, action_index, is_high_confidence(action_index));
  pt->push_back(ptentry);
  assert(pt->size() <= pt_size);

  (*tracker) = ptentry;
  MYLOG("end@%lx", address);

  return new_addr;
}

void Pythia::gen_multi_degree_pref(uint64_t page, uint32_t offset, int32_t action, float action_value, uint32_t pref_degree, vector<uint64_t>& pref_addr,
                                   vector<bool>& pref_level, vector<int>& offsets)
{
  stats.predict.multi_deg_called++;
  uint64_t addr = 0xdeadbeef;
  int32_t predicted_offset = 0;

  if (action != 0) {
    for (uint32_t degree = 2; degree <= pref_degree; ++degree) {
      predicted_offset = (int32_t)offset + degree * action;
      if (predicted_offset >= 0 && predicted_offset < 64) {
        addr = (page << LOG2_PAGE_SIZE) + (predicted_offset << LOG2_BLOCK_SIZE);
        pref_addr.push_back(addr);

        // Prefetch level (0 = default, if knob disabled, otherwise  or default (high confidence) or LLC (low confidence.)
        // Always high confidence, if pythia_enable_dyn_level is false.
        pref_level.push_back(is_high_confidence(action_value));

        offsets.push_back(action);

        if (is_high_confidence(action_value))
          stats.confidence.high_conf++;
        else
          stats.confidence.low_conf++;

        MYLOG("degree %u pred_off %d pred_addr %lx confidence %d", degree, predicted_offset, addr, is_high_confidence(action_value));
        stats.predict.multi_deg++;
        stats.predict.multi_deg_histogram[degree]++;
      }
    }
  }
}

uint32_t Pythia::get_dyn_pref_degree(float max_to_avg_q_ratio, uint64_t page, int32_t action)
{
  uint32_t counted = false;
  uint32_t degree = 1;

  if (knob::pythia_multi_deg_select_type == 2) {
    auto st_index = find_if(signature_table.begin(), signature_table.end(), [page](Pythia_STEntry* stentry) { return stentry->page == page; });
    if (st_index != signature_table.end()) {
      int32_t conf = 0;
      bool found = (*st_index)->search_action_tracker(action, conf);
      vector<int32_t> conf_thresholds, deg_afterburning, deg_normal;

      conf_thresholds = is_high_bw() ? knob::pythia_last_pref_offset_conf_thresholds_hbw : knob::pythia_last_pref_offset_conf_thresholds;
      deg_normal = is_high_bw() ? knob::pythia_dyn_degrees_type2_hbw : knob::pythia_dyn_degrees_type2;

      if (found) {
        for (uint32_t index = 0; index < conf_thresholds.size(); ++index) {
          /* pythia_last_pref_offset_conf_thresholds is a sorted list in ascending order of values */
          if (conf <= conf_thresholds[index]) {
            degree = deg_normal[index];
            counted = true;
            break;
          }
        }
        if (!counted) {
          degree = deg_normal.back();
        }
      } else {
        degree = 1;
      }
    }
  }
  return degree;
}

/* This reward fucntion is called after seeing a demand access to the address */
/* TODO: what if multiple prefetch request generated the same address?
 * Currently, it just rewards the oldest prefetch request to the address
 * Should we reward all? */
void Pythia::reward(uint64_t address)
{
  MYLOG("addr @ %lx", address);

  stats.reward.demand.called++;
  vector<Pythia_PTEntry*> ptentries = search_all_pts(address, knob::pythia_enable_reward_all);

  if (ptentries.empty()) {
    MYLOG("PT miss");
    stats.reward.demand.pt_not_found++;
    return;
  } else {
    stats.reward.demand.pt_found++;
  }

  for (uint32_t index = 0; index < ptentries.size(); ++index) {
    Pythia_PTEntry* ptentry = ptentries[index];
    stats.reward.demand.pt_found_total++;

    MYLOG("PT hit. state %x act_idx %u act %d", ptentry->state->value(), ptentry->action_index, Actions[ptentry->action_index]);
    /* Do not compute reward if already has a reward.
     * This can happen when a prefetch access sees multiple demand reuse */
    if (ptentry->has_reward) {
      MYLOG("entry already has reward: %d", ptentry->reward);
      stats.reward.demand.has_reward++;
      return;
    }

    if (ptentry->is_filled) /* timely */
    {
      assign_reward(ptentry, RewardType::correct_timely);
      MYLOG("assigned reward correct_timely(%d)", ptentry->reward);
    } else {
      assign_reward(ptentry, RewardType::correct_untimely);
      MYLOG("assigned reward correct_untimely(%d)", ptentry->reward);
    }
    ptentry->has_reward = true;
  }
}

/* This reward function is called during eviction from prefetch_tracker */
void Pythia::reward(Pythia_PTEntry* ptentry)
{
  MYLOG("reward PT evict %lx state %x act_idx %u act %d confidence %d", ptentry->address, ptentry->state->value(), ptentry->action_index,
        Actions[ptentry->action_index], ptentry->high_confidence);

  stats.reward.train.called++;
  assert(!ptentry->has_reward);
  /* this is called during eviction from prefetch tracker
   * that means, this address doesn't see a demand reuse.
   * hence it either can be incorrect, or no prefetch */
  if (ptentry->address == 0xdeadbeef) /* no prefetch */
  {
    assign_reward(ptentry, RewardType::none);
    MYLOG("assigned reward no_pref(%d)", ptentry->reward);
  } else /* incorrect prefetch */
  {
    assign_reward(ptentry, RewardType::incorrect);
    MYLOG("assigned reward incorrect(%d)", ptentry->reward);
  }
  ptentry->has_reward = true;
}

void Pythia::assign_reward(Pythia_PTEntry* ptentry, RewardType type)
{
  MYLOG("assign_reward PT evict %lx state %x act_idx %u act %d confidence %d", ptentry->address, ptentry->state->value(), ptentry->action_index,
        Actions[ptentry->action_index], ptentry->high_confidence);
  assert(!ptentry->has_reward);

  /* compute the reward */
  int32_t reward = compute_reward(ptentry, type);

  /* assign */
  ptentry->reward = reward;
  ptentry->reward_type = type;
  ptentry->has_reward = true;

  /* maintain stats */
  stats.reward.assign_reward.called++;
  switch (type) {
  case RewardType::correct_timely:
    stats.reward.correct_timely++;
    break;
  case RewardType::correct_untimely:
    stats.reward.correct_untimely++;
    break;
  case RewardType::incorrect:
    stats.reward.incorrect++;
    break;
  case RewardType::none:
    stats.reward.no_pref++;
    break;
  case RewardType::out_of_bounds:
    stats.reward.out_of_bounds++;
    break;
  case RewardType::tracker_hit:
    stats.reward.tracker_hit++;
    break;
  case RewardType::correct_timely_lowconf:
    stats.reward.correct_timely_lowconf++;
    break;
  case RewardType::correct_untimely_lowconf:
    stats.reward.correct_untimely_lowconf++;
    break;
  case RewardType::incorrect_lowconf:
    stats.reward.incorrect_lowconf++;
    break;
  default:
    assert(false);
  }
  stats.reward.dist[ptentry->action_index][type]++;
}

int32_t Pythia::compute_reward(Pythia_PTEntry* ptentry, RewardType type)
{
  bool high_bw = (knob::pythia_enable_hbw_reward && is_high_bw()) ? true : false;
  int32_t reward = 0;

  stats.reward.compute_reward.dist[type][high_bw]++;

  if (type == RewardType::correct_timely) {
    reward = high_bw ? knob::pythia_reward_hbw_correct_timely : knob::pythia_reward_correct_timely;
  } else if (type == RewardType::correct_untimely) {
    reward = high_bw ? knob::pythia_reward_hbw_correct_untimely : knob::pythia_reward_correct_untimely;
  } else if (type == RewardType::incorrect) {
    reward = high_bw ? knob::pythia_reward_hbw_incorrect : knob::pythia_reward_incorrect;
  } else if (type == RewardType::none) {
    reward = high_bw ? knob::pythia_reward_hbw_none : knob::pythia_reward_none;
  } else if (type == RewardType::out_of_bounds) {
    reward = high_bw ? knob::pythia_reward_hbw_out_of_bounds : knob::pythia_reward_out_of_bounds;
  } else if (type == RewardType::tracker_hit) {
    reward = high_bw ? knob::pythia_reward_hbw_tracker_hit : knob::pythia_reward_tracker_hit;
  } else if (type == RewardType::correct_timely_lowconf) {
    // reward = high_bw ? knob::pythia_reward_hbw_correct_timely_lowconf : knob::pythia_reward_correct_timely_lowconf;
  } else if (type == RewardType::correct_untimely_lowconf) {
    // reward = high_bw ? knob::pythia_reward_hbw_correct_untimely_lowconf : knob::pythia_reward_correct_untimely_lowconf;
  } else if (type == RewardType::incorrect_lowconf) {
    // reward = high_bw ? knob::pythia_reward_hbw_incorrect_lowconf : knob::pythia_reward_incorrect_lowconf;
  } else {
    cout << "Invalid reward type found " << type << endl;
    assert(false);
  }

  return reward;
}

void Pythia::train(Pythia_PTEntry* curr_evicted, Pythia_PTEntry* last_evicted)
{
  MYLOG("victim %s %u %d last_victim %s %u %d", curr_evicted->state->to_string().c_str(), curr_evicted->action_index, Actions[curr_evicted->action_index],
        last_evicted->state->to_string().c_str(), last_evicted->action_index, Actions[last_evicted->action_index]);

  // if((last_evicted->address >> LOG2_BLOCK_SIZE) == 0x50c0cd6 || (curr_evicted->address >> LOG2_BLOCK_SIZE) == 0x50c0dcd6) {
  //  std::cout << "[DEBUG] training last_evicted=0x" << std::hex << (last_evicted->address >> LOG2_BLOCK_SIZE)
  //            << " curr_evicted=0x" << std::hex << (curr_evicted->address >> LOG2_BLOCK_SIZE) << std::endl;
  //}

  stats.train.called++;
  if (!last_evicted->has_reward) {
    stats.train.compute_reward++;
    reward(last_evicted);
  }
  assert(last_evicted->has_reward);

  /* train */
  MYLOG("===SARSA=== S1: %s A1: %u R1: %d S2: %s A2: %u", last_evicted->state->to_string().c_str(), last_evicted->action_index, last_evicted->reward,
        curr_evicted->state->to_string().c_str(), curr_evicted->action_index);
  if (knob::pythia_enable_featurewise_engine) {
    brain_featurewise->learn(last_evicted->state, last_evicted->action_index, last_evicted->reward, curr_evicted->state, curr_evicted->action_index,
                             last_evicted->consensus_vec, last_evicted->reward_type);
  } else {
    brain->learn(last_evicted->state->value(), last_evicted->action_index, last_evicted->reward, curr_evicted->state->value(), curr_evicted->action_index);
  }
  MYLOG("train done");
}

/* TODO: what if multiple prefetch request generated the same address?
 * Currently it just sets the fill bit of the oldest prefetch request.
 * Do we need to set it for everyone? */
void Pythia::register_fill(uint64_t address, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint64_t curr_cycle)
{
  MYLOG("fill @ %lx", address);

  stats.register_fill.called++;
  vector<Pythia_PTEntry*> ptentries = search_all_pts(address, knob::pythia_enable_reward_all);
  if (!ptentries.empty()) {
    stats.register_fill.set++;
    for (uint32_t index = 0; index < ptentries.size(); ++index) {
      stats.register_fill.set_total++;
      stats.register_fill.set_actions[ptentries[index]->action_index]++;
      stats.register_fill.set_conf[ptentries[index]->high_confidence]++;
      ptentries[index]->is_filled = true;
      MYLOG("fill PT hit. pref with act_idx %u act %d confidence %d", ptentries[index]->action_index, Actions[ptentries[index]->action_index],
            ptentries[index]->high_confidence, );
    }
  }
}

void Pythia::register_llc_fill(uint64_t address)
{
  if (!knob::pythia_enable_dyn_level) // Don't care about LLC fills, if we only prefetch to L2.
    return;

  MYLOG("LLC fill @ %lx", address);

  stats.register_llc_fill.called++;
  vector<Pythia_PTEntry*> ptentries = search_all_pts(address, knob::pythia_enable_reward_all);

  if (!ptentries.empty()) {
    // cout << "[DEBUG] [register_llc_fill] found in EQ: " << hex << address << endl;
    stats.register_llc_fill.set++;
    for (uint32_t index = 0; index < ptentries.size(); ++index) {
      // if (ptentries[index]->high_confidence) {
      //    cout << "[DEBUG] [register_llc_fill] HIGH CONFIDENCE ptentry found: addr=" << hex << address
      //         << " action=" << dec << Actions[ptentries[index]->action_index]
      //         << " action_index=" << dec << ptentries[index]->action_index
      //         << " is_filled=" << dec << ptentries[index]->is_filled
      //         << " pf_cache_hit=" << dec << ptentries[index]->pf_cache_hit
      //         << " reward=" << dec << ptentries[index]->reward
      //         << endl;
      // } else {
      //    cout << "[DEBUG] [register_llc_fill] Low confidence ptentry found: addr=" << hex << address
      //         << " action=" << dec << Actions[ptentries[index]->action_index]
      //         << " action_index=" << dec << ptentries[index]->action_index
      //         << " is_filled=" << dec << ptentries[index]->is_filled
      //         << " pf_cache_hit=" << dec << ptentries[index]->pf_cache_hit
      //         << " reward=" << dec << ptentries[index]->reward
      //         << endl;
      // }

      stats.register_llc_fill.set_total++;
      stats.register_llc_fill.set_actions[ptentries[index]->action_index]++;
      stats.register_llc_fill.set_conf[ptentries[index]->high_confidence]++;
      ptentries[index]->is_filled = true;
      MYLOG("fill PT hit. LLC pref with act_idx %u act %d confidence %d", ptentries[index]->action_index, Actions[ptentries[index]->action_index],
            ptentries[index]->high_confidence, );
    }
  } // else {
  //    cout << "[DEBUG] [register_llc_fill] NOT found in EQ: " << hex << address << endl;
  //}
}

void Pythia::register_prefetch_hit(uint64_t address)
{
  MYLOG("pref_hit @ %lx", address);

  stats.register_prefetch_hit.called++;
  vector<Pythia_PTEntry*> ptentries = search_all_pts(address, knob::pythia_enable_reward_all);
  if (!ptentries.empty()) {
    stats.register_prefetch_hit.set++;
    for (uint32_t index = 0; index < ptentries.size(); ++index) {
      stats.register_prefetch_hit.set_total++;
      ptentries[index]->pf_cache_hit = true;
      MYLOG("pref_hit PT hit. pref with act_idx %u act %d confidence %d", ptentries[index]->action_index, Actions[ptentries[index]->action_index],
            ptentries[index]->high_confidence, );
    }
  }
}

vector<Pythia_PTEntry*> Pythia::search_pt(uint64_t address, bool search_all, const deque<Pythia_PTEntry*>& pt)
{
  vector<Pythia_PTEntry*> entries;
  for (uint32_t index = 0; index < pt.size(); ++index) {
    if (pt[index]->address == address) {
      entries.push_back(pt[index]);
      if (!search_all)
        break;
    }
  }
  return entries;
}

vector<Pythia_PTEntry*> Pythia::search_all_pts(uint64_t address, bool search_all)
{
  vector<Pythia_PTEntry*> entries = search_pt(address, search_all, prefetch_tracker);

  if (knob::pythia_separate_lowconf_pt) {
    vector<Pythia_PTEntry*> entries_lowconf = search_pt(address, search_all, prefetch_tracker_lowconf);
    entries.insert(entries.end(), entries_lowconf.begin(), entries_lowconf.end());
  }
  return entries;
}

void Pythia::update_stats(uint32_t state, uint32_t action_index, uint32_t pref_degree)
{
  auto it = state_action_dist.find(state);
  if (it != state_action_dist.end()) {
    it->second[action_index]++;
  } else {
    vector<uint64_t> act_dist;
    act_dist.resize(knob::pythia_max_actions, 0);
    act_dist[action_index]++;
    state_action_dist.insert(std::pair<uint32_t, vector<uint64_t>>(state, act_dist));
  }
}

void Pythia::update_stats(State* state, uint32_t action_index, uint32_t degree)
{
  string state_str = state->to_string();
  auto it = state_action_dist2.find(state_str);
  if (it != state_action_dist2.end()) {
    it->second[action_index]++;
    it->second[knob::pythia_max_actions]++; /* counts total occurences of this state */
  } else {
    vector<uint64_t> act_dist;
    act_dist.resize(knob::pythia_max_actions + 1, 0);
    act_dist[action_index]++;
    act_dist[knob::pythia_max_actions]++; /* counts total occurences of this state */
    state_action_dist2.insert(std::pair<string, vector<uint64_t>>(state_str, act_dist));
  }

  auto it2 = action_deg_dist.find(getAction(action_index));
  if (it2 != action_deg_dist.end()) {
    it2->second[degree]++;
  } else {
    vector<uint64_t> deg_dist;
    deg_dist.resize(MAX_PYTHIA_DEGREE, 0);
    deg_dist[degree]++;
    action_deg_dist.insert(std::pair<int32_t, vector<uint64_t>>(getAction(action_index), deg_dist));
  }
}

int32_t Pythia::getAction(uint32_t action_index)
{
  assert(action_index < Actions.size());
  return Actions[action_index];
}

void Pythia::track_in_st(uint64_t page, uint32_t pred_offset, int32_t pref_offset)
{
  auto st_index = find_if(signature_table.begin(), signature_table.end(), [page](Pythia_STEntry* stentry) { return stentry->page == page; });
  if (st_index != signature_table.end()) {
    (*st_index)->track_prefetch(pred_offset, pref_offset);
  }
}

void Pythia::update_bw(uint8_t bw)
{
  assert(bw < DRAM_BW_LEVELS);
  bw_level = bw;
  stats.bandwidth.epochs++;
  stats.bandwidth.histogram[bw_level]++;
}

void Pythia::update_ipc(uint8_t ipc)
{
  assert(ipc < PYTHIA_MAX_IPC_LEVEL);
  core_ipc = ipc;
  stats.ipc.epochs++;
  stats.ipc.histogram[ipc]++;
}

void Pythia::update_acc(uint32_t acc)
{
  assert(acc < CACHE_ACC_LEVELS);
  acc_level = acc;
  stats.cache_acc.epochs++;
  stats.cache_acc.histogram[acc]++;
}

bool Pythia::is_high_bw() { return bw_level >= knob::pythia_high_bw_thresh ? true : false; }

void Pythia::dump_stats()
{
  cout << "pythia_st_lookup " << stats.st.lookup << endl
       << "pythia_st_hit " << stats.st.hit << endl
       << "pythia_st_evict " << stats.st.evict << endl
       << "pythia_st_insert " << stats.st.insert << endl
       << "pythia_st_streaming " << stats.st.streaming << endl
       << endl

       << "pythia_predict_called " << stats.predict.called
       << endl
       // << "pythia_predict_shaggy_called " << stats.predict.shaggy_called << endl
       << "pythia_predict_out_of_bounds " << stats.predict.out_of_bounds << endl;

  for (uint32_t index = 0; index < Actions.size(); ++index) {
    cout << "pythia_predict_action_" << Actions[index] << " " << stats.predict.action_dist[index] << endl;
    cout << "pythia_predict_issue_action_" << Actions[index] << " " << stats.predict.issue_dist[index] << endl;
    cout << "pythia_predict_hit_action_" << Actions[index] << " " << stats.predict.pred_hit[index] << endl;
    cout << "pythia_predict_out_of_bounds_action_" << Actions[index] << " " << stats.predict.out_of_bounds_dist[index] << endl;
  }

  cout << "pythia_predict_multi_deg_called " << stats.predict.multi_deg_called << endl
       << "pythia_predict_predicted " << stats.predict.predicted << endl
       << "pythia_predict_multi_deg " << stats.predict.multi_deg << endl;
  for (uint32_t index = 2; index <= MAX_PYTHIA_DEGREE; ++index) {
    cout << "pythia_predict_multi_deg_" << index << " " << stats.predict.multi_deg_histogram[index] << endl;
  }
  cout << endl;
  for (uint32_t index = 1; index <= MAX_PYTHIA_DEGREE; ++index) {
    cout << "pythia_selected_deg_" << index << " " << stats.predict.deg_histogram[index] << endl;
  }
  cout << endl;

  if (knob::pythia_enable_state_action_stats) {
    if (knob::pythia_enable_featurewise_engine) {
      std::vector<std::pair<string, vector<uint64_t>>> pairs;
      for (auto itr = state_action_dist2.begin(); itr != state_action_dist2.end(); ++itr)
        pairs.push_back(*itr);
      sort(pairs.begin(), pairs.end(), [](std::pair<string, vector<uint64_t>>& a, std::pair<string, vector<uint64_t>>& b) {
        return a.second[knob::pythia_max_actions] > b.second[knob::pythia_max_actions];
      });
      for (auto it = pairs.begin(); it != pairs.end(); ++it) {
        cout << "pythia_state_" << hex << it->first << dec << " ";
        for (uint32_t index = 0; index < it->second.size(); ++index) {
          cout << it->second[index] << ",";
        }
        cout << endl;
      }
    } else {
      for (auto it = state_action_dist.begin(); it != state_action_dist.end(); ++it) {
        cout << "pythia_state_" << hex << it->first << dec << " ";
        for (uint32_t index = 0; index < it->second.size(); ++index) {
          cout << it->second[index] << ",";
        }
        cout << endl;
      }
    }
  }
  cout << endl;

  for (auto it = action_deg_dist.begin(); it != action_deg_dist.end(); ++it) {
    cout << "pythia_action_" << it->first << "_deg_dist ";
    for (uint32_t index = 0; index < MAX_PYTHIA_DEGREE; ++index) {
      cout << it->second[index] << ",";
    }
    cout << endl;
  }
  cout << endl;

  cout << "pythia_track_called " << stats.track.called << endl
       << "pythia_track_same_address " << stats.track.same_address << endl
       << "pythia_track_evict " << stats.track.evict << endl
       << endl

       << "pythia_reward_demand_called " << stats.reward.demand.called << endl
       << "pythia_reward_demand_pt_not_found " << stats.reward.demand.pt_not_found << endl
       << "pythia_reward_demand_pt_found " << stats.reward.demand.pt_found << endl
       << "pythia_reward_demand_pt_found_total " << stats.reward.demand.pt_found_total << endl
       << "pythia_reward_demand_has_reward " << stats.reward.demand.has_reward << endl
       << "pythia_reward_train_called " << stats.reward.train.called << endl
       << "pythia_reward_assign_reward_called " << stats.reward.assign_reward.called << endl
       << "pythia_reward_no_pref " << stats.reward.no_pref << endl
       << "pythia_reward_incorrect " << stats.reward.incorrect << endl
       << "pythia_reward_correct_untimely " << stats.reward.correct_untimely << endl
       << "pythia_reward_correct_timely " << stats.reward.correct_timely << endl
       << "pythia_reward_out_of_bounds " << stats.reward.out_of_bounds << endl
       << "pythia_reward_tracker_hit " << stats.reward.tracker_hit << endl
       << "pythia_reward_incorrect_lowconf " << stats.reward.incorrect_lowconf << endl
       << "pythia_reward_correct_untimely_lowconf " << stats.reward.correct_untimely_lowconf << endl
       << "pythia_reward_correct_timely_lowconf " << stats.reward.correct_timely_lowconf << endl
       << endl;

  for (uint32_t reward = 0; reward < RewardType::num_rewards; ++reward) {
    cout << "pythia_reward_" << getRewardTypeString((RewardType)reward) << "_low_bw " << stats.reward.compute_reward.dist[reward][0] << endl
         << "pythia_reward_" << getRewardTypeString((RewardType)reward) << "_high_bw " << stats.reward.compute_reward.dist[reward][1] << endl;
  }
  cout << endl;

  for (uint32_t action = 0; action < Actions.size(); ++action) {
    cout << "pythia_reward_" << Actions[action] << " ";
    for (uint32_t reward = 0; reward < RewardType::num_rewards; ++reward) {
      cout << stats.reward.dist[action][reward] << ",";
    }
    cout << endl;
  }

  cout << endl
       << "pythia_train_called " << stats.train.called << endl
       << "pythia_train_compute_reward " << stats.train.compute_reward << endl
       << endl

       << "pythia_register_fill_called " << stats.register_fill.called << endl
       << "pythia_register_fill_set " << stats.register_fill.set << endl
       << "pythia_register_fill_set_total " << stats.register_fill.set_total << endl
       << "pythia_register_fill_set_actions ";
  for (uint32_t action_index = 0; action_index < Actions.size(); ++action_index) {
    cout << stats.register_fill.set_actions[action_index] << ",";
  }
  cout << endl << "pythia_register_fill_set_conf ";
  for (uint32_t index = 0; index < 2; ++index) {
    cout << stats.register_fill.set_conf[index] << ",";
  }
  cout << endl
       << endl

       << "pythia_register_llc_fill_called " << stats.register_llc_fill.called << endl
       << "pythia_register_llc_fill_set " << stats.register_llc_fill.set << endl
       << "pythia_register_llc_fill_set_total " << stats.register_llc_fill.set_total << endl
       << "pythia_register_llc_fill_set_actions ";
  for (uint32_t action_index = 0; action_index < Actions.size(); ++action_index) {
    cout << stats.register_llc_fill.set_actions[action_index] << ",";
  }
  cout << endl << "pythia_register_llc_fill_set_conf ";
  for (uint32_t index = 0; index < 2; ++index) {
    cout << stats.register_llc_fill.set_conf[index] << ",";
  }
  cout << endl
       << endl

       << "pythia_register_prefetch_hit_called " << stats.register_prefetch_hit.called << endl
       << "pythia_register_prefetch_hit_set " << stats.register_prefetch_hit.set << endl
       << "pythia_register_prefetch_hit_set_total " << stats.register_prefetch_hit.set_total << endl
       << endl

       << "pythia_pref_issue_pythia " << stats.pref_issue.pythia << endl
       << "pythia_low_conf_pref " << stats.confidence.low_conf << endl
       << "pythia_high_conf_pref " << stats.confidence.high_conf
       << endl
       // << "pythia_pref_issue_shaggy " << stats.pref_issue.shaggy << endl
       << endl;

  std::vector<std::pair<string, uint64_t>> pairs;
  for (auto itr = target_action_state.begin(); itr != target_action_state.end(); ++itr)
    pairs.push_back(*itr);
  sort(pairs.begin(), pairs.end(), [](std::pair<string, uint64_t>& a, std::pair<string, uint64_t>& b) { return a.second > b.second; });
  for (auto it = pairs.begin(); it != pairs.end(); ++it) {
    cout << it->first << "," << it->second << endl;
  }

  if (brain_featurewise) {
    brain_featurewise->dump_stats();
  }
  if (brain) {
    brain->dump_stats();
  }
  recorder->dump_stats();

  cout << "pythia_bw_epochs " << stats.bandwidth.epochs << endl;
  for (uint32_t index = 0; index < DRAM_BW_LEVELS; ++index) {
    cout << "pythia_bw_level_" << index << " " << stats.bandwidth.histogram[index] << endl;
  }
  cout << endl;

  cout << "pythia_ipc_epochs " << stats.ipc.epochs << endl;
  for (uint32_t index = 0; index < PYTHIA_MAX_IPC_LEVEL; ++index) {
    cout << "pythia_ipc_level_" << index << " " << stats.ipc.histogram[index] << endl;
  }
  cout << endl;

  cout << "pythia_cache_acc_epochs " << stats.cache_acc.epochs << endl;
  for (uint32_t index = 0; index < CACHE_ACC_LEVELS; ++index) {
    cout << "pythia_cache_acc_level_" << index << " " << stats.cache_acc.histogram[index] << endl;
  }
  cout << endl;

  // [DEBUG] Bias-only Q-values
  cout << "[DEBUG] Pythia action-bias stats:" << endl;
  State* dummy_state = new State();
  for (uint32_t action = 0; action < Actions.size(); ++action) {
    cout << "[DEBUG] " << setw(2) << Actions[action] << ": " << brain_featurewise->consultQ(dummy_state, action) << endl;
  }
}