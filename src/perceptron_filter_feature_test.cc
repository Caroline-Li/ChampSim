#include "perceptron_filter_feature_test.h"
#include "champsim_constants.h"
#include <math.h>
using namespace std;

// #define SPP_DEBUG_PRINT false

//#define SPP_DEBUG_PRINT
#ifdef SPP_DEBUG_PRINT
#define SPP_DP(x) x
#else
#define SPP_DP(x)
#endif

//#define SPP_PERC_WGHT
#ifdef SPP_PERC_WGHT
#define SPP_PW(x) x
#else 
#define SPP_PW(x)
#endif

// Prefetch filter parameters
#define QUOTIENT_BIT  10
#define REMAINDER_BIT 6
#define HASH_BIT (QUOTIENT_BIT + REMAINDER_BIT + 1)
#define FILTER_SET (1 << QUOTIENT_BIT)

#define QUOTIENT_BIT_REJ  10
#define REMAINDER_BIT_REJ 8
#define HASH_BIT_REJ (QUOTIENT_BIT_REJ + REMAINDER_BIT_REJ + 1)
#define FILTER_SET_REJ (1 << QUOTIENT_BIT_REJ)

#define CACHE_LINE(a) (a >> LOG2_BLOCK_SIZE)

#define PAGE(a) (a >> LOG2_PAGE_SIZE)

Prefetch_Feature_Filter::Prefetch_Feature_Filter() {
}
void Prefetch_Feature_Filter::set_interval_start_cycle(uint64_t current_cycle) {
    interval_start = current_cycle;
}

uint64_t Prefetch_Feature_Filter::get_interval_start_cycle(void) {
    return interval_start;
}

void Prefetch_Feature_Filter::get_indices_training(uint64_t base_addr, uint64_t ip, uint64_t ip_1, uint64_t ip_2, uint64_t ip_3, int32_t prefetch_offset, uint32_t degree,
        int dram_bucket, int pollution_bucket, int ip_accuracy_bucket, int lateness_bucket, int degree_accuracy_bucket, double dram_availability, int dram_occupancy, double ip_accuracy, 
        double lateness, double pollution, double degree_accuracy, uint64_t perc_set[NUM_TEST_FEATURES]) {
    // get features
    uint64_t  pre_hash[NUM_TEST_FEATURES];

    //get pre-hash indices
    // pre_hash[0] = ip ^ degree ^ (prefetch_offset + 40);
    // pre_hash[1] = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);

    // pre_hash[0] = base_addr;
    // pre_hash[1] = CACHE_LINE(base_addr);
    // pre_hash[2] = PAGE(base_addr);
    // pre_hash[3] = ip ^ degree;
    // pre_hash[4] = ip ^ (prefetch_offset + 40);
    // pre_hash[5] = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);

    pre_hash[0] = ip;
    pre_hash[1] = degree;
    pre_hash[2] = prefetch_offset + 40;
    pre_hash[3] = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
    // pre_hash[3] = ip_1;
    // pre_hash[4] = ip_2;
    // pre_hash[5] = ip_3;
    // pre_hash[4] = dram_occupancy;
    // pre_hash[5] = pollution_bucket;
    // pre_hash[6] = degree_accuracy_bucket;
    // pre_hash[7] = ip_accuracy_bucket;
    // pre_hash[8] = lateness_bucket;

    // uint64_t ip_history = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
    // pre_hash[0] = ip;
    // pre_hash[1] = degree;
    // pre_hash[2] = prefetch_offset + 40;
    // pre_hash[3] = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
    // pre_hash[4] = dram_bucket;
    // pre_hash[5] = pollution_bucket;
    // pre_hash[6] = degree_accuracy_bucket;
    // pre_hash[7] = ip_accuracy_bucket;
    // pre_hash[8] = lateness_bucket;
    // pre_hash[9] = PAGE(base_addr);
    // pre_hash[10] = CACHE_LINE(base_addr);
    // pre_hash[11] = dram_availability*100;
    // pre_hash[12] = (int) ip_accuracy*100;
    // pre_hash[13] = (int) lateness*100;
    // pre_hash[14] = (int) pollution*100;
    // pre_hash[15] = (int) degree_accuracy*100;
    // pre_hash[16] = dram_occupancy;
    // pre_hash[17] = ip ^ degree;
    // pre_hash[18] = ip ^ prefetch_offset;
    // pre_hash[19] = ip ^ degree ^ prefetch_offset;
    // pre_hash[20] = ip ^ degree ^ prefetch_offset ^ ip_history;
    // pre_hash[21] = ip_1;
    // pre_hash[22] = ip_2;
    // pre_hash[23] = ip_3;
    
    // pre_hash[0] = base_addr;
    // pre_hash[1] = CACHE_LINE(base_addr);
    // pre_hash[2] = PAGE(base_addr);
    // pre_hash[3] = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
    // pre_hash[4] = ip ^ degree;
    // pre_hash[5] = ip ^ prefetch_offset;

    // uint64_t ip_history = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
    // pre_hash[0] = ip;
    // pre_hash[1] = ip ^ degree;
    // pre_hash[2] = ip ^ degree ^ ip_history;
    // pre_hash[3] = ip ^ prefetch_offset;
    // pre_hash[4] = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
    // pre_hash[5] = ip ^ degree ^ prefetch_offset;
    // pre_hash[6] = ip ^ degree ^ prefetch_offset ^ ip_history;
    // pre_hash[7] = dram_bucket;
    // pre_hash[8] = ip ^ dram_bucket;
    // pre_hash[9] = degree ^ dram_bucket;
    // pre_hash[10] = ip ^ degree & dram_bucket;
    // pre_hash[11] = pollution_bucket;
    // pre_hash[12] = ip ^ pollution_bucket;
    // pre_hash[13] = degree ^ pollution_bucket;
    // pre_hash[14] = ip ^ degree ^ pollution_bucket;
    // pre_hash[15] = lateness_bucket;
    // pre_hash[16] = ip ^ lateness_bucket;
    // pre_hash[17] = degree ^ lateness_bucket;
    // pre_hash[18] = ip ^ degree ^ lateness_bucket;
    // pre_hash[19] = ip_accuracy_bucket;
    // pre_hash[20] = ip ^ ip_accuracy_bucket;
    // pre_hash[21] = degree_accuracy_bucket;
    // pre_hash[22] = degree ^ degree_accuracy_bucket;
    // pre_hash[23] = PAGE(base_addr);
    // pre_hash[24] = CACHE_LINE(base_addr);
    // pre_hash[25] = (int) dram_availability*100;
    // pre_hash[26] = (int) ip_accuracy*100;
    // pre_hash[27] = (int) lateness*100;
    // pre_hash[28] = (int) pollution*100;
    // pre_hash[29] = (int) degree_accuracy*100;
    // pre_hash[30] = ip ^ (int) dram_availability*100;
    // pre_hash[31] = ip ^ (int) ip_accuracy*100;
    // pre_hash[32] = ip ^ (int) lateness*100;
    // pre_hash[33] = ip ^ (int) pollution*100;
    // pre_hash[34] = ip ^ (int) degree_accuracy*100;
    // pre_hash[35] = ip ^ degree ^ (int) dram_availability*100;
    // pre_hash[36] = ip ^ degree ^ (int) ip_accuracy*100;
    // pre_hash[37] = ip ^ degree ^ (int) lateness*100;
    // pre_hash[38] = ip ^ degree ^ (int) pollution*100;
    // pre_hash[39] = ip ^ degree ^ (int) degree_accuracy*100;
    // pre_hash[40] = ip_history ^ (int) dram_availability*100;
    // pre_hash[41] = ip_history ^ (int) ip_accuracy*100;
    // pre_hash[42] = ip_history ^ (int) lateness*100;
    // pre_hash[43] = ip_history ^ (int) pollution*100;
    // pre_hash[44] = ip_history ^ (int) degree_accuracy*100;
    // pre_hash[45] = dram_occupancy;
    // pre_hash[46] = ip ^ dram_occupancy;
    // pre_hash[47] = ip ^ degree ^ dram_occupancy;
    // pre_hash[48] = degree ^ dram_occupancy;
    // pre_hash[49] = ip_history ^ dram_occupancy;



    for (int i = 0; i < NUM_TEST_FEATURES; i++) {
        perc_set[i] = pre_hash[i] % FEATURE_DEPTH;
    }
}

void Prefetch_Feature_Filter::get_indices(uint64_t base_addr, uint64_t ip, uint64_t ip_1, uint64_t ip_2, uint64_t ip_3, int32_t prefetch_offset, uint32_t degree, 
double dram_availability, int dram_occupancy, uint64_t perc_set[NUM_TEST_FEATURES], int* dram_bucket, int* pollution_bucket, int* ip_accuracy_bucket, int* lateness_bucket, int* degree_accuracy_bucket,
    double* ip_accuracy, double* lateness, double* pollution, double* degree_accuracy) {
    uint64_t  pre_hash[NUM_TEST_FEATURES];

    // get features
    uint64_t ip_history = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
    *ip_accuracy = (double) ip_accuracy_map[ip].first / (double) ip_accuracy_map[ip].second;
    double ip_lateness = (double) ip_late_map[ip] / (double) ip_accuracy_map[ip].first;
    double ip_pollution = (double) ip_pollution_map[ip] / (double) ip_demand_miss_interval_map[ip];
    *lateness = (double) late_prefetch_interval_counter / (double) useful_prefetch_interval_counter;
    *pollution = (double) pollution_interval_counter / (double) demand_miss_interval_counter;
    *degree_accuracy = (double) degree_accuracy_map[degree].first / (double) degree_accuracy_map[degree].second;

    // get buckets for stat features
    *dram_bucket = floor(dram_availability * 100.0);
    *lateness_bucket = floor(*lateness * 100.0);
    *pollution_bucket = floor(*pollution * 100.0);
    *ip_accuracy_bucket = floor(*ip_accuracy * 100.0);
    int ip_pollution_bucket = floor(ip_pollution * 100.0);
    *degree_accuracy_bucket = floor(*degree_accuracy * 100.0);

    if (FEATURE_TEST_LOGGING) {
        cout << "ip: " << ip << endl;
        cout << "ip_accuracy: " << ip_accuracy << endl;
        cout << "ip_lateness: " << ip_lateness << endl;
        cout << "ip_pollution: " << ip_pollution << endl;
        cout << "lateness: " << lateness << endl;
        cout << "pollution: " << pollution << endl;
        cout << "dram availability: " << dram_availability << endl;
        cout << "ip_1: " << ip_1 << endl;
        cout << "ip_2: " << ip_2 << endl;
        cout << "ip_3: " << ip_3 << endl;
        cout << "ip_accuracy_bucket: " << *ip_accuracy_bucket << endl;
    } 

    // get pre-hash indices
    // pre_hash[0] = ip ^ degree ^ (prefetch_offset + 40);
    // pre_hash[1] = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
    // pre_hash[1] = degree;
    // pre_hash[2] = prefetch_offset + 40;
    // pre_hash[3] = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);

    pre_hash[0] = ip;
    pre_hash[1] = degree;
    pre_hash[2] = prefetch_offset + 40;
    pre_hash[3] = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
    // pre_hash[3] = ip_1;
    // pre_hash[4] = ip_2;
    // pre_hash[5] = ip_3;
    // pre_hash[4] = dram_occupancy;
    // pre_hash[5] = *pollution_bucket;
    // pre_hash[6] = *degree_accuracy_bucket;
    // pre_hash[7] = *ip_accuracy_bucket;
    // pre_hash[8] = *lateness_bucket;
    // pre_hash[0] = base_addr;
    // pre_hash[1] = CACHE_LINE(base_addr);
    // pre_hash[2] = PAGE(base_addr);
    // pre_hash[3] = ip ^ degree;
    // pre_hash[4] = ip ^ (prefetch_offset + 40);
    // pre_hash[5] = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);





    // pre_hash[0] = ip;
    // pre_hash[1] = degree;
    // pre_hash[2] = prefetch_offset + 40;
    // pre_hash[3] = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
    // pre_hash[4] = *dram_bucket;
    // pre_hash[5] = *pollution_bucket;
    // pre_hash[6] = *degree_accuracy_bucket;
    // pre_hash[7] = *ip_accuracy_bucket;
    // pre_hash[8] = *lateness_bucket;
    // pre_hash[9] = PAGE(base_addr);
    // pre_hash[10] = CACHE_LINE(base_addr);
    // pre_hash[11] = dram_availability*100;
    // pre_hash[12] = (int) *ip_accuracy*100;
    // pre_hash[13] = (int) *lateness*100;
    // pre_hash[14] = (int) *pollution*100;
    // pre_hash[15] = (int) *degree_accuracy*100;
    // pre_hash[16] = dram_occupancy;
    // pre_hash[17] = ip ^ degree;
    // pre_hash[18] = ip ^ prefetch_offset;
    // pre_hash[19] = ip ^ degree ^ prefetch_offset;
    // pre_hash[20] = ip ^ degree ^ prefetch_offset ^ ip_history;
    // pre_hash[21] = ip_1;
    // pre_hash[22] = ip_2;
    // pre_hash[23] = ip_3;
    
    // pre_hash[0] = base_addr;
    // pre_hash[1] = CACHE_LINE(base_addr);
    // pre_hash[2] = PAGE(base_addr);
    // pre_hash[3] =
    // pre_hash[4] = ip ^ degree;
    // pre_hash[5] = ip ^ prefetch_offset;

    // pre_hash[0] = ip;
    // pre_hash[1] = ip ^ degree;
    // pre_hash[2] = ip ^ degree ^ ip_history;
    // pre_hash[3] = ip ^ prefetch_offset;
    // pre_hash[4] = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
    // pre_hash[5] = ip ^ degree ^ prefetch_offset;
    // pre_hash[6] = ip ^ degree ^ prefetch_offset ^ ip_history;
    // pre_hash[7] = *dram_bucket;
    // pre_hash[8] = ip ^ *dram_bucket;
    // pre_hash[9] = degree ^ *dram_bucket;
    // pre_hash[10] = ip ^ degree ^ *dram_bucket;
    // pre_hash[11] = *pollution_bucket;
    // pre_hash[12] = ip ^ *pollution_bucket;
    // pre_hash[13] = degree ^ *pollution_bucket;
    // pre_hash[14] = ip ^ degree ^ *pollution_bucket;
    // pre_hash[15] = *lateness_bucket;
    // pre_hash[16] = ip ^ *lateness_bucket;
    // pre_hash[17] = degree ^ *lateness_bucket;
    // pre_hash[18] = ip ^ degree ^ *lateness_bucket;
    // pre_hash[19] = *ip_accuracy_bucket;
    // pre_hash[20] = ip ^ *ip_accuracy_bucket;
    // pre_hash[21] = *degree_accuracy_bucket;
    // pre_hash[22] = degree ^ *degree_accuracy_bucket;
    // pre_hash[23] = PAGE(base_addr);
    // pre_hash[24] = CACHE_LINE(base_addr);
    // pre_hash[25] = dram_availability*100;
    // pre_hash[26] = (int) *ip_accuracy*100;
    // pre_hash[27] = (int) *lateness*100;
    // pre_hash[28] = (int) *pollution*100;
    // pre_hash[29] = (int) *degree_accuracy*100;
    // pre_hash[30] = ip ^ (int) dram_availability*100;
    // pre_hash[31] = ip ^ (int) *ip_accuracy*100;
    // pre_hash[32] = ip ^ (int) *lateness*100;
    // pre_hash[33] = ip ^ (int) *pollution*100;
    // pre_hash[34] = ip ^ (int) *degree_accuracy*100;
    // pre_hash[35] = ip ^ degree ^ (int) dram_availability*100;
    // pre_hash[36] = ip ^ degree ^ (int) *ip_accuracy*100;
    // pre_hash[37] = ip ^ degree ^ (int) *lateness*100;
    // pre_hash[38] = ip ^ degree ^ (int) *pollution*100;
    // pre_hash[39] = ip ^ degree ^ (int) *degree_accuracy*100;
    // pre_hash[40] = ip_history ^ (int) dram_availability*100;
    // pre_hash[41] = ip_history ^ (int) *ip_accuracy*100;
    // pre_hash[42] = ip_history ^ (int) *lateness*100;
    // pre_hash[43] = ip_history ^ (int) *pollution*100;
    // pre_hash[44] = ip_history ^ (int) *degree_accuracy*100;
    // pre_hash[45] = dram_occupancy;
    // pre_hash[46] = ip ^ dram_occupancy;
    // pre_hash[47] = ip ^ degree ^ dram_occupancy;
    // pre_hash[48] = degree ^ dram_occupancy;
    // pre_hash[49] = ip_history ^ dram_occupancy;

    for (int i = 0; i < NUM_TEST_FEATURES; i++) {
        perc_set[i] = pre_hash[i] % FEATURE_DEPTH;
    }
}

bool Prefetch_Feature_Filter::get_prediction(uint64_t ip, uint64_t pf_addr, int degree, int rrpv, uint64_t set, uint64_t prefetch_trigger_addr, int prefetch_offset, double dram_availability, int dram_occupancy, bool harmony_pred, uint64_t score, bool miss, vector<uint64_t> history) {
    uint64_t  perc_set[NUM_TEST_FEATURES];
    uint64_t ip_1 = history.size() > 0 ? history[0] : 0;
    uint64_t ip_2 = history.size() > 1 ? history[1] : 0;
    uint64_t ip_3 = history.size() > 2 ? history[2] : 0;
    int dram_bucket, pollution_bucket, lateness_bucket, ip_accuracy_bucket, degree_accuracy_bucket = 0;
    double ip_accuracy, pollution, lateness, degree_accuracy = 0.0;
    get_indices(prefetch_trigger_addr, ip, ip_1, ip_2, ip_3, prefetch_offset, degree, dram_availability, dram_occupancy, perc_set, 
                &dram_bucket, &pollution_bucket, &ip_accuracy_bucket, &lateness_bucket, &degree_accuracy_bucket,
                &ip_accuracy, &lateness, &pollution, &degree_accuracy);

    int sum = 0;
    for (int i = 0; i < NUM_TEST_FEATURES; i++) {
        std::set<uint64_t> s;
        s.insert(perc_set[i]);
      sum += features[i].predict(s);
      // Calculate Sum
    }
    if (FEATURE_TEST_LOGGING) {
        cout << "sum: " << sum << endl;
    }
    bool result = (sum > 0);
    if (FEATURE_TEST_VECTOR_TRAINING) {
        if (!miss && prev_results_vector.find(pf_addr) != prev_results_vector.end()) {
            prev_results_vector[pf_addr].count++;
            prev_results_vector[pf_addr].ip_vector.push_back(ip);
            prev_results_vector[pf_addr].trigger_addr_vector.push_back(prefetch_trigger_addr);
            prev_results_vector[pf_addr].prefetch_offset_vector.push_back(prefetch_offset);
            prev_results_vector[pf_addr].degree_vector.push_back(degree);
            prev_results_vector[pf_addr].history_vector.push_back(history);
            prev_results_vector[pf_addr].dram_occupancy_vector.push_back(dram_occupancy);
            prev_results_vector[pf_addr].degree_accuracy_bucket_vector.push_back(degree_accuracy_bucket);
            prev_results_vector[pf_addr].dram_bucket_vector.push_back(dram_bucket);
            prev_results_vector[pf_addr].lateness_bucket_vector.push_back(lateness_bucket);
            prev_results_vector[pf_addr].pollution_bucket_vector.push_back(pollution_bucket);
            prev_results_vector[pf_addr].ip_accuracy_bucket_vector.push_back(ip_accuracy_bucket);
            prev_results_vector[pf_addr].result_vector.push_back(true);
        }
        else if (miss) {
            std::vector<uint64_t> ip_vector;
            std::vector<int> degree_vector;
            std::vector<int> prefetch_offset_vector;
            std::vector<std::vector<uint64_t>> history_vector;
            vector<uint64_t> trigger_addr_vector;
            std::vector<bool> result_vector;
            std::vector<int> dram_bucket_vector;
            vector<int> pollution_bucket_vector;
            vector<int> lateness_bucket_vector;
            vector<int> ip_accuracy_bucket_vector;
            vector<int> degree_accuracy_bucket_vector;
            std::vector<int> dram_occupancy_vector;
            ip_vector.push_back(ip);
            degree_vector.push_back(degree);
            prefetch_offset_vector.push_back(prefetch_offset);
            history_vector.push_back(history);
            trigger_addr_vector.push_back(prefetch_trigger_addr);
            dram_bucket_vector.push_back(dram_bucket);
            ip_accuracy_bucket_vector.push_back(ip_accuracy_bucket);
            degree_accuracy_bucket_vector.push_back(degree_accuracy_bucket);
            pollution_bucket_vector.push_back(pollution_bucket);
            lateness_bucket_vector.push_back(lateness_bucket);
            result_vector.push_back(true);
            dram_occupancy_vector.push_back(dram_occupancy);
            prev_results_vector[pf_addr] = {ip_vector, history_vector, trigger_addr_vector, degree_vector, prefetch_offset_vector, dram_bucket_vector, pollution_bucket_vector, lateness_bucket_vector, 
                                                ip_accuracy_bucket_vector, degree_accuracy_bucket_vector, dram_occupancy_vector, result_vector, 1};
        }
    } else {
        prev_results[pf_addr] = {ip, ip_1, ip_2, ip_3, prefetch_trigger_addr, degree, prefetch_offset, dram_bucket, dram_occupancy, ip_accuracy_bucket, degree_accuracy_bucket, lateness_bucket, pollution_bucket, result,
                                dram_availability, ip_accuracy, pollution, lateness, degree_accuracy};
    }
    total_predictions++;
    
    if (result) {
        return true;
    }
    int hash = pf_addr % 4096;
    direct_mapped_reject_table[hash] = pf_addr;
    return false;
}
void Prefetch_Feature_Filter::train_vector(uint64_t pf_addr, bool result, bool reject_cache, bool useful) {
    vector<uint64_t> ip_vector;
    vector<int> degree_vector;
    vector<int> prefetch_offset_vector;
    vector<vector<uint64_t>> history_vector;
    vector<bool> result_vector;
    vector<int> dram_occupancy_vector;
    int count = 0;

    if (prev_results_vector.find(pf_addr) != prev_results_vector.end()) {
        ip_vector = prev_results_vector[pf_addr].ip_vector;
        degree_vector = prev_results_vector[pf_addr].degree_vector;
        prefetch_offset_vector = prev_results_vector[pf_addr].prefetch_offset_vector;
        history_vector = prev_results_vector[pf_addr].history_vector;
        result_vector = prev_results_vector[pf_addr].result_vector;
        dram_occupancy_vector = prev_results_vector[pf_addr].dram_occupancy_vector;
        count = prev_results_vector[pf_addr].count;
    }

    if (FEATURE_TEST_LOGGING) {
        cout << "result: " << result << endl; 
        cout << "count : " << count << endl;
        cout << "ip vector" << endl;
        for (int i = 0; i < count; i++) {
            cout << ip_vector[i] << ", ";
        }
        cout << endl;
        cout << "degree vector" << endl;
        for (int i = 0; i < count; i++) {
            cout << degree_vector[i] << ", ";
        }
        cout << endl;
        cout << "prefetch_offset vector" << endl;
        for (int i = 0; i < count; i++) {
            cout << prefetch_offset_vector[i] << ", ";
        }
        cout << endl;
        cout << "history vector" << endl;
        for (int i = 0; i < count; i++) {
            for (int j = 0; j < history_vector[i].size(); j++) {
                cout << history_vector[i][j] << ", ";
            }
        }
        cout << endl;
        cout << "dram occupancy vector" << endl;
        for (int i = 0; i < count; i++) {
            cout << dram_occupancy_vector[i] << ", ";
        }
        cout << endl;
        cout << "result vector" << endl;
        for (int i = 0; i < count; i++) {
            cout << result_vector[i] << ", ";
        }
        cout << endl;
    }

    for (int i = 0; i < count; i++) {
        uint64_t  perc_set[NUM_TEST_FEATURES];
        uint64_t ip_1 = prev_results_vector[pf_addr].history_vector[i].size() > 0 ? prev_results_vector[pf_addr].history_vector[i][0] : 0;
        uint64_t ip_2 = prev_results_vector[pf_addr].history_vector[i].size() > 1 ? prev_results_vector[pf_addr].history_vector[i][1] : 0;
        uint64_t ip_3 = prev_results_vector[pf_addr].history_vector[i].size() > 2 ? prev_results_vector[pf_addr].history_vector[i][2] : 0;
        get_indices_training(prev_results_vector[pf_addr].trigger_addr_vector[i], prev_results_vector[pf_addr].ip_vector[i], ip_1, ip_2, ip_3, 
        prev_results_vector[pf_addr].prefetch_offset_vector[i], prev_results_vector[pf_addr].degree_vector[i], prev_results_vector[pf_addr].dram_bucket_vector[i], prev_results_vector[pf_addr].pollution_bucket_vector[i], prev_results_vector[pf_addr].ip_accuracy_bucket_vector[i], 
        prev_results_vector[pf_addr].lateness_bucket_vector[i], prev_results_vector[pf_addr].degree_accuracy_bucket_vector[i], 0.0, prev_results_vector[pf_addr].dram_occupancy_vector[i], 0.0, 0.0,
        0.0, 0.0, perc_set);

        bool prev_result = prev_results_vector[pf_addr].result_vector[i];
        int sum = 0;
        int coeff = result ? 1 : -1;

        for (int i = 0; i < NUM_TEST_FEATURES; i++) {
            std::set<uint64_t> s;
            s.insert(perc_set[i]);
            int pred = features[i].predict(s);
            sum += pred;
            int pred_result = (pred >= 0);
            // uint64_t threshold = 1 << 31;
            // if (pred_result != result || std::abs(pred_result) < threshold) {
            //     features[i].update(prev_results[pf_addr].ip, coeff, s);
            // }
            if (result && pred_result == result) {
                features[i].positive_count++;
            }
            features[i].total_count++;
        // Calculate Sum
        }
        // training
        bool sum_result = (sum >= 0);
        uint64_t threshold = 1 << 31;
        for (int i = 0; i < NUM_TEST_FEATURES; i++) {
            
            if (sum_result != result || std::abs(sum) < training_threshold) {
                std::set<uint64_t> s;
                s.insert(perc_set[i]);
                features[i].update(prev_results_vector[pf_addr].ip_vector[i], coeff, s);
            }
        }
        degree_accuracy_map[prev_results_vector[pf_addr].degree_vector[i]].second++;

        if (i == 0) {
            if (result && prev_result == result) {
            useful_correct++;
            }
            else if (!result && prev_result == result) {
                useless_correct++;
            }
            else if (result && prev_result != result) {
                useful_wrong++;
            }
            else if (!result && prev_result != result) {
                useless_wrong++;
            }

            total_training++;
        }

        if (result_vector[i] == result) {
            accurate_predictions_count++;
        }
        total_predictions_count++;
        

        if (useful) {
            // ip_degree_accuracy_map[ip][degree].first++;
            degree_accuracy_map[prev_results_vector[pf_addr].degree_vector[i]].first++;
        }
        }
        prev_results_vector.erase(pf_addr);
}

void Prefetch_Feature_Filter::train(uint64_t pf_addr, bool result, bool reject_cache, bool useful) {
    if (FEATURE_TEST_VECTOR_TRAINING) {
        train_vector(pf_addr, result, reject_cache, useful);
        return;
    }
    uint64_t  perc_set[NUM_TEST_FEATURES];
    get_indices_training(prev_results[pf_addr].base_addr, prev_results[pf_addr].ip, prev_results[pf_addr].ip_1, prev_results[pf_addr].ip_2, prev_results[pf_addr].ip_3, 
    prev_results[pf_addr].prefetch_offset, prev_results[pf_addr].degree, prev_results[pf_addr].dram_bucket, prev_results[pf_addr].pollution_bucket, prev_results[pf_addr].ip_accuracy_bucket, 
    prev_results[pf_addr].lateness_bucket, prev_results[pf_addr].degree_accuracy_bucket, prev_results[pf_addr].dram_availability, prev_results[pf_addr].dram_occupancy, prev_results[pf_addr].ip_accuracy, prev_results[pf_addr].lateness,
    prev_results[pf_addr].pollution, prev_results[pf_addr].degree_accuracy, perc_set);
    bool prev_result = prev_results[pf_addr].prev_result;
    int sum = 0;
    int coeff = result ? 1 : -1;
    

    for (int i = 0; i < NUM_TEST_FEATURES; i++) {
        std::set<uint64_t> s;
        s.insert(perc_set[i]);
        int pred = features[i].predict(s);
        sum += pred;
        int pred_result = (pred >= 0);
        // uint64_t threshold = 1 << 31;
        // if (pred_result != result || std::abs(pred_result) < threshold) {
        //     features[i].update(prev_results[pf_addr].ip, coeff, s);
        // }
        if (result && pred_result == result) {
            features[i].positive_count++;
        }
        features[i].total_count++;
      // Calculate Sum
    }
    // training
    bool sum_result = (sum >= 0);
    uint64_t threshold = 1 << 31;
    for (int i = 0; i < NUM_TEST_FEATURES; i++) {
        
        if (prev_result != result || std::abs(sum) < training_threshold) {
            std::set<uint64_t> s;
            s.insert(perc_set[i]);
            features[i].update(prev_results[pf_addr].ip, coeff, s);
        }
    }
    degree_accuracy_map[prev_results[pf_addr].degree].second++;

    if (result && prev_result == result) {
        useful_correct++;
    }
    else if (!result && prev_result == result) {
        useless_correct++;
    }
    else if (result && prev_result != result) {
        useful_wrong++;
    }
    else if (!result && prev_result != result) {
        useless_wrong++;
    }

    total_training++;

    if (useful) {
        // ip_degree_accuracy_map[ip][degree].first++;
        degree_accuracy_map[prev_results[pf_addr].degree].first++;
    }
    prev_results.erase(pf_addr);
}

bool Prefetch_Feature_Filter::is_interval_complete(void) {
    return (num_useful_blocks_evicted >= num_useful_blocks_evicted_threshold);
}

int Prefetch_Feature_Filter::reset_interval(uint64_t current_cycle, int current_degree) {
    num_useful_blocks_evicted = 0;

    double pollution = (double) pollution_interval_counter / (double) demand_miss_interval_counter;
    double accuracy = 0.0;
    double dram_availability = 0.0;
    double lateness = 0.0;
    int num_demand_miss_interval_counter = demand_miss_interval_counter;
    int num_prefetch_miss_interval_counter = prefetch_miss_interval_counter;
    double demand_miss_latency_avg = (double) demand_miss_latency_interval_counter / (double) demand_miss_interval_counter;
    double prefetch_miss_latency_avg = (double) prefetch_miss_latency_interval_counter / (double) prefetch_miss_interval_counter;

    if (degree_accuracy_map[current_degree].second > 0) {
        accuracy = (double) degree_accuracy_map[current_degree].first / (double) degree_accuracy_map[current_degree].second;
    }
    if (prefetch_miss_interval_counter > 0) {
        dram_availability = (double) dram_availability_at_prefetch_issue_interval / (double) prefetch_miss_interval_counter / (double) 64.0;
    }
    if (useful_prefetch_interval_counter > 0) {
        lateness = (double) late_prefetch_interval_counter / (double) useful_prefetch_interval_counter;
    }

    // cout << "degree pollution counter: " << degree_pollution_map[current_degree] << endl;
    // cout << "degree pollution counter: " << degree_pollution_map[3] << endl;
    // cout << "degree pollution counter: " << degree_pollution_map[2] << endl;
    // cout << "degree pollution counter: " << degree_pollution_map[1] << endl;
    cout << "pollution_interval_counter: " << pollution_interval_counter << endl;
    cout << "demand_miss_interval_counter: " << demand_miss_interval_counter << endl;
    cout << "prefetch_miss_interval_counter: " << prefetch_miss_interval_counter << endl;
    cout << "dram availability: " << dram_availability << endl;
    cout << "late prefetch interval counter: " << late_prefetch_interval_counter << endl;
    cout << "useful prefetch interval counter: " << useful_prefetch_interval_counter << endl;
    cout << "current_demand_miss_latency_avg: " << current_demand_miss_latency_avg << endl;
    cout << "demand_miss_latency_avg: " << demand_miss_latency_avg << endl;
    cout << "prefetch_miss_latency_avg: " << prefetch_miss_latency_avg << endl;

    cout << "pf filter pollution: " << pollution << endl;
    cout << "pf filter degree accuracy: " << accuracy << endl;
    cout << "pf filter lateness: " << lateness << endl;

    interval_start = current_cycle;
    demand_miss_interval_counter = 0;
    prefetch_miss_interval_counter = 0;
    prefetch_miss_latency_interval_counter = 0;
    demand_miss_latency_interval_counter = 0;
    pollution_interval_counter = 0;
    dram_availability_at_prefetch_issue_interval = 0;
    useful_prefetch_interval_counter = 0;
    degree_pollution_map.clear();
    late_prefetch_interval_counter = 0;

    

    // print_ip_degree_accuracy_map();


    if (FEATURE_TEST_LOGGING) {
        // print_ip_accuracy_map();
        print_ip_degree_accuracy_map();
        for (auto& item : ip_pollution_map) {
            cout << "ip: " << item.first;
            cout << " pollution: " << item.second;
            cout << " demand miss total: " << demand_miss_total;
            cout << " pollution contribution: " << (double) item.second / (double) demand_miss_total << endl;
        }
    }
    return current_degree;
}

void Prefetch_Feature_Filter::print_ip_accuracy_map(void) {
    for (auto& item : ip_accuracy_map) {
        std::cout << "ip: " << item.first;
        int accurate = item.second.first;
        int total = item.second.second;

        std::cout << " accurate: " << accurate << " total: " << total << " late: " << ip_late_map[item.first];

        if (total > 0) {
            double lateness =  (double) ip_late_map[item.first] / (double) accurate;
            std::cout << " accuracy: " << (double) accurate/(double)total << " lateness: " << (double) ip_late_map[item.first] / (double) accurate << std::endl;
            if (lateness > 1.0) {
                cout << "lateness is greater than 100 percent error!" << endl;
            }
        }
        else {
            std::cout << " accuracy: " << "nan" << std::endl;
        }
    }
}

void Prefetch_Feature_Filter::print_ip_degree_accuracy_map(void) {
    for (auto& item : ip_degree_accuracy_map) {
        std::cout << "ip: " << item.first << endl;

        for (auto& degree : item.second) {
            cout << "    degree: " << degree.first;
            int accurate = degree.second.first;
            int total = degree.second.second;

            std::cout << " accurate: " << accurate << " total: " << total << " late: " << ip_late_map[degree.first];

            if (total > 0) {
                double lateness =  (double) ip_late_map[degree.first] / (double) accurate;
                std::cout << " accuracy: " << (double) accurate/(double)total << " lateness: " << (double) ip_late_map[degree.first] / (double) accurate << " coverage: " << (double) accurate / (double) demand_miss_total << std::endl;
                if (lateness > 1.0) {
                    cout << "lateness is greater than 100 percent error!" << endl;
                }
            }
            else {
                std::cout << " accuracy: " << "nan" << std::endl;
            }
        }
    }
}

void Prefetch_Feature_Filter::print_degree_accuracy_map(void) {
    for (auto& item : degree_accuracy_map) {
        std::cout << "degree: " << item.first << endl;

        int accurate = item.second.first;
        int total = item.second.second;

        std::cout << " accurate: " << accurate << " total: " << total << " late: " << ip_late_map[item.first];

        if (total > 0) {
            double lateness =  (double) ip_late_map[item.first] / (double) accurate;
            std::cout << " accuracy: " << (double) accurate/(double)total << " lateness: " << (double) ip_late_map[item.first] / (double) accurate << " coverage: " << (double) accurate / (double) demand_miss_total << std::endl;
            if (lateness > 1.0) {
                cout << "lateness is greater than 100 percent error!" << endl;
            }
        }
        else {
            std::cout << " accuracy: " << "nan" << std::endl;
        }
    }
}

void Prefetch_Feature_Filter::verify_stats(uint64_t useful_prefetch_total, uint64_t prefetch_fill_total, uint64_t late_total, uint64_t pollution_total) {
    cout << "verifying stats" << endl;
    uint64_t accurate_total_lhs = 0;
    uint64_t pf_total_lhs = 0;
    uint64_t late_total_lhs = 0;
    uint64_t pollution_total_lhs = 0;
    for (auto& item : ip_accuracy_map) {
        accurate_total_lhs += item.second.first;
        pf_total_lhs += item.second.second;
    }

    for (auto& item : ip_late_map) {
        late_total_lhs += item.second;
    }

    for (auto& item: ip_pollution_map) {
        pollution_total_lhs += item.second;
    }

    if (accurate_total_lhs != useful_prefetch_total) {
        cout << "useful prefetch total difference!" << endl;
        cout << "pf_filter: " << accurate_total_lhs << " tracer: " << useful_prefetch_total << endl;
    }
    if (pf_total_lhs != prefetch_fill_total) {
        cout << "prefetch fill total difference!" << endl;
        cout << "pf_filter: " << pf_total_lhs << " tracer: " << prefetch_fill_total << endl;
    }
    if (late_total_lhs != late_total) {
        cout << "late prefetch total difference!" << endl;
        cout << "pf_filter: " << late_total_lhs << " tracer: " << late_total << endl;
    }
    if (pollution_total_lhs != pollution_total) {
        cout << "pollution total difference!" << endl;
        cout << "pf_filter: " << pollution_total_lhs << " tracer: " << pollution_total << endl;
    }
}

void Prefetch_Feature_Filter::print_perceptrons(void) {
    for (int i = 0; i < NUM_TEST_FEATURES; i++) {
        cout << "feature: " << i << endl;
        features[i].print_weights();
        cout << "accuracy to ground truth: " << (double) features[i].positive_count / (double) features[i].total_count << endl;
    }
    // print_ip_degree_accuracy_map();
    print_degree_accuracy_map();

     uint64_t useless_correct_temp = useless_correct + (total_predictions - total_training);
    cout << "filter accuracy: " << (double) accurate_predictions_count / (double) total_predictions_count << endl;
    cout << "useful_correct: " << useful_correct << endl;
    cout << "useful_wrong: " << useful_wrong << endl;
    cout << "useless_correct: " << useless_correct_temp << endl;
    cout << "useless_wrong: " << useless_wrong << endl;
    cout << "total_predictions: " << total_predictions << endl;

    cout << "useful_correct_percent: " << (double) useful_correct / (double) total_predictions << endl;
    cout << "useful_wrong_percent: " << (double) useful_wrong / (double) total_predictions << endl;
    cout << "useless_correct_percent: " << (double) useless_correct_temp / (double) total_predictions << endl;
    cout << "useless_wrong_percent: " << (double) useless_wrong / (double) total_predictions << endl;
}

// bool Prefetch_Feature_Filter::check_direct_mapped_reject_table(uint64_t addr) {
//     int hash = addr % 4096;
//     if (direct_mapped_reject_table[hash] > 0) {
//         return true;
//     }
//     return false;
// }

// void Prefetch_Feature_Filter::erase_direct_mapped_reject_table(uint64_t addr) {
//     int hash = addr % 4096;
//     direct_mapped_reject_table[hash] = 0;
// }

bool Prefetch_Feature_Filter::check_direct_mapped_reject_table(uint64_t addr) {
    int hash = addr % 4096;
    if (direct_mapped_reject_table[hash] > 0) {
        return true;
    }
    return false;
}

void Prefetch_Feature_Filter::erase_direct_mapped_reject_table(uint64_t addr) {
    int hash = addr % 4096;
    direct_mapped_reject_table[hash] = 0;
}