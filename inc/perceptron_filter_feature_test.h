#ifndef PERCEPTRON_FILTER_FEATURE_TEST_H
#define PERCEPTRON_FILTER_FEATURE_TEST_H

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

#define FEATURE_TEST_LOGGING false
#define FEATURE_TEST false
#define FEATURE_TEST_VECTOR_TRAINING false

// enum PPF_PRED{PREFETCH_L2, PREFETCH_LLC, SUPPRESS};

struct Feature_Test_Info {
    uint64_t ip;
    uint64_t ip_1;
    uint64_t ip_2;
    uint64_t ip_3;
    uint64_t base_addr;
    int degree;
    int prefetch_offset;
    int dram_bucket;
    int dram_occupancy;
    int ip_accuracy_bucket;
    int degree_accuracy_bucket;
    int lateness_bucket;
    int pollution_bucket;
    bool prev_result;
    double dram_availability;
    double ip_accuracy;
    double pollution;
    double lateness;
    double degree_accuracy;
};

struct Feature_Test_Vector_Info {
    vector<uint64_t> ip_vector;
    vector<vector<uint64_t>> history_vector;
    vector<uint64_t> trigger_addr_vector;
    vector<int> degree_vector;
    vector<int> prefetch_offset_vector;
    vector<int> dram_bucket_vector;
    vector<int> pollution_bucket_vector;
    vector<int> lateness_bucket_vector;
    vector<int> ip_accuracy_bucket_vector;
    vector<int> degree_accuracy_bucket_vector;
    vector<int> dram_occupancy_vector;
    vector<bool> result_vector;
    int count;
};


#define NUM_TEST_FEATURES 4
#define NUM_TEST_BITS 8
#define FEATURE_DEPTH 8192

class Prefetch_Feature_Filter {
    Feature_Perceptron<NUM_TEST_BITS, false, 8192> features[NUM_TEST_FEATURES];
    std::map<uint64_t, Feature_Test_Info> prev_results;
    std::map<uint64_t, Feature_Test_Vector_Info> prev_results_vector;
    std::array<uint64_t, 4096> direct_mapped_reject_table;

    std::ofstream pca_out;
     
    int num_useful_blocks_evicted_threshold = 16384*2;
    double low_dram_availability_threshold = 0.4;
    double mid_dram_availability_threshold = 0.7;
    double low_accuracy_threshold = 0.4;
    double mid_accuracy_threshold = 0.7;
    int low_score_threshold = 20;
    int mid_score_threshold = 25;
    int training_threshold = 200;
    uint64_t interval_start;

    public:
    uint64_t demand_miss_interval_counter;
    std::map<uint64_t, uint64_t> ip_demand_miss_interval_map;
    uint64_t prefetch_miss_interval_counter;
    uint64_t demand_miss_total;
    int num_useful_blocks_evicted;
    uint64_t dram_availability_at_prefetch_issue_interval;
    uint64_t prefetch_fill_interval_counter;
    uint64_t late_prefetch_interval_counter;

    uint64_t demand_miss_latency_interval_counter;
    uint64_t prefetch_miss_latency_interval_counter;
    double current_demand_miss_latency_avg = 0.0;
    uint64_t useless_correct = 0;
    uint64_t useless_wrong = 0;
    uint64_t useful_correct = 0;
    uint64_t useful_wrong = 0;
    uint64_t total_training = 0;
    uint64_t total_predictions = 0;
    uint64_t accurate_predictions_count = 0;
    uint64_t total_predictions_count = 0;
    // int dram_occupancy;
    uint64_t memory_request_interval_counter;
    std::map<uint64_t, std::pair<int, int>> ip_accuracy_map;
    std::map<uint64_t, std::map<int, std::pair<int, int>>> ip_degree_accuracy_map;
    std::map<int, std::pair<int, int>> degree_accuracy_map;
    std::map<uint64_t, int> ip_late_map;

    std::map<uint64_t, std::pair<uint64_t, bool>> ip_pollution_map_tracker;
    std::map<uint64_t, bool> pollution_map;
    uint64_t pollution_interval_counter;
    uint64_t useful_prefetch_interval_counter;
    std::map<int, uint64_t> degree_pollution_map;
    std::map<uint64_t, std::pair<int, bool>> degree_pollution_tracker_map;
    std::map<uint64_t, double> addr_to_dram_availability_map;

    std::map<uint64_t, int> ip_pollution_map;

    std::map<uint64_t, int> ip_occupancy_map;

    Prefetch_Feature_Filter();

    void set_interval_start_cycle(uint64_t current_cycle);

    uint64_t get_interval_start_cycle(void);
    void train(uint64_t ip, bool result, bool reject_cache, bool useful);
    void train_vector(uint64_t pf_addr, bool result, bool reject_cache, bool useful);
    void get_indices(uint64_t base_addr, uint64_t ip, uint64_t ip_1, uint64_t ip_2, uint64_t ip_3, int32_t prefetch_offset, uint32_t degree, double dram_availability, int dram_occupancy,
    uint64_t perc_set[NUM_TEST_FEATURES], int* dram_bucket, int* pollution_bucket, int* ip_accuracy_bucket, int* lateness_bucket, int* degree_accuracy_bucket,
        double* ip_accuracy, double* lateness, double* pollution, double* degree_accuracy);
    void get_indices_training(uint64_t base_addr, uint64_t ip, uint64_t ip_1, uint64_t ip_2, uint64_t ip_3, int32_t prefetch_offset, uint32_t degree,
        int dram_bucket, int pollution_bucket, int ip_accuracy_bucket, int lateness_bucket, int degree_accuracy_bucket, double dram_availability, int dram_occupancy, double ip_accuracy, 
        double lateness, double pollution, double degree_accuracy, uint64_t perc_set[NUM_TEST_FEATURES]);

    bool get_prediction(uint64_t ip, uint64_t pf_addr, int degree, int rrpv, uint64_t set, uint64_t prefetch_trigger_addr, int prefetch_offset, double dram_availability, int dram_occupancy, bool harmony_pred, uint64_t score, bool miss, std::vector<uint64_t> history);

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


};

#endif
