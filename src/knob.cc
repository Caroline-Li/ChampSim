#include "knob.h"

#include <iostream>
#include <math.h>
#include <string.h>
#include <string>

#include "ini.h"
using namespace std;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

namespace knob
{
uint64_t warmup_instructions = 1000000;
uint64_t simulation_instructions = 10000000;
uint32_t champsim_seed = 0;
bool hide_heartbeat = false;
bool knob_cloudsuite = false;
vector<string> llc_prefetcher_types;
vector<string> l2c_prefetcher_types;
vector<string> l1d_prefetcher_types;
bool l1d_perfect = false;
bool l2c_perfect = false;
bool llc_perfect = false;
bool l1d_semi_perfect = false;
bool l2c_semi_perfect = false;
bool llc_semi_perfect = false;
uint32_t semi_perfect_cache_page_buffer_size = 64;
bool measure_ipc = false;
uint64_t measure_ipc_epoch = 1000;
uint32_t dram_io_freq = 2400;
bool measure_dram_bw = true;
uint64_t measure_dram_bw_epoch = 256;
bool measure_cache_acc = true;
uint64_t measure_cache_acc_epoch = 1024;
bool ignore_btb = false;
bool save_memory_cycle_dist = false;
string memory_cycle_dist_dir = string("/dev/null");

/* Prefetch statistics, per-PC and per-address */
bool measure_pc_prefetches = false;
bool measure_addr_prefetches = false;
string pc_prefetch_file_l1d = string("/dev/null"); // Per-PC prefetch statistics
string pc_prefetch_file_l2c = string("/dev/null");
string pc_prefetch_file_llc = string("/dev/null");
string addr_prefetch_file_l1d = string("/dev/null"); // Per-address prefetch statistics
string addr_prefetch_file_l2c = string("/dev/null");
string addr_prefetch_file_llc = string("/dev/null");

/* Prefetch address traces */
bool dump_prefetch_trace = false;
string prefetch_trace_llc = string("/dev/null");

/* multi_pc_trace */
string pc_trace_llc = string("/dev/null");
bool pc_trace_invoke_all = false;
bool pc_trace_credit_prefetch = false;

/* AC Table */
uint32_t ac_table_rt_size = 16;
uint32_t ac_table_trigger = 0; /* AC_TABLE_TRIGGER_ADDR */
uint32_t ac_table_stream = 1;  /* AC_TABLE_STREAM_PC */
uint32_t ac_table_max_frequency = 8;

/* Cygnus */
uint32_t cygnus_pref_degree = 1;
uint32_t cygnus_eq_size = 256;
bool cygnus_enable_track_multiple = false;
bool cygnus_enable_action_fallback = true;
bool cygnus_enable_page_boundaries = true;
bool cygnus_enable_fine_grained_timeliness = false;

/* IP-Stride */
uint32_t ip_stride_pref_degree = 1;
uint32_t ip_stride_tracker_count = 1024;
bool ip_stride_enable_page_boundaries = true;
bool ip_stride_fill_lower_level = false;

/* Next Line */
uint32_t next_line_pref_degree = 1;
int32_t next_line_offset = 1;
bool next_line_enable_page_boundaries = true;
uint32_t next_line_min_page_offset = 0;

/* SMS */
uint32_t sms_at_size = 32;
uint32_t sms_ft_size = 64;
uint32_t sms_pht_size = 16384;
uint32_t sms_pht_assoc = 16;
uint32_t sms_pref_degree = 4;
uint32_t sms_region_size = 2048;
uint32_t sms_region_size_log = 11;
bool sms_enable_pref_buffer = true;
uint32_t sms_pref_buffer_size = 256;

/* SPP */
uint32_t spp_st_size = 256;
uint32_t spp_pt_size = 512;
uint32_t spp_max_outcomes = 4;
double spp_max_confidence = 25.0;
uint32_t spp_max_depth = 64;
uint32_t spp_max_prefetch_per_level = 1;
uint32_t spp_max_confidence_counter_value = 16;
uint32_t spp_max_global_counter_value = 1024;
uint32_t spp_pf_size = 1024;
bool spp_enable_alpha = true;
bool spp_enable_pref_buffer = true;
uint32_t spp_pref_buffer_size = 256;
uint32_t spp_pref_degree = 4;
bool spp_enable_ghr = true;
uint32_t spp_ghr_size = 8;
uint32_t spp_signature_bits = 12;
uint32_t spp_alpha_epoch = 1024;

/* SPP_dev2 */
uint32_t spp_dev2_fill_threshold = 90;
uint32_t spp_dev2_pf_threshold = 25;
uint32_t spp_dev2_max_degree = 0; // 0 = no limit
bool spp_dev2_pf_l2_only = false;
bool spp_dev2_pf_llc_only = false;

/* BOP */
vector<int32_t> bo_candidates;
int32_t bo_default_candidate = 1;
int32_t bo_fallback_candidate = 0;
uint32_t bo_max_rounds = 100;
uint32_t bo_max_score = 31;
uint32_t bo_low_score = 20;
uint32_t bo_bad_score = 10;
uint32_t bo_rr_entries = 64;
uint32_t bo_pref_degree = 1;
bool bo_enable_page_boundaries = false;

/* Domino */
uint32_t domino_pref_degree = 1;
bool domino_eit_entry_limit = false;

/* ISB Ideal */
uint32_t isb_ideal_pref_degree = 1;
uint32_t isb_ideal_pref_distance = 1;
uint32_t isb_ideal_buffer_size = 128;
bool isb_ideal_restrict_region = false;
bool isb_ideal_cfix = false;
uint32_t isb_ideal_max_stream_length = 1024;
bool isb_ideal_enable_confidence_thresholds = false;
std::vector<uint32_t> isb_ideal_confidence_thresholds = {0};

/* ISB Real */
uint32_t isb_real_pref_degree = 1;
uint32_t isb_real_pref_distance = 1;
uint32_t isb_real_buffer_size = 1024;
bool isb_real_restrict_region = false;
bool isb_real_cfix = false;
bool isb_real_tlb_sync = true;
uint32_t isb_real_amc_ways = 256;
uint32_t isb_real_max_stream_length = 1024;
bool isb_real_enable_confidence_thresholds = false;
std::vector<uint32_t> isb_real_confidence_thresholds = {0};

/* STMS */
uint32_t stms_pref_degree = 1;

/* SISB */
uint32_t sisb_pref_degree = 1;
uint32_t sisb_pref_distance = 1;

/* Sandbox */
uint32_t sandbox_pref_degree = 4;
bool sandbox_enable_stream_detect = false;
uint32_t sandbox_stream_detect_length = 4;
uint32_t sandbox_num_access_in_phase = 256;
uint32_t sandbox_num_cycle_offsets = 4;
uint32_t sandbox_bloom_filter_size = 2048;
uint32_t sandbox_seed = 200;

/* DSPatch */
uint32_t dspatch_log2_region_size;
uint32_t dspatch_num_cachelines_in_region;
uint32_t dspatch_pb_size;
uint32_t dspatch_num_spt_entries;
uint32_t dspatch_compression_granularity;
uint32_t dspatch_pred_throttle_bw_thr;
uint32_t dspatch_bitmap_selection_policy;
uint32_t dspatch_sig_type;
uint32_t dspatch_sig_hash_type;
uint32_t dspatch_or_count_max;
uint32_t dspatch_measure_covP_max;
uint32_t dspatch_measure_accP_max;
uint32_t dspatch_acc_thr;
uint32_t dspatch_cov_thr;
bool dspatch_enable_pref_buffer;
uint32_t dspatch_pref_buffer_size;
uint32_t dspatch_pref_degree;

/* PPF */
int32_t ppf_perc_threshold_hi = -5;
int32_t ppf_perc_threshold_lo = -15;

/* MLOP */
uint32_t mlop_pref_degree;
uint32_t mlop_num_updates;
float mlop_l1d_thresh;
float mlop_l2c_thresh;
float mlop_llc_thresh;
uint32_t mlop_debug_level;

/* Bingo */
uint32_t bingo_region_size = 2048;
uint32_t bingo_pattern_len = 32;
uint32_t bingo_pc_width = 16;
uint32_t bingo_min_addr_width = 5;
uint32_t bingo_max_addr_width = 16;
uint32_t bingo_ft_size = 64;
uint32_t bingo_at_size = 128;
uint32_t bingo_pht_size = 8192;
uint32_t bingo_pht_ways = 16;
uint32_t bingo_pf_streamer_size = 128;
uint32_t bingo_debug_level = 0;
uint32_t bingo_max_degree = 0; // 0 = no limit
float bingo_l1d_thresh;
float bingo_l2c_thresh;
float bingo_llc_thresh;
string bingo_pc_address_fill_level;
bool bingo_pf_l2_only;
bool bingo_pf_llc_only;

/* Stride */
uint32_t stride_num_trackers = 64;
uint32_t stride_pref_degree = 2;

/* Streamer */
uint32_t streamer_num_trackers = 64;
uint32_t streamer_pref_degree = 5; /* models IBM POWER7 */

/* AMPM */
uint32_t ampm_pb_size = 64;
uint32_t ampm_pred_degree = 4;
uint32_t ampm_pref_degree = 4;
uint32_t ampm_pref_buffer_size = 256;
bool ampm_enable_pref_buffer = false;
uint32_t ampm_max_delta = 16;

/* Context Prefetcher */
uint32_t cp_cst_size = 2048;
uint32_t cp_cst_assoc = 16;
uint32_t cp_max_response_per_cst = 4;
int32_t cp_init_reward = 0;
uint32_t cp_prefetch_queue_size = 128;

/* From File Prefetcher */
std::string from_file_pref_path = "";
uint32_t from_file_pref_max_degree = 128;

/* Load Tracer (Prefetcher) */
std::string load_tracer_path = "/dev/null";
bool load_tracer_miss_only = false;

/* IBM POWER7 Prefetcher */
uint32_t power7_explore_epoch = 1000;
uint32_t power7_exploit_epoch = 100000;
uint32_t power7_default_streamer_degree = 4;

/* Pythia */
float pythia_alpha = 0.0065;
float pythia_gamma = 0.55;
float pythia_epsilon = 0.002;
uint32_t pythia_state_num_bits = 10;
uint32_t pythia_max_states = 1024;
uint32_t pythia_seed = 200;
string pythia_policy = std::string("EGreedy");
string pythia_learning_type = std::string("SARSA");
vector<int32_t> pythia_actions;
uint32_t pythia_max_actions = 64;
uint32_t pythia_pt_size = 256;
uint32_t pythia_st_size = 64;
int32_t pythia_reward_none = -4;
int32_t pythia_reward_incorrect = -8;
int32_t pythia_reward_incorrect_lowconf = -6;
int32_t pythia_reward_correct_untimely = 12;
int32_t pythia_reward_correct_untimely_lowconf = 10;
int32_t pythia_reward_correct_timely = 20;
int32_t pythia_reward_correct_timely_lowconf = 18;
uint32_t pythia_max_pcs = 5;
uint32_t pythia_max_offsets = 5;
uint32_t pythia_max_deltas = 5;
bool pythia_brain_zero_init = false;
bool pythia_enable_reward_all = false;
bool pythia_enable_track_multiple = false;
bool pythia_enable_reward_out_of_bounds = true;
int32_t pythia_reward_out_of_bounds = -12;
uint32_t pythia_state_type = 1;
bool pythia_access_debug = false;
bool pythia_print_access_debug = false;
uint64_t pythia_print_access_debug_pc = 0xdeadbeef;
uint32_t pythia_print_access_debug_pc_count = 0;
bool pythia_print_trace = false;
bool pythia_enable_state_action_stats = false;
bool pythia_enable_reward_tracker_hit = false;
int32_t pythia_reward_tracker_hit = -2;
bool pythia_enable_shaggy = false;
uint32_t pythia_state_hash_type = 11;
bool pythia_prefetch_with_shaggy = false;
bool pythia_enable_featurewise_engine = true;
uint32_t pythia_pref_degree = 1; /* default is set to 1 */
bool pythia_enable_dyn_degree = false;
vector<float> pythia_max_to_avg_q_thresholds; /* depricated */
vector<int32_t> pythia_dyn_degrees;
uint64_t pythia_early_exploration_window = 0;
uint32_t pythia_multi_deg_select_type = 2; /* type 1 is already depricated */
vector<int32_t> pythia_last_pref_offset_conf_thresholds;
vector<int32_t> pythia_dyn_degrees_type2;
uint32_t pythia_action_tracker_size = 2;
uint32_t pythia_high_bw_thresh = 4;
bool pythia_enable_hbw_reward = true;
int32_t pythia_reward_hbw_correct_timely = 20;
int32_t pythia_reward_hbw_correct_timely_lowconf = 18;
int32_t pythia_reward_hbw_correct_untimely = 12;
int32_t pythia_reward_hbw_correct_untimely_lowconf = 10;
int32_t pythia_reward_hbw_incorrect = -14;
int32_t pythia_reward_hbw_incorrect_lowconf = -12;
int32_t pythia_reward_hbw_none = -2;
int32_t pythia_reward_hbw_out_of_bounds = -12;
int32_t pythia_reward_hbw_tracker_hit = -2;
vector<int32_t> pythia_last_pref_offset_conf_thresholds_hbw;
vector<int32_t> pythia_dyn_degrees_type2_hbw;
bool pythia_enable_dyn_level = false;   // Prefetch in L2 if high-confidence, LLC if low-confidence
float pythia_dyn_level_threshold = 0.0; // TODO : Possibly use an automatic scheme instead?
bool pythia_separate_lowconf_pt = true; // Whether to use separate EQs for high
                                        // and low confidence prefetches.
uint32_t pythia_lowconf_pt_size = 1024;
bool pythia_enable_page_boundaries = true;

/* Learning Engine */
bool le_enable_trace;
uint32_t le_trace_interval;
string le_trace_file_name;
uint32_t le_trace_state;
bool le_enable_score_plot;
vector<int32_t> le_plot_actions;
string le_plot_file_name;
bool le_enable_action_trace;
uint32_t le_action_trace_interval;
std::string le_action_trace_name;
bool le_enable_action_plot;

/* Featurewise Learning Engine */
vector<int32_t> le_featurewise_active_features;
vector<int32_t> le_featurewise_num_tilings;
vector<int32_t> le_featurewise_num_tiles;
vector<int32_t> le_featurewise_hash_types;
vector<int32_t> le_featurewise_enable_tiling_offset;
float le_featurewise_max_q_thresh = 0.5;
bool le_featurewise_enable_action_fallback = true;
vector<float> le_featurewise_feature_weights;
bool le_featurewise_enable_dynamic_weight = false;
uint32_t le_featurewise_weight_type = 0;
float le_featurewise_weight_gradient = 0.001;
float le_featurewise_weight_minimum = 0.01;
bool le_featurewise_disable_adjust_weight_all_features_align = false;
bool le_featurewise_enable_weight_normalization = false;
bool le_featurewise_enable_action_bias = false;
bool le_featurewise_selective_update = false;
uint32_t le_featurewise_pooling_type = 2; /* max pooling */
bool le_featurewise_enable_dyn_action_fallback = true;
uint32_t le_featurewise_bw_acc_check_level = 1;
uint32_t le_featurewise_acc_thresh = 2;

bool le_featurewise_enable_trace = false;
uint32_t le_featurewise_trace_feature_type;
string le_featurewise_trace_feature;
uint32_t le_featurewise_trace_interval;
uint32_t le_featurewise_trace_record_count;
std::string le_featurewise_trace_file_name;
bool le_featurewise_enable_score_plot;
vector<int32_t> le_featurewise_plot_actions;
std::string le_featurewise_plot_file_name;
bool le_featurewise_remove_plot_script;

/* Hawkeye */
uint32_t hawkeye_rrpv_bits = 3;
uint32_t hawkeye_sampler_cache_size = 2800;
uint32_t hawkeye_max_shct = 31;
uint32_t hawkeye_shct_size = 2048;
bool hawkeye_use_sampling = true;
uint32_t hawkeye_sampled_sets = 64;
uint32_t hawkeye_hash_type = 2;
} // namespace knob

void parse_args(int argc, char* argv[])
{
  for (int index = 0; index < argc; ++index) {
    string arg = string(argv[index]);
    if (arg.compare(0, 2, "--") == 0) {
      arg = arg.substr(2);
    }
    if (ini_parse_string(arg.c_str(), handler, NULL) < 0) {
      printf("error parsing commandline %s\n", argv[index]);
      exit(1);
    }
  }
}

int handler(void* user, const char* section, const char* name, const char* value)
{
  char config_file_name[MAX_LEN];

  if (MATCH("", "config")) {
    strcpy(config_file_name, value);
    parse_config(config_file_name);
  } else {
    parse_knobs(user, section, name, value);
  }
  return 1;
}

void parse_config(char* config_file_name)
{
  cout << "parsing config file: " << string(config_file_name) << endl;
  if (ini_parse(config_file_name, parse_knobs, NULL) < 0) {
    printf("Failed to load %s\n", config_file_name);
    exit(1);
  }
}

int parse_knobs(void* user, const char* section, const char* name, const char* value)
{
  char config_file_name[MAX_LEN];

  if (MATCH("", "config")) {
    strcpy(config_file_name, value);
    parse_config(config_file_name);
  } else if (MATCH("", "warmup_instructions")) {
    knob::warmup_instructions = atol(value);
  } else if (MATCH("", "simulation_instructions")) {
    knob::simulation_instructions = atol(value);
  } else if (MATCH("", "champsim_seed")) {
    knob::champsim_seed = atol(value);
  } else if (MATCH("", "hide_heartbeat")) {
    knob::hide_heartbeat = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "knob_cloudsuite")) {
    knob::knob_cloudsuite = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "llc_prefetcher_types")) {
    knob::llc_prefetcher_types.push_back(string(value));
  } else if (MATCH("", "l2c_prefetcher_types")) {
    knob::l2c_prefetcher_types.push_back(string(value));
  } else if (MATCH("", "l1d_prefetcher_types")) {
    knob::l1d_prefetcher_types.push_back(string(value));
  } else if (MATCH("", "l1d_perfect")) {
    knob::l1d_perfect = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "l2c_perfect")) {
    knob::l2c_perfect = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "llc_perfect")) {
    knob::llc_perfect = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "l1d_semi_perfect")) {
    knob::l1d_semi_perfect = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "l2c_semi_perfect")) {
    knob::l2c_semi_perfect = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "llc_semi_perfect")) {
    knob::llc_semi_perfect = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "semi_perfect_cache_page_buffer_size")) {
    knob::semi_perfect_cache_page_buffer_size = atoi(value);
  } else if (MATCH("", "measure_ipc")) {
    knob::measure_ipc = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "measure_ipc_epoch")) {
    knob::measure_ipc_epoch = atoi(value);
  } else if (MATCH("", "dram_io_freq")) {
    knob::dram_io_freq = atoi(value);
  } else if (MATCH("", "measure_dram_bw")) {
    knob::measure_dram_bw = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "measure_dram_bw_epoch")) {
    knob::measure_dram_bw_epoch = atoi(value);
  } else if (MATCH("", "measure_cache_acc")) {
    knob::measure_cache_acc = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "measure_cache_acc_epoch")) {
    knob::measure_cache_acc_epoch = atoi(value);
  } else if (MATCH("", "ignore_btb")) {
    knob::ignore_btb = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "save_memory_cycle_dist")) {
    knob::save_memory_cycle_dist = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "memory_cycle_dist_dir")) {
    knob::memory_cycle_dist_dir = string(value);
  }

  else if (MATCH("", "measure_pc_prefetches")) {
    knob::measure_pc_prefetches = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "measure_addr_prefetches")) {
    knob::measure_addr_prefetches = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "pc_prefetch_file_l1d")) {
    knob::pc_prefetch_file_l1d = string(value);
  } else if (MATCH("", "pc_prefetch_file_l2c")) {
    knob::pc_prefetch_file_l2c = string(value);
  } else if (MATCH("", "pc_prefetch_file_llc")) {
    knob::pc_prefetch_file_llc = string(value);
  } else if (MATCH("", "addr_prefetch_file_l1d")) {
    knob::addr_prefetch_file_l1d = string(value);
  } else if (MATCH("", "addr_prefetch_file_l2c")) {
    knob::addr_prefetch_file_l2c = string(value);
  } else if (MATCH("", "addr_prefetch_file_llc")) {
    knob::addr_prefetch_file_llc = string(value);
  }

  /* prefetch trace */
  else if (MATCH("", "dump_prefetch_trace")) {
    knob::dump_prefetch_trace = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "prefetch_trace_llc")) {
    knob::prefetch_trace_llc = string(value);
  }

  /* multi_pc_trace */
  else if (MATCH("", "pc_trace_llc")) {
    knob::pc_trace_llc = string(value);
  } else if (MATCH("", "pc_trace_invoke_all")) {
    knob::pc_trace_invoke_all = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "pc_trace_credit_prefetch")) {
    knob::pc_trace_credit_prefetch = !strcmp(value, "true") ? true : false;
  }

  /* AC Table */
  else if (MATCH("", "ac_table_rt_size")) {
    knob::ac_table_rt_size = atoi(value);
  } else if (MATCH("", "ac_table_trigger")) {
    knob::ac_table_trigger = atoi(value);
  } else if (MATCH("", "ac_table_stream")) {
    knob::ac_table_stream = atoi(value);
  } else if (MATCH("", "ac_table_max_frequency")) {
    knob::ac_table_max_frequency = atoi(value);
  }

  /* Cygnus */
  else if (MATCH("", "cygnus_pref_degree")) {
    knob::cygnus_pref_degree = atoi(value);
  } else if (MATCH("", "cygnus_eq_size")) {
    knob::cygnus_eq_size = atoi(value);
  } else if (MATCH("", "cygnus_enable_track_multiple")) {
    knob::cygnus_enable_track_multiple = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "cygnus_enable_action_fallback")) {
    knob::cygnus_enable_action_fallback = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "cygnus_enable_page_boundaries")) {
    knob::cygnus_enable_page_boundaries = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "cygnus_enable_fine_grained_timeliness")) {
    knob::cygnus_enable_fine_grained_timeliness = !strcmp(value, "true") ? true : false;
  }

  /* IP-Stride */
  else if (MATCH("", "ip_stride_pref_degree")) {
    knob::ip_stride_pref_degree = atoi(value);
  } else if (MATCH("", "ip_stride_tracker_count")) {
    knob::ip_stride_tracker_count = atoi(value);
  } else if (MATCH("", "ip_stride_enable_page_boundaries")) {
    knob::ip_stride_enable_page_boundaries = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "ip_stride_fill_lower_level")) {
    knob::ip_stride_fill_lower_level = !strcmp(value, "true") ? true : false;
  }

  /* Next Line */
  else if (MATCH("", "next_line_pref_degree")) {
    knob::next_line_pref_degree = atoi(value);
  } else if (MATCH("", "next_line_offset")) {
    knob::next_line_offset = atoi(value);
  } else if (MATCH("", "next_line_enable_page_boundaries")) {
    knob::next_line_enable_page_boundaries = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "next_line_min_page_offset")) {
    knob::next_line_min_page_offset = atoi(value);
  }

  /* SMS */
  else if (MATCH("", "sms_at_size")) {
    knob::sms_at_size = atoi(value);
  } else if (MATCH("", "sms_ft_size")) {
    knob::sms_ft_size = atoi(value);
  } else if (MATCH("", "sms_pht_size")) {
    knob::sms_pht_size = atoi(value);
  } else if (MATCH("", "sms_pht_assoc")) {
    knob::sms_pht_assoc = atoi(value);
  } else if (MATCH("", "sms_pref_degree")) {
    knob::sms_pref_degree = atoi(value);
  } else if (MATCH("", "sms_region_size")) {
    knob::sms_region_size = atoi(value);
    knob::sms_region_size_log = log2(knob::sms_region_size);
  } else if (MATCH("", "sms_enable_pref_buffer")) {
    knob::sms_enable_pref_buffer = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "sms_pref_buffer_size")) {
    knob::sms_pref_buffer_size = atoi(value);
  }

  /* SPP */
  else if (MATCH("", "spp_st_size")) {
    knob::spp_st_size = atoi(value);
  } else if (MATCH("", "spp_pt_size")) {
    knob::spp_pt_size = atoi(value);
  } else if (MATCH("", "spp_max_outcomes")) {
    knob::spp_max_outcomes = atoi(value);
  } else if (MATCH("", "spp_max_confidence")) {
    knob::spp_max_confidence = strtod(value, NULL);
  } else if (MATCH("", "spp_max_depth")) {
    knob::spp_max_depth = atoi(value);
  } else if (MATCH("", "spp_max_prefetch_per_level")) {
    knob::spp_max_prefetch_per_level = atoi(value);
  } else if (MATCH("", "spp_max_confidence_counter_value")) {
    knob::spp_max_confidence_counter_value = atoi(value);
  } else if (MATCH("", "spp_max_global_counter_value")) {
    knob::spp_max_global_counter_value = atoi(value);
  } else if (MATCH("", "spp_pf_size")) {
    knob::spp_pf_size = atoi(value);
  } else if (MATCH("", "spp_enable_alpha")) {
    knob::spp_enable_alpha = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "spp_enable_pref_buffer")) {
    knob::spp_enable_pref_buffer = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "spp_pref_buffer_size")) {
    knob::spp_pref_buffer_size = atoi(value);
  } else if (MATCH("", "spp_pref_degree")) {
    knob::spp_pref_degree = atoi(value);
  } else if (MATCH("", "spp_enable_ghr")) {
    knob::spp_enable_ghr = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "spp_ghr_size")) {
    knob::spp_ghr_size = atoi(value);
  } else if (MATCH("", "spp_signature_bits")) {
    knob::spp_signature_bits = atoi(value);
  } else if (MATCH("", "spp_alpha_epoch")) {
    knob::spp_alpha_epoch = atoi(value);
  }

  /* SPP_dev2 */
  else if (MATCH("", "spp_dev2_fill_threshold")) {
    knob::spp_dev2_fill_threshold = atoi(value);
  } else if (MATCH("", "spp_dev2_pf_threshold")) {
    knob::spp_dev2_pf_threshold = atoi(value);
  } else if (MATCH("", "spp_dev2_max_degree")) {
    knob::spp_dev2_max_degree = atoi(value);
  } else if (MATCH("", "spp_dev2_pf_l2_only")) {
    knob::spp_dev2_pf_l2_only = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "spp_dev2_pf_llc_only")) {
    knob::spp_dev2_pf_llc_only = !strcmp(value, "true") ? true : false;
  }

  /* BO */
  else if (MATCH("", "bo_candidates")) {
    knob::bo_candidates = get_array_int(value);
  } else if (MATCH("", "bo_default_candidate")) {
    knob::bo_default_candidate = atoi(value);
  } else if (MATCH("", "bo_fallback_candidate")) {
    knob::bo_fallback_candidate = atoi(value);
  } else if (MATCH("", "bo_max_rounds")) {
    knob::bo_max_rounds = atoi(value);
  } else if (MATCH("", "bo_max_score")) {
    knob::bo_max_score = atoi(value);
  } else if (MATCH("", "bo_low_score")) {
    knob::bo_low_score = atoi(value);
  } else if (MATCH("", "bo_bad_score")) {
    knob::bo_bad_score = atoi(value);
  } else if (MATCH("", "bo_rr_entries")) {
    knob::bo_rr_entries = atoi(value);
  } else if (MATCH("", "bo_pref_degree")) {
    knob::bo_pref_degree = atoi(value);
  } else if (MATCH("", "bo_enable_page_boundaries")) {
    knob::bo_enable_page_boundaries = !strcmp(value, "true") ? true : false;
  }

  /* Domino */
  else if (MATCH("", "domino_pref_degree")) {
    knob::domino_pref_degree = atoi(value);
  } else if (MATCH("", "domino_eit_entry_limit")) {
    knob::domino_pref_degree = !strcmp(value, "true") ? true : false;
  }

  /* File Prefetcher */
  else if (MATCH("", "from_file_pref_path")) {
    knob::from_file_pref_path = value;
  } else if (MATCH("", "from_file_pref_max_degree")) {
    knob::from_file_pref_max_degree = atoi(value);
  }

  /* Load Tracer (Prefetcher) */
  else if (MATCH("", "load_tracer_path")) {
    knob::load_tracer_path = value;
  } else if (MATCH("", "load_tracer_miss_only")) {
    knob::load_tracer_miss_only = !strcmp(value, "true") ? true : false;
  }

  /* ISB Ideal */
  else if (MATCH("", "isb_ideal_pref_degree")) {
    knob::isb_ideal_pref_degree = atoi(value);
  } else if (MATCH("", "isb_ideal_pref_distance")) {
    knob::isb_ideal_pref_distance = atoi(value);
  } else if (MATCH("", "isb_ideal_buffer_size")) {
    knob::isb_ideal_buffer_size = atoi(value);
  } else if (MATCH("", "isb_ideal_restrict_region")) {
    knob::isb_ideal_restrict_region = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "isb_ideal_cfix")) {
    knob::isb_ideal_cfix = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "isb_ideal_max_stream_length")) {
    knob::isb_ideal_max_stream_length = atoi(value);
  } else if (MATCH("", "isb_ideal_enable_confidence_thresholds")) {
    knob::isb_ideal_enable_confidence_thresholds = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "isb_ideal_confidence_thresholds")) {
    knob::isb_ideal_confidence_thresholds = get_array_uint(value);
  }

  /* ISB Real */
  else if (MATCH("", "isb_real_pref_degree")) {
    knob::isb_real_pref_degree = atoi(value);
  } else if (MATCH("", "isb_real_pref_distance")) {
    knob::isb_real_pref_distance = atoi(value);
  } else if (MATCH("", "isb_real_buffer_size")) {
    knob::isb_real_buffer_size = atoi(value);
  } else if (MATCH("", "isb_real_restrict_region")) {
    knob::isb_real_restrict_region = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "isb_real_cfix")) {
    knob::isb_real_cfix = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "isb_real_tlb_sync")) {
    knob::isb_real_tlb_sync = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "isb_real_amc_ways")) {
    knob::isb_real_amc_ways = atoi(value);
  } else if (MATCH("", "isb_real_max_stream_length")) {
    knob::isb_real_max_stream_length = atoi(value);
  } else if (MATCH("", "isb_real_enable_confidence_thresholds")) {
    knob::isb_real_enable_confidence_thresholds = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "isb_real_confidence_thresholds")) {
    knob::isb_real_confidence_thresholds = get_array_uint(value);
  }

  /* STMS */
  else if (MATCH("", "stms_pref_degree")) {
    knob::stms_pref_degree = atoi(value);
  }

  /* SISB */
  else if (MATCH("", "sisb_pref_degree")) {
    knob::sisb_pref_degree = atoi(value);
  } else if (MATCH("", "sisb_pref_distance")) {
    knob::sisb_pref_distance = atoi(value);
  }

  /* Sandbox */
  else if (MATCH("", "sandbox_pref_degree")) {
    knob::sandbox_pref_degree = atoi(value);
  } else if (MATCH("", "sandbox_enable_stream_detect")) {
    knob::sandbox_enable_stream_detect = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "sandbox_stream_detect_length")) {
    knob::sandbox_stream_detect_length = atoi(value);
  } else if (MATCH("", "sandbox_num_access_in_phase")) {
    knob::sandbox_num_access_in_phase = atoi(value);
  } else if (MATCH("", "sandbox_num_cycle_offsets")) {
    knob::sandbox_num_cycle_offsets = atoi(value);
  } else if (MATCH("", "sandbox_bloom_filter_size")) {
    knob::sandbox_bloom_filter_size = atoi(value);
  } else if (MATCH("", "sandbox_seed")) {
    knob::sandbox_seed = atoi(value);
  }

  /* DSPatch */
  else if (MATCH("", "dspatch_log2_region_size")) {
    knob::dspatch_log2_region_size = atoi(value);
    knob::dspatch_num_cachelines_in_region = 1 << (knob::dspatch_log2_region_size - 6); /* considers traditional 64B cachelines */
  } else if (MATCH("", "dspatch_pb_size")) {
    knob::dspatch_pb_size = atoi(value);
  } else if (MATCH("", "dspatch_num_spt_entries")) {
    knob::dspatch_num_spt_entries = atoi(value);
  } else if (MATCH("", "dspatch_compression_granularity")) {
    knob::dspatch_compression_granularity = atoi(value);
  } else if (MATCH("", "dspatch_pred_throttle_bw_thr")) {
    knob::dspatch_pred_throttle_bw_thr = atoi(value);
  } else if (MATCH("", "dspatch_bitmap_selection_policy")) {
    knob::dspatch_bitmap_selection_policy = atoi(value);
  } else if (MATCH("", "dspatch_sig_type")) {
    knob::dspatch_sig_type = atoi(value);
  } else if (MATCH("", "dspatch_sig_hash_type")) {
    knob::dspatch_sig_hash_type = atoi(value);
  } else if (MATCH("", "dspatch_or_count_max")) {
    knob::dspatch_or_count_max = atoi(value);
  } else if (MATCH("", "dspatch_measure_covP_max")) {
    knob::dspatch_measure_covP_max = atoi(value);
  } else if (MATCH("", "dspatch_measure_accP_max")) {
    knob::dspatch_measure_accP_max = atoi(value);
  } else if (MATCH("", "dspatch_acc_thr")) {
    knob::dspatch_acc_thr = atoi(value);
  } else if (MATCH("", "dspatch_cov_thr")) {
    knob::dspatch_cov_thr = atoi(value);
  } else if (MATCH("", "dspatch_enable_pref_buffer")) {
    knob::dspatch_enable_pref_buffer = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "dspatch_pref_buffer_size")) {
    knob::dspatch_pref_buffer_size = atoi(value);
  } else if (MATCH("", "dspatch_pref_degree")) {
    knob::dspatch_pref_degree = atoi(value);
  }

  /* PPF */
  else if (MATCH("", "ppf_perc_threshold_hi")) {
    knob::ppf_perc_threshold_hi = atoi(value);
  } else if (MATCH("", "ppf_perc_threshold_lo")) {
    knob::ppf_perc_threshold_lo = atoi(value);
  }

  /* MLOP */
  else if (MATCH("", "mlop_pref_degree")) {
    knob::mlop_pref_degree = atoi(value);
  } else if (MATCH("", "mlop_num_updates")) {
    knob::mlop_num_updates = atoi(value);
  } else if (MATCH("", "mlop_l1d_thresh")) {
    knob::mlop_l1d_thresh = atof(value);
  } else if (MATCH("", "mlop_l2c_thresh")) {
    knob::mlop_l2c_thresh = atof(value);
  } else if (MATCH("", "mlop_llc_thresh")) {
    knob::mlop_llc_thresh = atof(value);
  } else if (MATCH("", "mlop_debug_level")) {
    knob::mlop_debug_level = atoi(value);
  }

  /* Bingo */
  else if (MATCH("", "bingo_region_size")) {
    knob::bingo_region_size = atoi(value);
  } else if (MATCH("", "bingo_pattern_len")) {
    knob::bingo_pattern_len = atoi(value);
  } else if (MATCH("", "bingo_pc_width")) {
    knob::bingo_pc_width = atoi(value);
  } else if (MATCH("", "bingo_min_addr_width")) {
    knob::bingo_min_addr_width = atoi(value);
  } else if (MATCH("", "bingo_max_addr_width")) {
    knob::bingo_max_addr_width = atoi(value);
  } else if (MATCH("", "bingo_ft_size")) {
    knob::bingo_ft_size = atoi(value);
  } else if (MATCH("", "bingo_at_size")) {
    knob::bingo_at_size = atoi(value);
  } else if (MATCH("", "bingo_pht_size")) {
    knob::bingo_pht_size = atoi(value);
  } else if (MATCH("", "bingo_pht_ways")) {
    knob::bingo_pht_ways = atoi(value);
  } else if (MATCH("", "bingo_pf_streamer_size")) {
    knob::bingo_pf_streamer_size = atoi(value);
  } else if (MATCH("", "bingo_debug_level")) {
    knob::bingo_debug_level = atoi(value);
  } else if (MATCH("", "bingo_max_degree")) {
    knob::bingo_max_degree = atoi(value);
  } else if (MATCH("", "bingo_l1d_thresh")) {
    knob::bingo_l1d_thresh = atof(value);
  } else if (MATCH("", "bingo_l2c_thresh")) {
    knob::bingo_l2c_thresh = atof(value);
  } else if (MATCH("", "bingo_llc_thresh")) {
    knob::bingo_llc_thresh = atof(value);
  } else if (MATCH("", "bingo_pc_address_fill_level")) {
    knob::bingo_pc_address_fill_level = string(value);
  } else if (MATCH("", "bingo_pf_l2_only")) {
    knob::bingo_pf_l2_only = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "bingo_pf_llc_only")) {
    knob::bingo_pf_llc_only = !strcmp(value, "true") ? true : false;
  }

  /* Stride Prefetcher */
  else if (MATCH("", "stride_num_trackers")) {
    knob::stride_num_trackers = atoi(value);
  } else if (MATCH("", "stride_pref_degree")) {
    knob::stride_pref_degree = atoi(value);
  }

  else if (MATCH("", "streamer_num_trackers")) {
    knob::streamer_num_trackers = atoi(value);
  } else if (MATCH("", "streamer_pref_degree")) {
    knob::streamer_pref_degree = atoi(value);
  }

  /* AMPM */
  else if (MATCH("", "ampm_pb_size")) {
    knob::ampm_pb_size = atoi(value);
  } else if (MATCH("", "ampm_pred_degree")) {
    knob::ampm_pred_degree = atoi(value);
  } else if (MATCH("", "ampm_pref_degree")) {
    knob::ampm_pref_degree = atoi(value);
  } else if (MATCH("", "ampm_pref_buffer_size")) {
    knob::ampm_pref_buffer_size = atoi(value);
  } else if (MATCH("", "ampm_enable_pref_buffer")) {
    knob::ampm_enable_pref_buffer = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "ampm_max_delta")) {
    knob::ampm_max_delta = atoi(value);
  }

  /* Context Prefetcher */
  else if (MATCH("", "cp_cst_size")) {
    knob::cp_cst_size = atoi(value);
  } else if (MATCH("", "cp_cst_assoc")) {
    knob::cp_cst_assoc = atoi(value);
  } else if (MATCH("", "cp_max_response_per_cst")) {
    knob::cp_max_response_per_cst = atoi(value);
  } else if (MATCH("", "cp_init_reward")) {
    knob::cp_init_reward = atoi(value);
  } else if (MATCH("", "cp_prefetch_queue_size")) {
    knob::cp_prefetch_queue_size = atoi(value);
  }

  /* POWER7 Prefetcher */
  else if (MATCH("", "power7_explore_epoch")) {
    knob::power7_explore_epoch = atoi(value);
  } else if (MATCH("", "power7_exploit_epoch")) {
    knob::power7_exploit_epoch = atoi(value);
  } else if (MATCH("", "power7_default_streamer_degree")) {
    knob::power7_default_streamer_degree = atoi(value);
  }

  /* Pythia */
  else if (MATCH("", "pythia_alpha")) {
    knob::pythia_alpha = atof(value);
  } else if (MATCH("", "pythia_gamma")) {
    knob::pythia_gamma = atof(value);
  } else if (MATCH("", "pythia_epsilon")) {
    knob::pythia_epsilon = atof(value);
  } else if (MATCH("", "pythia_state_num_bits")) {
    knob::pythia_state_num_bits = atoi(value);
    knob::pythia_max_states = pow(2.0, knob::pythia_state_num_bits);
  } else if (MATCH("", "pythia_seed")) {
    knob::pythia_seed = atoi(value);
  } else if (MATCH("", "pythia_policy")) {
    knob::pythia_policy = string(value);
  } else if (MATCH("", "pythia_learning_type")) {
    knob::pythia_learning_type = string(value);
  } else if (MATCH("", "pythia_actions")) {
    knob::pythia_actions = get_array_int(value);
    knob::pythia_max_actions = knob::pythia_actions.size();
  } else if (MATCH("", "pythia_pt_size")) {
    knob::pythia_pt_size = atoi(value);
  } else if (MATCH("", "pythia_st_size")) {
    knob::pythia_st_size = atoi(value);
  } else if (MATCH("", "pythia_reward_none")) {
    knob::pythia_reward_none = atoi(value);
  } else if (MATCH("", "pythia_reward_incorrect")) {
    knob::pythia_reward_incorrect = atoi(value);
  } else if (MATCH("", "pythia_reward_incorrect_lowconf")) {
    knob::pythia_reward_incorrect_lowconf = atoi(value);
  } else if (MATCH("", "pythia_reward_correct_untimely")) {
    knob::pythia_reward_correct_untimely = atoi(value);
  } else if (MATCH("", "pythia_reward_correct_untimely_lowconf")) {
    knob::pythia_reward_correct_untimely_lowconf = atoi(value);
  } else if (MATCH("", "pythia_reward_correct_timely")) {
    knob::pythia_reward_correct_timely = atoi(value);
  } else if (MATCH("", "pythia_reward_correct_timely_lowconf")) {
    knob::pythia_reward_correct_timely_lowconf = atoi(value);
  } else if (MATCH("", "pythia_max_pcs")) {
    knob::pythia_max_pcs = atoi(value);
  } else if (MATCH("", "pythia_max_offsets")) {
    knob::pythia_max_offsets = atoi(value);
  } else if (MATCH("", "pythia_max_deltas")) {
    knob::pythia_max_deltas = atoi(value);
  } else if (MATCH("", "pythia_brain_zero_init")) {
    knob::pythia_brain_zero_init = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "pythia_enable_reward_all")) {
    knob::pythia_enable_reward_all = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "pythia_enable_track_multiple")) {
    knob::pythia_enable_track_multiple = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "pythia_enable_reward_out_of_bounds")) {
    knob::pythia_enable_reward_out_of_bounds = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "pythia_reward_out_of_bounds")) {
    knob::pythia_reward_out_of_bounds = atoi(value);
  } else if (MATCH("", "pythia_state_type")) {
    knob::pythia_state_type = atoi(value);
  } else if (MATCH("", "pythia_access_debug")) {
    knob::pythia_access_debug = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "pythia_print_access_debug")) {
    knob::pythia_print_access_debug = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "pythia_print_access_debug_pc")) {
    knob::pythia_print_access_debug_pc = strtoul(value, NULL, 0);
  } else if (MATCH("", "pythia_print_access_debug_pc_count")) {
    knob::pythia_print_access_debug_pc_count = atoi(value);
  } else if (MATCH("", "pythia_print_trace")) {
    knob::pythia_print_trace = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "pythia_enable_state_action_stats")) {
    knob::pythia_enable_state_action_stats = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "pythia_enable_reward_tracker_hit")) {
    knob::pythia_enable_reward_tracker_hit = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "pythia_reward_tracker_hit")) {
    knob::pythia_reward_tracker_hit = atoi(value);
  } else if (MATCH("", "pythia_enable_shaggy")) {
    knob::pythia_enable_shaggy = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "pythia_state_hash_type")) {
    knob::pythia_state_hash_type = atoi(value);
  } else if (MATCH("", "pythia_prefetch_with_shaggy")) {
    knob::pythia_prefetch_with_shaggy = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "pythia_enable_featurewise_engine")) {
    knob::pythia_enable_featurewise_engine = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "pythia_pref_degree")) {
    knob::pythia_pref_degree = atoi(value);
  } else if (MATCH("", "pythia_enable_dyn_degree")) {
    knob::pythia_enable_dyn_degree = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "pythia_max_to_avg_q_thresholds")) {
    knob::pythia_max_to_avg_q_thresholds = get_array_float(value);
  } else if (MATCH("", "pythia_early_exploration_window")) {
    knob::pythia_early_exploration_window = atoi(value);
  } else if (MATCH("", "pythia_dyn_degrees")) {
    knob::pythia_dyn_degrees = get_array_int(value);
  } else if (MATCH("", "pythia_multi_deg_select_type")) {
    knob::pythia_multi_deg_select_type = atoi(value);
  } else if (MATCH("", "pythia_last_pref_offset_conf_thresholds")) {
    knob::pythia_last_pref_offset_conf_thresholds = get_array_int(value);
  } else if (MATCH("", "pythia_dyn_degrees_type2")) {
    knob::pythia_dyn_degrees_type2 = get_array_int(value);
  } else if (MATCH("", "pythia_action_tracker_size")) {
    knob::pythia_action_tracker_size = atoi(value);
  } else if (MATCH("", "pythia_high_bw_thresh")) {
    knob::pythia_high_bw_thresh = atoi(value);
  } else if (MATCH("", "pythia_enable_hbw_reward")) {
    knob::pythia_enable_hbw_reward = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "pythia_reward_hbw_correct_timely")) {
    knob::pythia_reward_hbw_correct_timely = atoi(value);
  } else if (MATCH("", "pythia_reward_hbw_correct_timely_lowconf")) {
    knob::pythia_reward_hbw_correct_timely_lowconf = atoi(value);
  } else if (MATCH("", "pythia_reward_hbw_correct_untimely")) {
    knob::pythia_reward_hbw_correct_untimely = atoi(value);
  } else if (MATCH("", "pythia_reward_hbw_correct_untimely_lowconf")) {
    knob::pythia_reward_hbw_correct_untimely_lowconf = atoi(value);
  } else if (MATCH("", "pythia_reward_hbw_incorrect")) {
    knob::pythia_reward_hbw_incorrect = atoi(value);
  } else if (MATCH("", "pythia_reward_hbw_incorrect_lowconf")) {
    knob::pythia_reward_hbw_incorrect_lowconf = atoi(value);
  } else if (MATCH("", "pythia_reward_hbw_none")) {
    knob::pythia_reward_hbw_none = atoi(value);
  } else if (MATCH("", "pythia_reward_hbw_out_of_bounds")) {
    knob::pythia_reward_hbw_out_of_bounds = atoi(value);
  } else if (MATCH("", "pythia_reward_hbw_tracker_hit")) {
    knob::pythia_reward_hbw_tracker_hit = atoi(value);
  } else if (MATCH("", "pythia_last_pref_offset_conf_thresholds_hbw")) {
    knob::pythia_last_pref_offset_conf_thresholds_hbw = get_array_int(value);
  } else if (MATCH("", "pythia_dyn_degrees_type2_hbw")) {
    knob::pythia_dyn_degrees_type2_hbw = get_array_int(value);
  } else if (MATCH("", "pythia_enable_dyn_level")) {
    knob::pythia_enable_dyn_level = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "pythia_dyn_level_threshold")) {
    knob::pythia_dyn_level_threshold = stof(value);
  } else if (MATCH("", "pythia_separate_lowconf_pt")) {
    knob::pythia_separate_lowconf_pt = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "pythia_lowconf_pt_size")) {
    knob::pythia_lowconf_pt_size = atoi(value);
  } else if (MATCH("", "pythia_enable_page_boundaries")) {
    knob::pythia_enable_page_boundaries = !strcmp(value, "true") ? true : false;
  }

  /* Learning Engine */
  else if (MATCH("", "le_enable_trace")) {
    knob::le_enable_trace = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "le_trace_interval")) {
    knob::le_trace_interval = atoi(value);
  } else if (MATCH("", "le_trace_file_name")) {
    knob::le_trace_file_name = string(value);
  } else if (MATCH("", "le_trace_state")) {
    knob::le_trace_state = strtoul(value, NULL, 0);
  } else if (MATCH("", "le_enable_score_plot")) {
    knob::le_enable_score_plot = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "le_plot_actions")) {
    knob::le_plot_actions = get_array_int(value);
  } else if (MATCH("", "le_plot_file_name")) {
    knob::le_plot_file_name = string(value);
  } else if (MATCH("", "le_enable_action_trace")) {
    knob::le_enable_action_trace = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "le_action_trace_interval")) {
    knob::le_action_trace_interval = atoi(value);
  } else if (MATCH("", "le_action_trace_name")) {
    knob::le_action_trace_name = string(value);
  } else if (MATCH("", "le_enable_action_plot")) {
    knob::le_enable_action_plot = !strcmp(value, "true") ? true : false;
  }

  /* Featurewise Learning Engine */
  else if (MATCH("", "le_featurewise_active_features")) {
    knob::le_featurewise_active_features = get_array_int(value);
  } else if (MATCH("", "le_featurewise_num_tilings")) {
    knob::le_featurewise_num_tilings = get_array_int(value);
  } else if (MATCH("", "le_featurewise_num_tiles")) {
    knob::le_featurewise_num_tiles = get_array_int(value);
  } else if (MATCH("", "le_featurewise_hash_types")) {
    knob::le_featurewise_hash_types = get_array_int(value);
  } else if (MATCH("", "le_featurewise_enable_tiling_offset")) {
    knob::le_featurewise_enable_tiling_offset = get_array_int(value);
  } else if (MATCH("", "le_featurewise_max_q_thresh")) {
    knob::le_featurewise_max_q_thresh = atof(value);
  } else if (MATCH("", "le_featurewise_enable_action_fallback")) {
    knob::le_featurewise_enable_action_fallback = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "le_featurewise_feature_weights")) {
    knob::le_featurewise_feature_weights = get_array_float(value);
  } else if (MATCH("", "le_featurewise_enable_dynamic_weight")) {
    knob::le_featurewise_enable_dynamic_weight = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "le_featurewise_weight_type")) {
    knob::le_featurewise_weight_type = atoi(value);
  } else if (MATCH("", "le_featurewise_weight_gradient")) {
    knob::le_featurewise_weight_gradient = atof(value);
  } else if (MATCH("", "le_featurewise_weight_minimum")) {
    knob::le_featurewise_weight_minimum = atof(value);
  } else if (MATCH("", "le_featurewise_disable_adjust_weight_all_features_align")) {
    knob::le_featurewise_disable_adjust_weight_all_features_align = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "le_featurewise_enable_weight_normalization")) {
    knob::le_featurewise_enable_weight_normalization = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "le_featurewise_enable_action_bias")) {
    knob::le_featurewise_enable_action_bias = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "le_featurewise_selective_update")) {
    knob::le_featurewise_selective_update = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "le_featurewise_pooling_type")) {
    knob::le_featurewise_pooling_type = atoi(value);
  } else if (MATCH("", "le_featurewise_enable_dyn_action_fallback")) {
    knob::le_featurewise_enable_dyn_action_fallback = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "le_featurewise_bw_acc_check_level")) {
    knob::le_featurewise_bw_acc_check_level = atoi(value);
  } else if (MATCH("", "le_featurewise_acc_thresh")) {
    knob::le_featurewise_acc_thresh = atoi(value);
  } else if (MATCH("", "le_featurewise_enable_trace")) {
    knob::le_featurewise_enable_trace = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "le_featurewise_trace_feature_type")) {
    knob::le_featurewise_trace_feature_type = atoi(value);
  } else if (MATCH("", "le_featurewise_trace_feature")) {
    knob::le_featurewise_trace_feature = string(value);
  } else if (MATCH("", "le_featurewise_trace_interval")) {
    knob::le_featurewise_trace_interval = atoi(value);
  } else if (MATCH("", "le_featurewise_trace_record_count")) {
    knob::le_featurewise_trace_record_count = atoi(value);
  } else if (MATCH("", "le_featurewise_trace_file_name")) {
    knob::le_featurewise_trace_file_name = string(value);
  } else if (MATCH("", "le_featurewise_enable_score_plot")) {
    knob::le_featurewise_enable_score_plot = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "le_featurewise_plot_actions")) {
    knob::le_featurewise_plot_actions = get_array_int(value);
  } else if (MATCH("", "le_featurewise_plot_file_name")) {
    knob::le_featurewise_plot_file_name = string(value);
  } else if (MATCH("", "le_featurewise_remove_plot_script")) {
    knob::le_featurewise_remove_plot_script = !strcmp(value, "true") ? true : false;
  }

  /* Hawkeye */
  else if (MATCH("", "hawkeye_rrpv_bits")) {
    knob::hawkeye_rrpv_bits = atoi(value);
  } else if (MATCH("", "hawkeye_sampler_cache_size")) {
    knob::hawkeye_sampler_cache_size = atoi(value);
  } else if (MATCH("", "hawkeye_max_shct")) {
    knob::hawkeye_max_shct = atoi(value);
  } else if (MATCH("", "hawkeye_shct_size")) {
    knob::hawkeye_shct_size = atoi(value);
  } else if (MATCH("", "hawkeye_use_sampling")) {
    knob::hawkeye_use_sampling = !strcmp(value, "true") ? true : false;
  } else if (MATCH("", "hawkeye_sampled_sets")) {
    knob::hawkeye_sampled_sets = atoi(value);
  } else if (MATCH("", "hawkeye_hash_type")) {
    knob::hawkeye_hash_type = atoi(value);
  }

  else {
    printf("unable to parse section: %s, name: %s, value: %s\n", section, name, value);
    return 0;
  }
  return 1;
}

std::vector<uint32_t> get_array_uint(const char* str)
{
  std::vector<uint32_t> value;
  char* tmp_str = strdup(str);
  char* pch = strtok(tmp_str, ",");
  while (pch) {
    value.push_back(strtol(pch, NULL, 0));
    pch = strtok(NULL, ",");
  }
  free(tmp_str);
  return value;
}

std::vector<int32_t> get_array_int(const char* str)
{
  std::vector<int32_t> value;
  char* tmp_str = strdup(str);
  char* pch = strtok(tmp_str, ",");
  while (pch) {
    value.push_back(strtol(pch, NULL, 0));
    pch = strtok(NULL, ",");
  }
  free(tmp_str);
  return value;
}

std::vector<float> get_array_float(const char* str)
{
  std::vector<float> value;
  char* tmp_str = strdup(str);
  char* pch = strtok(tmp_str, ",");
  while (pch) {
    value.push_back(atof(pch));
    pch = strtok(NULL, ",");
  }
  free(tmp_str);
  return value;
}