#ifndef PREFETCH_IP_SUPPRESS_FILTER_H
#define PREFETCH_IP_SUPPRESS_FILTER_H

#include <stdint.h>
#include <map>
#include <vector>
#include <utility>
#include <iostream>
#include <set>
#include <cassert>
#include <fstream>
#include "perceptron.h"

using namespace std;

#define NUM_FEATURES 1
#define NUM_BEST_FEATURES 3
#define NUM_BITS 8
#define PERCEPTRON_IP_FILTER false
#define LOGGING false
#define NO_DRAM_OCCUPANCY false
#define PERCEPTRON_DRAM_AVAILABILITY false
#define PERCEPTRON_HISTORY false
#define PERCEPTRON_REJECT_TABLE false
#define PERCEPTRON_DIRECT_MAPPED_REJECT_TABLE false
#define PERCEPTRON_REJECT_CACHE false
#define PERCEPTRON_REJECT_CACHE_LOGGING false
#define PERCEPTRON_DEGREE false
#define PERCEPTRON_DEGREE_DRAM_AVAILABILITY false
#define PERCEPTRON_DEGREE_PREFETCH_OFFSET false
#define PERCEPTRON_DEGREE_PREFETCH_OFFSET_SCORE_BUCKET false
#define PERCEPTRON_DEGREE_PREFETCH_OFFSET_SCORE false
#define PERCEPTRON_DEGREE_PREFETCH_OFFSET_SCORE_MAP false
#define PERCEPTRON_DEGREE_PREFETCH_OFFSET_PLUS_PAGE false
#define PERCEPTRON_DEGREE_PREFETCH_OFFSET_PRED false
#define PERCEPTRON_DEGREE_XOR_PREFETCH_OFFSET false
#define PERCEPTRON_PC_XOR_DEGREE_PREFETCH_OFFSET false
#define PERCEPTRON_EXTERNAL_FEATURE false
#define PERCEPTRON_COMBINED false
#define PERCEPTRON_CONSTRAINED false
#define PERCEPTRON_GLOBAL_WEIGHTS false
#define PERCEPTRON_ACCURACY_GATE false
#define PERCEPTRON_IP_ACCURACY_GATE false
#define REJECT_CACHE_TRAIN_LOGGING false
#define PERCEPTRON_DEGREE_PREFETCH_OFFSET_DRAM_OCCUPANCY_WEIGHTS false
#define PERCEPTRON_DEGREE_PREFETCH_OFFSET_DRAM_OCCUPANCY_IP_HISTORY_WEIGHTS false

#define PERCEPTRON_DYNAMIC_DEGREE_CONTROL false
#define PERCEPTRON_PRINT_WEIGHTS false
#define PERCEPTRON_PCA false
#define PERCEPTRON_VECTOR_TRAINING false
#define BEST_FEATURE_TEST_IMPL false
#define DRAM_EXTRA_TRAINING false

#define NUM_DEGREE_XOR_OFFSET_ENTRIES 16
#define NUM_PC_DEGREE_ENTRIES 8192

#define ADAPTIVE_PREFETCH_FILTER false

#define PERCEPTRON_WITH_HARMONY false

#define LOW_PREFETCHER_ACCURACY 41
#define MID_PREFETCHER_ACCURACY 42
#define HIGH_PREFETCHER_ACCURACY 43
#define LOW_DRAM_AVAILABILITY 44
#define MID_DRAM_AVAILABILITY 45
#define HIGH_DRAM_AVAILABILITY 46
#define LOW_SCORE 50
#define MEDIUM_SCORE 51
#define HIGH_SCORE 52

#define max_rrpv 7

#define PPF false


// Perceptron paramaters
// #define PERC_ENTRIES 4096 //Upto 12-bit addressing in hashed perceptron
// #define PERC_FEATURES 7 //Keep increasing based on new features
// #define PERC_COUNTER_MAX 15 //-16 to +15: 5 bits counter 
// #define PERC_THRESHOLD_HI  -5     //-5
// #define PERC_THRESHOLD_LO  -15     // -15
// #define POS_UPDT_THRESHOLD  90
// #define NEG_UPDT_THRESHOLD -80
// #define SIG_DELTA_BIT 7

enum PPF_PRED{PREFETCH_L2, PREFETCH_LLC, SUPPRESS};
enum Training_Type{LOW, NORMAL, HIGH};

struct Prev_Result {
    bool result;
    int degree;
    uint64_t ip;
    uint64_t trigger_addr_page;
    int prefetch_offset;
    int dram_availability_category;
    int prefetcher_accuracy_category;
    bool harmony_prediction;
    uint64_t score;
    std::vector<uint64_t> history;
};

struct Best_Feature_Result {
  uint64_t ip;
  uint64_t ip_1;
  uint64_t ip_2;
  uint64_t ip_3;
  uint64_t trigger_addr;
  int degree;
  int prefetch_offset;
  int dram_bucket;
  int lateness_bucket;
  int pollution_bucket;
  int ip_accuracy_bucket;
  int degree_accuracy_bucket;
  bool result;
};

struct Best_Feature_Result_Vector {
  std::vector<uint64_t> ip_vector;
  std::vector<std::vector<uint64_t>> history_vector;
  std::vector<uint64_t> trigger_addr_vector;
  std::vector<int> degree_vector;
  std::vector<int> prefetch_offset_vector;
  std::vector<int> dram_bucket_vector;
  std::vector<int> lateness_bucket_vector;
  std::vector<int> pollution_bucket_vector;
  std::vector<int> ip_accuracy_bucket_vector;
  std::vector<int> degree_accuracy_bucket_vector;
  std::vector<bool> result_vector;
  int count;
};

struct Prev_Result_Vector {
  std::vector<uint64_t> ip_vector;
  std::vector<int> degree_vector;
  std::vector<int> prefetch_offset_vector;
  std::vector<std::vector<uint64_t>> history_vector;
  std::vector<int> dram_occupancy_vector;
  vector<bool> result_vector;
  int count;
};

struct Prev_Stat {
    bool result;
    double accuracy;
    double lateness;
    double pollution;
    double dram_availability;
    int degree;
    int prefetch_offset;
};

struct Reject_Cache {
    int rrpv;
    uint64_t pf_addr;
    bool hit;
};

struct APF {
    uint64_t victim_cache_line;
    bool valid_victim;

};

struct PPF_INFO {
    uint64_t addr;
    uint64_t base_addr;
    uint64_t ip;
    uint64_t ip_1;
    uint64_t ip_2;
    uint64_t ip_3;
    int sum;
    int delta;
    int depth;
    bool valid;
};

enum PV_Type {PHIT, VHIT, NO};
// enum low_accuracy_category{NONE, HIGH, MIDDLE, LOW};
#define MY_THRESHOLD_LO 1
#define MY_THRESHOLD_HI 5

class Prefetch_Filter {
    std::map<uint64_t, Perceptron<double, NUM_FEATURES, NUM_BITS>> perceptron_table;
    std::map<uint64_t, Feature_Perceptron<NUM_BITS, true, 100>> feature_perceptron_table;
    std::map<uint64_t, std::map<int, Feature_Perceptron<NUM_BITS, true, 100>>> feature_degree_perceptron_table;
    std::map<uint64_t, std::map<int, std::map<int, Feature_Perceptron<NUM_BITS, true, 100>>>> feature_degree_dram_category_perceptron_table;
    std::map<uint64_t, std::map<int, std::map<int, Feature_Perceptron<NUM_BITS, false, 100>>>> feature_degree_prefetch_offset_perceptron_table;
    Feature_Perceptron<NUM_BITS, false, 10000> features[NUM_BEST_FEATURES];
    std::map<uint64_t, std::map<uint32_t, Feature_Perceptron<NUM_BITS, false, 100>>> pc_degree_xor_prefetch_offset_perceptron_table;
    std::map<uint64_t, std::map<int, Feature_Perceptron<NUM_BITS, false, 100>>> pc_xor_degree_prefetch_offset_perceptron_table;
    std::map<uint64_t, Feature_Perceptron<NUM_BITS, false, 16>> constrained_feature_degree_prefetch_offset_table;
    // Feature_Perceptron<NUM_BITS, false> features_[NUM_FEATURES];
    Feature_Perceptron<NUM_BITS, false, 100> trigger_addr_page_weights;
    Feature_Perceptron<NUM_BITS, false, 100> dram_availability_weights;
    std::map<int, Feature_Perceptron<NUM_BITS, false, 100>> dram_occupancy_ip_history_weights;
    Feature_Perceptron<NUM_BITS, false, 100> score_weights;
    std::map<uint64_t, std::map<int, Feature_Perceptron<NUM_BITS, false, 100>>> score_weights_map;
    std::map<uint64_t, std::map<int, std::map<int, std::map<bool, Feature_Perceptron<NUM_BITS, true, 100>>>>> feature_degree_prefetch_offset_pred_perceptron_table;
    Feature_Perceptron<NUM_BITS, true, 100> weight_table;
    Counter_Predictor<NUM_BITS> APF;
    std::map<uint64_t, uint64_t> prefetch_victim_pair_map;
    std::map<uint64_t, uint64_t> victim_prefetch_pair_map;
    std::map<int, int> FC;
    std::map<uint64_t, Prev_Result> prev_results;
    std::map<uint64_t, Best_Feature_Result> best_feature_prev_results;
    std::map<uint64_t, Prev_Result_Vector> prev_results_vector;
    std::map<uint64_t, Best_Feature_Result_Vector> best_feature_results_vector;
    std::set<uint64_t> reject_table;
    std::array<uint64_t, 8192> direct_mapped_reject_table;
    std::map<uint64_t, std::vector<Reject_Cache>> reject_table_cache;
    std::vector<int> past_degrees;
    std::vector<int> dram_occupancy_global_vector;
    double average_degree; 
    uint64_t accurate_predictions_count;
    uint64_t total_predictions_count;
    uint64_t accurate_predictions_interval_counter;
    uint64_t total_predictions_interval_counter;
    // std::vector<low_accuracy_category> low_degree_accuracy;
    bool high_lateness = false;
    uint64_t useless_correct = 0;
    uint64_t useless_wrong = 0;
    uint64_t useful_correct = 0;
    uint64_t useful_wrong = 0;
    uint64_t total_training = 0;
    uint64_t total_predictions = 0;

    uint64_t useless_correct_interval_counter = 0;
    uint64_t useless_wrong_interval_counter = 0;
    uint64_t useful_correct_interval_counter = 0;
    uint64_t useful_wrong_interval_counter = 0;
    uint64_t total_training_interval_counter = 0;
    uint64_t total_predictions_interval_counter_2 = 0;
    bool decided = false;
    double width_good_ratio = 0.0;
    double depth_good_ratio = 0.0;
    bool width_testing_done = false;
    bool depth_testing_done = false;


    std::ofstream pca_out;


    int training_threshold = 200;
    bool higher_positive_training = true;
    bool disable_filter = false;
    int num_useful_blocks_evicted_threshold = 16384*2;
    int num_intervals_large = 4;
    double low_dram_availability_threshold = 0.4;
    double mid_dram_availability_threshold = 0.7;
    double low_accuracy_threshold = 0.4;
    double mid_accuracy_threshold = 0.7;
    int low_score_threshold = 20;
    int mid_score_threshold = 25;
    uint64_t interval_start;

    std::map<uint64_t, Prev_Stat> results;
    std::map<uint64_t, std::array<double, NUM_FEATURES>> past_features;

    public:
    bool depth = false;
    bool width = true;
    bool width_cont = true;
    int lo_threshold = 1;
    uint64_t demand_miss_interval_counter;
    uint64_t prefetch_miss_interval_counter;
    uint64_t prefetch_miss_total;
    uint64_t demand_miss_total;
    int num_useful_blocks_evicted;
    uint64_t dram_availability_at_prefetch_issue_interval;
    uint64_t bank_occupancy_at_prefetch_issue_interval;
    uint64_t prefetch_fill_interval_counter;
    uint64_t late_prefetch_interval_counter;
    double prefetch_miss_latency_avg = 0.0;

    uint64_t demand_miss_latency_interval_counter;
    uint64_t prefetch_miss_latency_interval_counter;
    double current_demand_miss_latency_avg = 0.0;
    // int dram_occupancy;
    uint64_t memory_request_interval_counter;
    std::map<uint64_t, std::pair<int, int>> ip_accuracy_map;
    std::map<uint64_t, std::map<int, std::pair<int, int>>> ip_degree_accuracy_map;
    std::map<int, std::pair<int, int>> degree_accuracy_map;
    std::map<uint64_t, int> ip_late_map;

    std::map<uint64_t, std::pair<uint64_t, bool>> pollution_map;
    std::map<uint64_t, bool> pollution_map_all;
    uint64_t pollution_interval_counter;
    uint64_t useful_prefetch_interval_counter;
    uint64_t useful_prefetch_total;
    std::map<int, uint64_t> degree_pollution_map;
    std::map<uint64_t, std::pair<int, bool>> degree_pollution_tracker_map;
    std::map<uint64_t, std::pair<uint64_t, bool>> in_cache_pollution_map;
    std::map<uint64_t, bool> pf_addr_pollution_map;
    std::map<uint64_t, bool> pf_addr_blocking_demand_map;
    std::map<uint64_t, double> addr_to_dram_availability_map;
    uint64_t total_false = 0;
    uint64_t num_false_with_negative_impact = 0;
    bool normal_false_training = false;
    Training_Type training_type = NORMAL;

    std::map<uint64_t, int> ip_pollution_map;

    std::map<uint64_t, int> ip_occupancy_map;

    Prefetch_Filter();

    void set_interval_start_cycle(uint64_t current_cycle);

    uint64_t get_interval_start_cycle(void);

    bool get_prediction(uint64_t ip, int dram_occupancy);
    void train(uint64_t ip, bool result, int dram_occupancy);

    bool get_prediction(uint64_t ip);
    void train(uint64_t pf_addr, bool result, bool reject_cache, bool useful, bool pollution_training, bool late, bool erase, int reward);
    void train_vector(uint64_t pf_addr, bool result, bool reject_cache, bool useful, bool pollution_training, bool late, bool erase, int reward);
    void train(uint64_t ip, bool result, uint64_t pf_addr);
    void train_best_feature(uint64_t pf_addr, bool result, bool reject_cache, bool useful);
    void train_best_feature_vector(uint64_t pf_addr, bool result, bool reject_cache, bool useful);

    bool get_prediction(uint64_t ip, double dram_availability, uint64_t pf_addr, int degree, int prefetch_offset);
    // void train(uint64_t ip, bool result);


    void train(uint64_t ip, uint64_t pf_addr, bool result, std::vector<uint64_t> history);
    PPF_PRED get_prediction(uint64_t ip, uint64_t pf_addr, int degree, int rrpv, uint64_t set, uint64_t prefetch_trigger_addr, int prefetch_offset, int dram_availability, bool harmony_pred, uint64_t score, bool miss, bool bypass, bool warmup, std::vector<uint64_t> history);
    int get_int_prediction(uint64_t ip, uint64_t pf_addr, int degree, uint64_t prefetch_trigger_addr, int prefetch_offset, vector<uint64_t> history);

    void get_indices(uint64_t base_addr, uint64_t ip, uint64_t ip_1, uint64_t ip_2, uint64_t ip_3, int32_t prefetch_offset, uint32_t degree, double dram_availability, 
        uint64_t perc_set[NUM_BEST_FEATURES], int* dram_bucket, int* pollution_bucket, int* ip_accuracy_bucket, int* lateness_bucket, int* degree_accuracy_bucket,
        double* ip_accuracy, double* lateness, double* pollution, double* degree_accuracy);

    void get_indices_training(uint64_t base_addr, uint64_t ip, uint64_t ip_1, uint64_t ip_2, uint64_t ip_3, int32_t prefetch_offset, uint32_t degree,
        int dram_bucket, int pollution_bucket, int ip_accuracy_bucket, int lateness_bucket, int degree_accuracy_bucket, double dram_availability, double ip_accuracy, 
        double lateness, double pollution, double degree_accuracy, uint64_t perc_set[NUM_BEST_FEATURES]);
    void add_to_reject_table(uint64_t ip, uint64_t pf_addr, int degree, uint64_t prefetch_trigger_addr, int prefetch_offset, bool result, vector<uint64_t> history);
    
    bool check_reject_table(uint64_t addr);
    void erase_reject_table(uint64_t addr);

    bool check_direct_mapped_reject_table(uint64_t addr);
    void erase_direct_mapped_reject_table(uint64_t addr);

    bool is_interval_complete(void);
    int reset_interval(uint64_t current_cycle, int current_degree);

    void print_ip_accuracy_map(void);

    void print_ip_degree_accuracy_map(void);

    void print_degree_accuracy_map(void);

    void print_pollution_map(void);

    void print_perceptrons(void);

    void verify_stats(uint64_t useful_prefetch_total, uint64_t prefetch_fill_total, uint64_t late_total, uint64_t pollution_total);

    void update_reject_cache(uint64_t set);

    bool check_reject_cache(uint64_t set, uint64_t pf_addr);

    void erase_reject_cache(uint64_t set, uint64_t pf_addr);

    void evict_victim_reject_cache(uint64_t set);

    void add_prefetch_victim_addr_pair(uint64_t prefetch_cache_line, uint64_t victim_cache_line);

    void invalidate_prefetch_victim_addr_pair(uint64_t prefetch_cache_line);

    PV_Type check_prefetch_victim(uint64_t cache_line);

    bool get_apf_prediction(uint64_t cache_line);

    void update_apf_with_demand_access(uint64_t demand_access);

    void update_apf_with_victim_cache_miss(uint64_t victim);

    void add_filtered_prefetch(uint64_t prefetch_cache_line);
    
    bool find_past_degrees_not_four(void);
    void print_FC(void);

    void print_reject_cache(uint64_t set);
    int get_perceptron_ip_size(void);

};

// class PERCEPTRON_ {
//   public:
//     // Perc Weights
//     int32_t perc_weights[PERC_ENTRIES][PERC_FEATURES];

//     // Only for dumping csv
//     bool    perc_touched[PERC_ENTRIES][PERC_FEATURES];

//     // CONST depths for different features
//     int32_t PERC_DEPTH[PERC_FEATURES];

//     std::array<PPF_INFO, 1024> reject_table;
//     std::array<PPF_INFO, 1024> prefetch_table;

//     PERCEPTRON_() {
//       cout << "\nInitialize PERCEPTRON" << endl;
//       cout << "PERC_ENTRIES: " << PERC_ENTRIES << endl;
//       cout << "PERC_FEATURES: " << PERC_FEATURES << endl;

//       PERC_DEPTH[0] = 2048;   //base_addr;
//       PERC_DEPTH[1] = 4096;   //cache_line;
//       PERC_DEPTH[2] = 4096;  	//page_addr;
//       PERC_DEPTH[3] = 4096; 	//ip_1 ^ ip_2 ^ ip_3;		
//       PERC_DEPTH[4] = 1024; 	//ip ^ depth;
//       PERC_DEPTH[5] = 2048;   //ip ^ sig_delta;
//       PERC_DEPTH[6] = 128;    // confidence / score

//       for (int i = 0; i < PERC_ENTRIES; i++) {
//         for (int j = 0;j < PERC_FEATURES; j++) {
//           perc_weights[i][j] = 0;
//           perc_touched[i][j] = 0;
//         }
//       }
//     }

//     void get_perc_index(uint64_t base_addr, uint64_t ip, uint64_t ip_1, uint64_t ip_2, uint64_t ip_3, int32_t cur_delta, uint32_t depth, uint64_t perc_set[PERC_FEATURES]);

//     void	 perc_update(uint64_t check_addr, uint64_t ip, uint64_t ip_1, uint64_t ip_2, uint64_t ip_3, int32_t cur_delta, uint32_t depth, bool direction, int32_t perc_sum);
//     int32_t	perc_predict(uint64_t check_addr, uint64_t ip, uint64_t ip_1, uint64_t ip_2, uint64_t ip_3, int32_t cur_delta, uint32_t depth);
//     void perc_train_from_prefetch_table(uint64_t addr, bool result);
//     void perc_train_from_reject_table(uint64_t addr, bool result);
//     PPF_PRED get_perc_prediction(uint64_t base_addr, uint64_t pf_addr, uint64_t ip, uint64_t ip_1, uint64_t ip_2, uint64_t ip_3, int32_t offset, uint32_t degree);
//     bool check_reject_table(uint64_t base_addr);
//     void update_prefetch_and_reject_table(uint64_t pf_addr);
//     uint64_t get_hash(uint64_t key);
//     void print_perceptron_weights(void);
// };


#endif

