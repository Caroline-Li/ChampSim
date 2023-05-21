#include "prefetch_ip_suppress_filter.h"
#include "champsim_constants.h"
#include <math.h>
using namespace std;

// #define SPP_DEBUG_PRINT false

// //#define SPP_DEBUG_PRINT
// #ifdef SPP_DEBUG_PRINT
// #define SPP_DP(x) x
// #else
// #define SPP_DP(x)
// #endif

// //#define SPP_PERC_WGHT
// #ifdef SPP_PERC_WGHT
// #define SPP_PW(x) x
// #else 
// #define SPP_PW(x)
// #endif

// // Prefetch filter parameters
// #define QUOTIENT_BIT  10
// #define REMAINDER_BIT 6
// #define HASH_BIT (QUOTIENT_BIT + REMAINDER_BIT + 1)
// #define FILTER_SET (1 << QUOTIENT_BIT)

// #define QUOTIENT_BIT_REJ  10
// #define REMAINDER_BIT_REJ 8
// #define HASH_BIT_REJ (QUOTIENT_BIT_REJ + REMAINDER_BIT_REJ + 1)
// #define FILTER_SET_REJ (1 << QUOTIENT_BIT_REJ)

#define CACHE_LINE(a) (a >> LOG2_BLOCK_SIZE)

#define PAGE(a) (a >> LOG2_PAGE_SIZE)

#define FEATURE_DEPTH 8192
// #define NOFFSET 46
// extern int OFFSET[NOFFSET];

Prefetch_Filter::Prefetch_Filter() {
    if (PERCEPTRON_PCA) {
        pca_out.open("pca.csv");
        pca_out << "IP, Degree, Prefetch Offset, History IP_1, History IP_2, History IP_3, Dram Bank Occupancy, Pollution, Result" << endl;
    }
    cout << "NUM_BEST_FEATURES: " << NUM_BEST_FEATURES << endl;
    // for (int i = 0; i < 4; i++) {
    //     low_degree_accuracy.push_back(NONE);
    // }
}
void Prefetch_Filter::set_interval_start_cycle(uint64_t current_cycle) {
    interval_start = current_cycle;
}

uint64_t Prefetch_Filter::get_interval_start_cycle(void) {
    return interval_start;
}

bool Prefetch_Filter::get_prediction(uint64_t ip, int dram_occupancy) {
    std::array<double, NUM_FEATURES> features;
    if (ip_accuracy_map[ip].second > 0 && ip_accuracy_map[ip].first > 0 && demand_miss_total > 0) {
        // accuracy
        features[0] = (double) ip_accuracy_map[ip].first / (double) ip_accuracy_map[ip].second;
        // lateness
        features[1] = (double) ip_late_map[ip] / (double) ip_accuracy_map[ip].first;
        // pollution
        features[2] = (double) ip_pollution_map[ip] / (double) demand_miss_total;
        // dram occupancy
        features[3] = (double) dram_occupancy / 50.0;

        past_features[ip] = features;
        double output = perceptron_table[ip].predict(features);

        if (LOGGING) {
            cout << "accurate_total: " << ip_accuracy_map[ip].first << endl;
            cout << "prefetch fill total: " << ip_accuracy_map[ip].second << endl;
            cout << "late total: " << ip_late_map[ip] << endl; 
            cout << "accuracy: " << features[0] << endl;
            cout << "lateness: " << features[1] << endl;
            cout << "pollution: " << features[2] << endl;
            cout << "dram occupancy: " << features[3] << endl;

            perceptron_table[ip].print_weights();
            cout << "output: " << output << endl;
        }

        if (output > 0.0) {
        results[ip] = {true, features[0], features[1], features[2], 0.0, 0};
        if (LOGGING) {
            cout << "get prediction for ip: " << ip << " true" << endl;
        }
        return true;
        }
        if (LOGGING) {
            cout << "get prediction for ip: " << ip << " false" << endl;
        }
        results[ip] = {false, features[0], features[1], features[2], 0.0, 0};
        return false;
    }
    if (LOGGING) {
            cout << "get prediction for ip: " << ip << " true" << endl;
    }
    return true;
}

bool Prefetch_Filter::get_prediction(uint64_t ip, double dram_availability, uint64_t pf_addr, int degree, int prefetch_offset) {
    std::array<double, NUM_FEATURES> features;
    if (ip_accuracy_map[ip].second > 0 && ip_accuracy_map[ip].first > 0 && demand_miss_total > 0) {
        // accuracy
        features[0] = (double) ip_accuracy_map[ip].first / (double) ip_accuracy_map[ip].second;
        // lateness
        features[1] = (double) ip_late_map[ip] / (double) ip_accuracy_map[ip].first;
        // pollution
        features[2] = (double) ip_pollution_map[ip] / (double) demand_miss_total;
        // dram occupancy
        features[3] = (double) dram_availability;

        past_features[ip] = features;
        double output = perceptron_table[ip].predict(features);

        if (LOGGING) {
            cout << "accurate_total: " << ip_accuracy_map[ip].first << endl;
            cout << "prefetch fill total: " << ip_accuracy_map[ip].second << endl;
            cout << "late total: " << ip_late_map[ip] << endl; 
            cout << "accuracy: " << features[0] << endl;
            cout << "lateness: " << features[1] << endl;
            cout << "pollution: " << features[2] << endl;
            cout << "dram availability: " << features[3] << endl;

            perceptron_table[ip].print_weights();
            cout << "output: " << output << endl;
        }

        if (output > 0.0) {
        results[pf_addr] = {true, features[0], features[1], features[2], dram_availability, degree, prefetch_offset};
        if (LOGGING) {
            cout << "get prediction for ip: " << ip << " true" << endl;
        }
        return true;
        }
        if (LOGGING) {
            cout << "get prediction for ip: " << ip << " false" << endl;
        }
        results[pf_addr] = {false, features[0], features[1], features[2], dram_availability, degree, prefetch_offset};

        if (PERCEPTRON_REJECT_TABLE) {
            reject_table.insert(pf_addr);
        }
        else if (PERCEPTRON_DIRECT_MAPPED_REJECT_TABLE) {
            int hash = pf_addr % 8192;
            direct_mapped_reject_table[hash] = pf_addr;
        }
        // else if (PERCEPTRON_REJECT_CACHE) {
        //     if (reject_table_cache.find(set) == reject_table_cache.end()) {
        //         vector<Reject_Cache> v;
        //         reject_table_cache[set] = v;
        //     }
        //     reject_table_cache[set].push_back({max(rrpv-1, 0), pf_addr, false});
        // }
        return false;
    }
    if (LOGGING) {
            cout << "get prediction for ip: " << ip << " true" << endl;
    }
    return true;
}

bool Prefetch_Filter::get_prediction(uint64_t ip) {
    std::array<double, NUM_FEATURES> features;
    if (ip_accuracy_map[ip].second > 0 && ip_accuracy_map[ip].first > 0 && demand_miss_total > 0) {
        // accuracy
        features[0] = (double) ip_accuracy_map[ip].first / (double) ip_accuracy_map[ip].second;
        // lateness
        features[1] = (double) ip_late_map[ip] / (double) ip_accuracy_map[ip].first;
        // pollution
        features[2] = (double) ip_pollution_map[ip] / (double) demand_miss_total;

        past_features[ip] = features;
        double output = perceptron_table[ip].predict(features);

        if (LOGGING) {
            cout << "accurate_total: " << ip_accuracy_map[ip].first << endl;
            cout << "prefetch fill total: " << ip_accuracy_map[ip].second << endl;
            cout << "late total: " << ip_late_map[ip] << endl; 
            cout << "accuracy: " << features[0] << endl;
            cout << "lateness: " << features[1] << endl;
            cout << "pollution: " << features[2] << endl;

            perceptron_table[ip].print_weights();
            cout << "output: " << output << endl;
        }

        if (output > 0.0) {
        results[ip] = {true, features[0], features[1], features[2], 0.0};
        if (LOGGING) {
            cout << "get prediction for ip: " << ip << " true" << endl;
        }
        return true;
        }
        if (LOGGING) {
            cout << "get prediction for ip: " << ip << " false" << endl;
        }
        results[ip] = {false, features[0], features[1], features[2], 0.0};
        return false;
    }
    if (LOGGING) {
            cout << "get prediction for ip: " << ip << " true" << endl;
    }
    return true;
}

PPF_PRED Prefetch_Filter::get_prediction(uint64_t ip, uint64_t pf_addr, int degree, int rrpv, uint64_t set, uint64_t prefetch_trigger_addr, int prefetch_offset, int dram_availability, bool harmony_pred, uint64_t score, bool miss, bool bypass, bool warmup_complete, vector<uint64_t> history) {
    if (LOGGING) {
        cout << "get prediction for ip: " << ip << " degree: " << degree << " prefetch_trigger_addr: " << prefetch_trigger_addr << " prefetch_offset: " << prefetch_offset << endl;
        cout << "dram availability: " << dram_availability << endl;
    }
    int dram_availability_category = LOW_DRAM_AVAILABILITY;
    int accuracy_category = LOW_PREFETCHER_ACCURACY;
    int score_category = LOW_SCORE;

    if (dram_availability < low_dram_availability_threshold) {
        dram_availability_category = LOW_DRAM_AVAILABILITY;
    }
    else if (dram_availability >= low_dram_availability_threshold && dram_availability < mid_dram_availability_threshold) {
        dram_availability_category = MID_DRAM_AVAILABILITY;
    }
    else {
        dram_availability_category = HIGH_DRAM_AVAILABILITY;
    }


    if (score < low_score_threshold) {
        score_category = LOW_SCORE;
    }
    else if (score > low_score_threshold && score < mid_score_threshold) {
        score_category = MEDIUM_SCORE;
    }
    else {
        score_category = HIGH_SCORE;
    }

    //  ip accuracy
    double ip_accuracy = (double) ip_accuracy_map[ip].first / (double) ip_accuracy_map[ip].second;

    if (ip_accuracy < low_accuracy_threshold) {
        accuracy_category = LOW_PREFETCHER_ACCURACY;
    }
    else if (ip_accuracy >= low_accuracy_threshold && ip_accuracy < mid_accuracy_threshold) {
        accuracy_category = MID_PREFETCHER_ACCURACY;
    }
    else {
        accuracy_category = HIGH_PREFETCHER_ACCURACY;
    }


    // if (PERCEPTRON_COMBINED) {
    //     if (feature_perceptron_table.find(ip) == feature_perceptron_table.end()) {
    //         if (LOGGING) {
    //             cout << "not trained yet: get prediction for ip: " << ip << " true" << endl;
    //         }
    //         prev_results[pf_addr] = {true, degree, ip, prefetch_trigger_addr, prefetch_offset, dram_availability_category, accuracy_category, harmony_pred, score_category, history};
    //         return PREFETCH_L2;
    //     }
    // }
    

    // if (PERCEPTRON_DEGREE) {
    //     if (feature_degree_perceptron_table.find(ip) == feature_degree_perceptron_table.end()) {
    //         if (LOGGING) {
    //         cout << "not trained yet: get prediction for ip: " << ip << " true" << endl;
    //         }
    //         prev_results[pf_addr] = {true, degree, ip, prefetch_trigger_addr, prefetch_offset, dram_availability_category, accuracy_category, harmony_pred, score_category, history};
    //         return PREFETCH_L2;
    //     }
    //     else if (feature_degree_perceptron_table.find(ip) != feature_degree_perceptron_table.end() 
    //         && feature_degree_perceptron_table[ip].find(degree) == feature_degree_perceptron_table[ip].end()) {
    //             if (LOGGING) {
    //             cout << "not trained yet: get prediction for ip: " << ip << " true" << endl;
    //             }
    //             prev_results[pf_addr] = {true, degree, ip, prefetch_trigger_addr, prefetch_offset, dram_availability_category, accuracy_category, harmony_pred, score_category, history};
    //             return PREFETCH_L2;
    //         }
    // }

    // if (PERCEPTRON_COMBINED) {
    //     feature_perceptron_table[ip].total_count++;
    // }
    // else if (PERCEPTRON_DEGREE) {
    //     feature_degree_perceptron_table[ip][degree].total_count++;
    // }
    // else if (PERCEPTRON_GLOBAL_WEIGHTS) {
    //     weight_table.total_count++;
    // }



    if (LOGGING) {
        for (int i = 0; i < history.size(); i++) {
            cout << history[i] << ", ";
        }
        cout << endl;
        cout << "degree: " << degree << endl;
    }

    int pred = 0;

    if (PERCEPTRON_COMBINED) {
        std::set<uint64_t> s(history.begin(), history.end());
        // s.insert(degree);
        // s.insert(prefetch_offset);
        // s.insert(PAGE(pf_addr));
        pred = feature_perceptron_table[ip].predict(s);
        if (LOGGING) {
            cout << "pred: " << pred << endl;
            feature_perceptron_table[ip].print_weights();
        }
    }
    else if (PERCEPTRON_DEGREE) {
        std::set<uint64_t> s(history.begin(), history.end());
        // s.insert(PAGE(prefetch_trigger_addr));
        // s.insert(prefetch_offset);
        // s.insert(dram_availability_category);
        // s.insert(accuracy_category);
        // s.insert(pf_addr);
        pred = feature_degree_perceptron_table[ip][degree].predict(s);
        if (LOGGING) {
            cout << "pred: " << pred << endl;
            feature_degree_perceptron_table[ip][degree].print_weights();
        }
    }
    else if (PERCEPTRON_DEGREE_PREFETCH_OFFSET) {
        std::set<uint64_t> s(history.begin(), history.end());
        std::set<uint64_t> hashed_history_set;
        uint64_t ip_1 = history.size() > 0 ? history[0] : 0;
        uint64_t ip_2 = history.size() > 1 ? history[1] : 0;
        uint64_t ip_3 = history.size() > 2 ? history[2] : 0;
        uint64_t hashed_history = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
        hashed_history_set.insert(hashed_history);
        // s.insert(harmony_pred);

        pred = feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].predict(s);

        // if (low_degree_accuracy[degree - 1] == LOW) {
        //     pred = 0;
        // }

        if (LOGGING) {
            cout << "pred: " << pred << endl;
            feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
        }

        // ip_degree_accuracy_map[ip][degree].second++;
        // degree_accuracy_map[degree].second++;
        // ip_accuracy_map[ip].second++;

        // feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].total_count++;
        total_predictions++;
        total_predictions_interval_counter_2++;
    }
    else if (PERCEPTRON_CONSTRAINED) {
        std::set<uint64_t> hashed_history_set;
        uint64_t ip_1 = history.size() > 0 ? history[0] : 0;
        uint64_t ip_2 = history.size() > 1 ? history[1] : 0;
        uint64_t ip_3 = history.size() > 2 ? history[2] : 0;
        uint64_t hashed_history = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
        hashed_history_set.insert(hashed_history);

        uint32_t index = (ip ^ (prefetch_offset + 40)) % NUM_PC_DEGREE_ENTRIES;

        pred = constrained_feature_degree_prefetch_offset_table[index].predict(hashed_history_set);

        ip_degree_accuracy_map[ip][degree].second++;
        degree_accuracy_map[degree].second++;
        // ip_accuracy_map[ip].second++;

        // feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].total_count++;
        total_predictions++;
        total_predictions_interval_counter_2++;
    }
    else if (PERCEPTRON_DEGREE_PREFETCH_OFFSET_DRAM_OCCUPANCY_IP_HISTORY_WEIGHTS) {
        std::set<uint64_t> s(history.begin(), history.end());
        // s.insert(harmony_pred);
        pred = feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].predict(s);
        int dram_pred = dram_occupancy_ip_history_weights[dram_availability].predict(s);
        if (LOGGING) {
            cout << "pred: " << pred << endl;
            cout << "dram_pred: " << dram_pred << endl;
            feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
            dram_occupancy_ip_history_weights[dram_availability].print_weights();

        }
        pred += dram_pred;

        ip_degree_accuracy_map[ip][degree].second++;
        degree_accuracy_map[degree].second++;
        // ip_accuracy_map[ip].second++;
    }
    else if (PERCEPTRON_DEGREE_PREFETCH_OFFSET_DRAM_OCCUPANCY_WEIGHTS) {
        std::set<uint64_t> s(history.begin(), history.end());
        // s.insert(harmony_pred);
        std::set<uint64_t> dram_occupancy_set;
        dram_occupancy_set.insert(dram_availability);

        pred = feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].predict(s);
        int dram_pred = dram_availability_weights.predict(dram_occupancy_set);
        if (LOGGING) {
            cout << "pred: " << pred << endl;
            cout << "dram_pred: " << dram_pred << endl;
            feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
            dram_availability_weights.print_weights();

        }
        pred += dram_pred;

        ip_degree_accuracy_map[ip][degree].second++;
        degree_accuracy_map[degree].second++;
        // ip_accuracy_map[ip].second++;
    }
    else if (BEST_FEATURE_TEST_IMPL) {
        uint64_t  perc_set[NUM_BEST_FEATURES];
        uint64_t ip_1 = history.size() > 0 ? history[0] : 0;
        uint64_t ip_2 = history.size() > 1 ? history[1] : 0;
        uint64_t ip_3 = history.size() > 2 ? history[2] : 0;
        int dram_bucket, pollution_bucket, lateness_bucket, ip_accuracy_bucket, degree_accuracy_bucket = 0;
        double ip_accuracy, pollution, lateness, degree_accuracy = 0.0;
        get_indices(prefetch_trigger_addr, ip, ip_1, ip_2, ip_3, prefetch_offset, degree, dram_availability, perc_set, 
                    &dram_bucket, &pollution_bucket, &ip_accuracy_bucket, &lateness_bucket, &degree_accuracy_bucket,
                    &ip_accuracy, &lateness, &pollution, &degree_accuracy);

        int sum = 0;
        for (int i = 0; i < NUM_BEST_FEATURES; i++) {
            std::set<uint64_t> s;
            s.insert(perc_set[i]);
        sum += features[i].predict(s);
        // Calculate Sum
        }
        pred = sum;
        if (LOGGING) {
            cout << "dram bucket: " << dram_bucket << endl;
            cout << "pollution_bucket: " << pollution_bucket << endl;
            cout << "ip_accuracy_bucket: " << ip_accuracy_bucket << endl;
            cout << "sum: " << sum << endl;
        }
        bool result = (sum > 0);
        if (PERCEPTRON_VECTOR_TRAINING) {
            if (best_feature_results_vector.find(pf_addr) != best_feature_results_vector.end()) {
            best_feature_results_vector[pf_addr].count++;
            best_feature_results_vector[pf_addr].ip_vector.push_back(ip);
            best_feature_results_vector[pf_addr].prefetch_offset_vector.push_back(prefetch_offset);
            best_feature_results_vector[pf_addr].degree_vector.push_back(degree);
            best_feature_results_vector[pf_addr].history_vector.push_back(history);
            best_feature_results_vector[pf_addr].result_vector.push_back(result);
            best_feature_results_vector[pf_addr].degree_accuracy_bucket_vector.push_back(degree_accuracy_bucket);
            best_feature_results_vector[pf_addr].ip_accuracy_bucket_vector.push_back(ip_accuracy_bucket);
            best_feature_results_vector[pf_addr].dram_bucket_vector.push_back(dram_bucket);
            best_feature_results_vector[pf_addr].lateness_bucket_vector.push_back(lateness_bucket);
            best_feature_results_vector[pf_addr].pollution_bucket_vector.push_back(pollution_bucket);
            }
            else if (miss) {
                std::vector<uint64_t> ip_vector;
                std::vector<int> degree_vector;
                std::vector<int> prefetch_offset_vector;
                std::vector<std::vector<uint64_t>> history_vector;
                std::vector<bool> result_vector;
                std::vector<int> dram_bucket_vector;
                std::vector<int> pollution_bucket_vector;
                std::vector<int> lateness_bucket_vector;
                std::vector<int> ip_accuracy_bucket_vector;
                std::vector<int> degree_accuracy_bucket_vector;
                std::vector<uint64_t> trigger_addr_vector;
                ip_vector.push_back(ip);
                degree_vector.push_back(degree);
                prefetch_offset_vector.push_back(prefetch_offset);
                history_vector.push_back(history);
                result_vector.push_back(true);
                trigger_addr_vector.push_back(prefetch_trigger_addr);
                dram_bucket_vector.push_back(dram_bucket);
                pollution_bucket_vector.push_back(pollution_bucket);
                lateness_bucket_vector.push_back(lateness_bucket);
                ip_accuracy_bucket_vector.push_back(ip_accuracy_bucket);
                degree_accuracy_bucket_vector.push_back(degree_accuracy_bucket);

                best_feature_results_vector[pf_addr] = {ip_vector, history_vector, trigger_addr_vector, degree_vector, prefetch_offset_vector, dram_bucket_vector, lateness_bucket_vector, pollution_bucket_vector, ip_accuracy_bucket_vector, degree_accuracy_bucket_vector, result_vector, 1};
            }
        }
        else {
            best_feature_prev_results[pf_addr] = {ip, ip_1, ip_2, ip_3, prefetch_trigger_addr, degree, prefetch_offset, dram_bucket, lateness_bucket, pollution_bucket, ip_accuracy_bucket, degree_accuracy_bucket, result};
        }
    }
    else if (PERCEPTRON_DEGREE_XOR_PREFETCH_OFFSET) {
        uint32_t index = (degree ^ prefetch_offset) % NUM_DEGREE_XOR_OFFSET_ENTRIES;
        std::set<uint64_t> s(history.begin(), history.end());

        pred = pc_degree_xor_prefetch_offset_perceptron_table[ip][index].predict(s);

        if (LOGGING) {
            cout << "pred: " << pred << endl;
            pc_degree_xor_prefetch_offset_perceptron_table[ip][index].print_weights();
        }

        // ip_degree_accuracy_map[ip][degree].second++;
        // degree_accuracy_map[degree].second++;
        // ip_accuracy_map[ip].second++;

    }
    else if (PERCEPTRON_PC_XOR_DEGREE_PREFETCH_OFFSET) {
        uint32_t index = (ip ^ degree) % NUM_PC_DEGREE_ENTRIES;
        std::set<uint64_t> s(history.begin(), history.end());

        pred = pc_xor_degree_prefetch_offset_perceptron_table[index][prefetch_offset].predict(s);

        if (LOGGING) {
            cout << "pred: " << pred << endl;
            pc_xor_degree_prefetch_offset_perceptron_table[index][prefetch_offset].print_weights();
        }

        // ip_degree_accuracy_map[ip][degree].second++;
        // degree_accuracy_map[degree].second++;
        // ip_accuracy_map[ip].second++;

    }
    else if (PERCEPTRON_DEGREE_PREFETCH_OFFSET_SCORE_MAP) {
        std::set<uint64_t> score_set;
        score_set.insert(score_category);
        int pred_score = score_weights_map[ip][prefetch_offset].predict(score_set);
        std::set<uint64_t> s(history.begin(), history.end());
        // s.insert(harmony_pred);

        pred = feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].predict(s);

        if (LOGGING) {
            cout << "pred: " << pred << endl;
            feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
        }
        pred += pred_score;

        ip_degree_accuracy_map[ip][degree].second++;
        degree_accuracy_map[degree].second++;
        // ip_accuracy_map[ip].second++;
        score_weights_map[ip][prefetch_offset].total_count++;
        feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].total_count++;
    }
    else if (PERCEPTRON_DEGREE_PREFETCH_OFFSET_SCORE) {
        std::set<uint64_t> score_set;
        score_set.insert(score);
        int pred_score = score_weights.predict(score_set);
        std::set<uint64_t> s(history.begin(), history.end());
        // s.insert(harmony_pred);

        pred = feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].predict(s);

        if (LOGGING) {
            cout << "pred: " << pred << endl;
            feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
        }
        pred += pred_score;

        ip_degree_accuracy_map[ip][degree].second++;
        degree_accuracy_map[degree].second++;
        // ip_accuracy_map[ip].second++;
        score_weights.total_count++;
        feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].total_count++;
    }
    else if (PERCEPTRON_DEGREE_PREFETCH_OFFSET_SCORE_BUCKET) {
        std::set<uint64_t> s(history.begin(), history.end());
        s.insert(score_category);
        // s.insert(harmony_pred);

        pred = feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].predict(s);

        if (LOGGING) {
            cout << "pred: " << pred << endl;
            feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
        }

        ip_degree_accuracy_map[ip][degree].second++;
        degree_accuracy_map[degree].second++;
        // ip_accuracy_map[ip].second++;

        feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].total_count++;
    }
    else if (PERCEPTRON_DEGREE_PREFETCH_OFFSET_PRED) {
        std::set<uint64_t> s(history.begin(), history.end());
        // s.insert(harmony_pred);

        pred = feature_degree_prefetch_offset_pred_perceptron_table[ip][degree][prefetch_offset][harmony_pred].predict(s);

        if (LOGGING) {
            cout << "pred: " << pred << endl;
            feature_degree_prefetch_offset_pred_perceptron_table[ip][degree][prefetch_offset][harmony_pred].print_weights();
        }

        ip_degree_accuracy_map[ip][degree].second++;
        degree_accuracy_map[degree].second++;
        // ip_accuracy_map[ip].second++;

        feature_degree_prefetch_offset_pred_perceptron_table[ip][degree][prefetch_offset][harmony_pred].total_count++;
    }
    else if (PERCEPTRON_DEGREE_PREFETCH_OFFSET_PLUS_PAGE) {
        std::set<uint64_t> page;
        page.insert(PAGE(prefetch_trigger_addr));
        int pred_page = trigger_addr_page_weights.predict(page);
        std::set<uint64_t> s(history.begin(), history.end());
        // s.insert(harmony_pred);

        pred = feature_degree_prefetch_offset_pred_perceptron_table[ip][degree][prefetch_offset][harmony_pred].predict(s);

        if (LOGGING) {
            cout << "pred: " << pred << endl;
            cout << "pred_page: " << pred_page << endl;
            feature_degree_prefetch_offset_pred_perceptron_table[ip][degree][prefetch_offset][harmony_pred].print_weights();
        }
        pred += pred_page;

        ip_degree_accuracy_map[ip][degree].second++;
        degree_accuracy_map[degree].second++;
        // ip_accuracy_map[ip].second++;
        trigger_addr_page_weights.total_count++;
        feature_degree_prefetch_offset_pred_perceptron_table[ip][degree][prefetch_offset][harmony_pred].total_count++;
    }
    else if (PERCEPTRON_GLOBAL_WEIGHTS) {
        std::set<uint64_t> s(history.begin(), history.end());
        // s.insert(degree);
        // s.insert(PAGE(pf_addr));

        pred = weight_table.predict(s);
        if (LOGGING) {
            cout << "pred: " << pred << endl;
            weight_table.print_weights();
        }
    }
    // if (warmup_complete) {
    //     lo_threshold = 1;
    // }
    bool do_pf = (pred >= lo_threshold) ? true : false;
    bool fill_l2 = (pred >= MY_THRESHOLD_HI) ? true : false;

    // if (dram_availability > 4 && pred < 50) {
    //     do_pf = false;
    // }
    if (do_pf && bypass) {
        return PREFETCH_L2;
    }
    // else if (!do_pf && bypass && !warmup_complete) {
    //     return PREFETCH_L2;
    // }
    
    if (!do_pf) {
        if (LOGGING) {
            cout << "get prediction for ip: " << ip << " false" << endl;
        }

        // if (PERCEPTRON_ACCURACY_GATE) {
        //     int accurate = 0;
        //     int total = 0;
        //     if (PERCEPTRON_COMBINED) {
        //         accurate = feature_perceptron_table[ip].positive_count;
        //         total = feature_perceptron_table[ip].total_count;
        //     }
        //     else if (PERCEPTRON_DEGREE) {
        //         accurate = feature_degree_perceptron_table[ip][degree].positive_count;
        //         total = feature_degree_perceptron_table[ip][degree].total_count;
        //     }
        //     else if (PERCEPTRON_DEGREE_PREFETCH_OFFSET) {
        //         accurate = feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].positive_count;
        //         total = feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].total_count;
        //     }
        //     else if (PERCEPTRON_GLOBAL_WEIGHTS) {
        //         accurate = weight_table.positive_count;
        //         total = weight_table.total_count;
        //     }

        //     if (total > 100) {
        //         double accuracy = (double) accurate / (double) total;
        //         // cout << "accuracy for ip: " << ip << " " << accuracy << endl;
        //         if (accuracy < 0.04) {
        //             prev_results[pf_addr] = {true, degree, ip, prefetch_trigger_addr, prefetch_offset, dram_availability_category, accuracy_category, harmony_pred, history};
        //             return true;
        //         }
        //     }
        // }

        // if (PERCEPTRON_IP_ACCURACY_GATE) {
        //     if (ip_accuracy_map[ip].second > 0) {
        //         double accuracy = (double) ip_accuracy_map[ip].first / (double) ip_accuracy_map[ip].second;
        //         if (accuracy > 0.6) {
        //             prev_results[pf_addr] = {true, degree, ip, prefetch_trigger_addr, prefetch_offset, dram_availability_category, accuracy_category, harmony_pred, history};
        //             return true;
        //         }
        //     }
        // }

        if (PERCEPTRON_REJECT_TABLE) {
            reject_table.insert(pf_addr);
        }
        else if (PERCEPTRON_DIRECT_MAPPED_REJECT_TABLE) {
            int hash = pf_addr % 8192;
            direct_mapped_reject_table[hash] = pf_addr;
        }
        else if (PERCEPTRON_REJECT_CACHE) {
            if (reject_table_cache.find(set) == reject_table_cache.end()) {
                vector<Reject_Cache> v;
                reject_table_cache[set] = v;
            }
            reject_table_cache[set].push_back({max(rrpv-1, 0), pf_addr, false});
        }
        if (!BEST_FEATURE_TEST_IMPL && PERCEPTRON_VECTOR_TRAINING) {
            if (prev_results_vector.find(pf_addr) != prev_results_vector.end()) {
                prev_results_vector[pf_addr].count++;
                prev_results_vector[pf_addr].ip_vector.push_back(ip);
                prev_results_vector[pf_addr].prefetch_offset_vector.push_back(prefetch_offset);
                prev_results_vector[pf_addr].degree_vector.push_back(degree);
                prev_results_vector[pf_addr].history_vector.push_back(history);
                prev_results_vector[pf_addr].dram_occupancy_vector.push_back(dram_availability);
                prev_results_vector[pf_addr].result_vector.push_back(false);
            }
            else if (miss && prev_results_vector.find(pf_addr) == prev_results_vector.end()) {
                std::vector<uint64_t> ip_vector;
                std::vector<int> degree_vector;
                std::vector<int> prefetch_offset_vector;
                std::vector<std::vector<uint64_t>> history_vector;
                std::vector<bool> result_vector;
                std::vector<int> dram_occupancy_vector;
                ip_vector.push_back(ip);
                degree_vector.push_back(degree);
                prefetch_offset_vector.push_back(prefetch_offset);
                history_vector.push_back(history);
                result_vector.push_back(false);
                dram_occupancy_vector.push_back(dram_availability);
                prev_results_vector[pf_addr] = {ip_vector, degree_vector, prefetch_offset_vector, history_vector, dram_occupancy_vector, result_vector, 1};
            }
        }
        else if (!BEST_FEATURE_TEST_IMPL) {
            prev_results[pf_addr] = {true, degree, ip, prefetch_trigger_addr, prefetch_offset, dram_availability, accuracy_category, harmony_pred, score_category, history};
        }
        // if (!warmup_complete) {
        //     return PREFETCH_L2;
        // }
        // else {
        //     return SUPPRESS;
        // }
        // return PREFETCH_L2;
        return SUPPRESS;
    }

    if (LOGGING) {
            cout << "get prediction for ip: " << ip << " true" << endl;
    }
    if (!BEST_FEATURE_TEST_IMPL && PERCEPTRON_VECTOR_TRAINING) {
        if (prev_results_vector.find(pf_addr) != prev_results_vector.end()) {
            prev_results_vector[pf_addr].count++;
            prev_results_vector[pf_addr].ip_vector.push_back(ip);
            prev_results_vector[pf_addr].prefetch_offset_vector.push_back(prefetch_offset);
            prev_results_vector[pf_addr].degree_vector.push_back(degree);
            prev_results_vector[pf_addr].history_vector.push_back(history);
            prev_results_vector[pf_addr].dram_occupancy_vector.push_back(dram_availability);
            prev_results_vector[pf_addr].result_vector.push_back(true);
        }
        else if (miss) {
            std::vector<uint64_t> ip_vector;
            std::vector<int> degree_vector;
            std::vector<int> prefetch_offset_vector;
            std::vector<std::vector<uint64_t>> history_vector;
            std::vector<bool> result_vector;
            std::vector<int> dram_occupancy_vector;
            ip_vector.push_back(ip);
            degree_vector.push_back(degree);
            prefetch_offset_vector.push_back(prefetch_offset);
            history_vector.push_back(history);
            result_vector.push_back(true);
            dram_occupancy_vector.push_back(dram_availability);
            prev_results_vector[pf_addr] = {ip_vector, degree_vector, prefetch_offset_vector, history_vector, dram_occupancy_vector, result_vector, 1};
        }
    } else if (!BEST_FEATURE_TEST_IMPL) {
        prev_results[pf_addr] = {true, degree, ip, prefetch_trigger_addr, prefetch_offset, dram_availability, accuracy_category, harmony_pred, score_category, history};
    }
    // if (miss) {
    //     cout << "bank occupancy: " << dram_availability << endl;
    //     cout << "sum: " << pred << endl;
    // }
    // return pred;
    if (disable_filter) {
        return SUPPRESS;
    }
    if (fill_l2) {
        return PREFETCH_L2;
    }
    return PREFETCH_LLC;
}

int Prefetch_Filter::get_int_prediction(uint64_t ip, uint64_t pf_addr, int degree, uint64_t prefetch_trigger_addr, int prefetch_offset, vector<uint64_t> history) {
    int pred = 0;
    if (PERCEPTRON_DEGREE_PREFETCH_OFFSET) {
        std::set<uint64_t> s(history.begin(), history.end());
        // s.insert(harmony_pred);

        pred = feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].predict(s);

        // if (low_degree_accuracy[degree - 1] == LOW) {
        //     pred = 0;
        // }

        if (LOGGING) {
            cout << "pred: " << pred << endl;
            feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
        }
    }
    return pred;
}

void Prefetch_Filter::add_to_reject_table(uint64_t ip, uint64_t pf_addr, int degree, uint64_t prefetch_trigger_addr, int prefetch_offset, bool result, vector<uint64_t> history) {
    if (!BEST_FEATURE_TEST_IMPL && PERCEPTRON_VECTOR_TRAINING) {
        if (prev_results_vector.find(pf_addr) != prev_results_vector.end()) {
            prev_results_vector[pf_addr].count++;
            prev_results_vector[pf_addr].ip_vector.push_back(ip);
            prev_results_vector[pf_addr].prefetch_offset_vector.push_back(prefetch_offset);
            prev_results_vector[pf_addr].degree_vector.push_back(degree);
            prev_results_vector[pf_addr].history_vector.push_back(history);
            prev_results_vector[pf_addr].dram_occupancy_vector.push_back(0);
            prev_results_vector[pf_addr].result_vector.push_back(result);
        }
        else {
            std::vector<uint64_t> ip_vector;
            std::vector<int> degree_vector;
            std::vector<int> prefetch_offset_vector;
            std::vector<std::vector<uint64_t>> history_vector;
            std::vector<bool> result_vector;
            std::vector<int> dram_occupancy_vector;
            ip_vector.push_back(ip);
            degree_vector.push_back(degree);
            prefetch_offset_vector.push_back(prefetch_offset);
            history_vector.push_back(history);
            result_vector.push_back(result);
            dram_occupancy_vector.push_back(0);
            prev_results_vector[pf_addr] = {ip_vector, degree_vector, prefetch_offset_vector, history_vector, dram_occupancy_vector, result_vector, 1};
        }
    }
    int hash = pf_addr % 8192;
    direct_mapped_reject_table[hash] = pf_addr;

}

void Prefetch_Filter::train(uint64_t ip, uint64_t pf_addr, bool result, vector<uint64_t> history) {
    set<uint64_t> s(history.begin(), history.end());
    bool prev_result = true;
    int degree = 1;
    uint64_t prefetch_trigger_addr_page = 0;
    uint64_t prefetch_offset= 0;
    uint64_t score_category = LOW_SCORE;
    if (prev_results.find(pf_addr) != prev_results.end()) {
        prev_result = prev_results[pf_addr].result;
        degree = prev_results[pf_addr].degree;
        prefetch_trigger_addr_page = prev_results[pf_addr].trigger_addr_page;
        prefetch_offset = prev_results[pf_addr].prefetch_offset;
    }
    // s.insert(pf_addr);
    s.insert(degree);

    int pred = feature_perceptron_table[ip].predict(s);
    int coeff = result ? 1 : -1;

    if (LOGGING) {
        cout << "training with result: " << result << endl;
        feature_perceptron_table[ip].print_weights();
    }

    if ((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold) {
        feature_perceptron_table[ip].update(ip, coeff, s);
    }

    if (LOGGING) {
        feature_perceptron_table[ip].print_weights();
    }

    if (prev_result == result) {
        feature_perceptron_table[ip].positive_count++;
    }
    
}

void Prefetch_Filter::train_vector(uint64_t pf_addr, bool result, bool reject_cache, bool useful, bool pollution_training, bool late, bool erase, int reward) {
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

    if (LOGGING) {
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
        // if (!result && low_degree_accuracy[degree_vector[i]-1] == MIDDLE) {
        //     reward += 1;
        // }
        // else if (!result && low_degree_accuracy[degree_vector[i]-1] == LOW) {
        //     reward += 3;
        // }
        // if (!result && high_lateness) {
        //     reward += 1;
        // }
        // if (pollution_training && low_degree_accuracy[degree_vector[i]-1] == HIGH) {
        //     reward = 0;
        // }
        // if (!reject_cache && !late && normal_false_training && erase) {
        //     reward += 2;
        // }
        // reward = (!result && low_degree_accuracy[degree_vector[i]-1]) ? reward + 1 : reward;
        // if (result && reject_cache && higher_positive_training) {
        //     reward += 1;
        // }
        int coeff = result ? reward : -1*reward;
        set<uint64_t> s(history_vector[i].begin(), history_vector[i].end());
        std::set<uint64_t> hashed_history_set;
        uint64_t ip_1 = history_vector[i].size() > 0 ? history_vector[i][0] : 0;
        uint64_t ip_2 = history_vector[i].size() > 1 ? history_vector[i][1] : 0;
        uint64_t ip_3 = history_vector[i].size() > 2 ? history_vector[i][2] : 0;
        uint64_t hashed_history = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
        hashed_history_set.insert(hashed_history);

        if (PERCEPTRON_DEGREE_PREFETCH_OFFSET) {
            int pred = feature_degree_prefetch_offset_perceptron_table[ip_vector[i]][degree_vector[i]][prefetch_offset_vector[i]].predict(s);

            if (LOGGING) {
                cout << "pred: " << pred << endl;   
            }

            if (((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold)) {
                feature_degree_prefetch_offset_perceptron_table[ip_vector[i]][degree_vector[i]][prefetch_offset_vector[i]].update(ip_vector[i], coeff, s);
            }

            if (result_vector[i] == result) {
                feature_degree_prefetch_offset_perceptron_table[ip_vector[i]][degree_vector[i]][prefetch_offset_vector[i]].positive_count++;
                accurate_predictions_interval_counter++;
                accurate_predictions_count++;
            }
            total_predictions_interval_counter++;
            total_predictions_count++;

            feature_degree_prefetch_offset_perceptron_table[ip_vector[i]][degree_vector[i]][prefetch_offset_vector[i]].total_count++;
        }
        else if (PERCEPTRON_GLOBAL_WEIGHTS) {
            int pred = weight_table.predict(s);

            if (((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold)) {
                weight_table.update(ip_vector[i], coeff, s);
            }

            if (result_vector[i] == result) {
                weight_table.positive_count++;
                accurate_predictions_interval_counter++;
                accurate_predictions_count++;
            }
            total_predictions_interval_counter++;
            total_predictions_count++;

            weight_table.total_count++;
        }
        else if (PERCEPTRON_DEGREE) {
            int pred = feature_degree_perceptron_table[ip_vector[i]][degree_vector[i]].predict(s);

            if (((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold)) {
                feature_degree_perceptron_table[ip_vector[i]][degree_vector[i]].update(ip_vector[i], coeff, s);
            }

            if (result_vector[i] == result) {
                feature_degree_perceptron_table[ip_vector[i]][degree_vector[i]].positive_count++;
                accurate_predictions_interval_counter++;
                accurate_predictions_count++;
            }
            total_predictions_interval_counter++;
            total_predictions_count++;

            feature_degree_perceptron_table[ip_vector[i]][degree_vector[i]].total_count++;
        }
        else if (PERCEPTRON_COMBINED) {
            int pred = feature_perceptron_table[ip_vector[i]].predict(s);

            if (((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold)) {
                feature_perceptron_table[ip_vector[i]].update(ip_vector[i], coeff, s);
            }

            if (result_vector[i] == result) {
                feature_perceptron_table[ip_vector[i]].positive_count++;
                accurate_predictions_interval_counter++;
                accurate_predictions_count++;
            }
            total_predictions_interval_counter++;
            total_predictions_count++;

            feature_perceptron_table[ip_vector[i]].total_count++;
        }
        else if (PERCEPTRON_CONSTRAINED) {
            uint32_t index = (ip_vector[i] ^ degree_vector[i] ^ (prefetch_offset_vector[i] + 40)) % NUM_PC_DEGREE_ENTRIES;
            std::set<uint64_t> hashed_history_set;
            uint64_t ip_1 = history_vector[i].size() > 0 ? history_vector[i][0] : 0;
            uint64_t ip_2 = history_vector[i].size() > 1 ? history_vector[i][1] : 0;
            uint64_t ip_3 = history_vector[i].size() > 2 ? history_vector[i][2] : 0;
            uint64_t hashed_history = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
            hashed_history_set.insert(hashed_history);

            int pred = constrained_feature_degree_prefetch_offset_table[index].predict(s);

            if (((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold)) {
                constrained_feature_degree_prefetch_offset_table[index].update(ip_vector[i], coeff, hashed_history_set);
            }

            if (result_vector[i] == result) {
                constrained_feature_degree_prefetch_offset_table[index].positive_count++;
                accurate_predictions_interval_counter++;
                accurate_predictions_count++;
            }
            total_predictions_interval_counter++;
            total_predictions_count++;

            constrained_feature_degree_prefetch_offset_table[index].total_count++;
        }

        if (PERCEPTRON_DEGREE_PREFETCH_OFFSET_DRAM_OCCUPANCY_WEIGHTS) {
            std::set<uint64_t> dram_occupancy_set;
            dram_occupancy_set.insert(dram_occupancy_vector[i]);
            int dram_pred = dram_availability_weights.predict(dram_occupancy_set);
            if (((dram_pred <= 0 && result) || (dram_pred > 0 && !result) || std::abs(dram_pred) < training_threshold)) {
                dram_availability_weights.update(ip_vector[i], coeff, dram_occupancy_set);
            }
            bool dram_result = (dram_pred > 0);
            if (dram_result == result) {
                dram_availability_weights.positive_count++;
            }
            dram_availability_weights.total_count++;
        }
        if (PERCEPTRON_DEGREE_PREFETCH_OFFSET_DRAM_OCCUPANCY_IP_HISTORY_WEIGHTS) {
            int dram_pred = dram_occupancy_ip_history_weights[dram_occupancy_vector[i]].predict(s);
            if (((dram_pred <= 0 && result) || (dram_pred > 0 && !result) || std::abs(dram_pred) < training_threshold)) {
                dram_occupancy_ip_history_weights[dram_occupancy_vector[i]].update(ip_vector[i], coeff, s);
            }
            bool dram_result = (dram_pred > 0);
            if (dram_result == result) {
                dram_occupancy_ip_history_weights[dram_occupancy_vector[i]].positive_count++;
            }
            dram_occupancy_ip_history_weights[dram_occupancy_vector[i]].total_count++;
        }

        // extra training
        if (DRAM_EXTRA_TRAINING && !result && dram_occupancy_vector[i] >= 20) {
            feature_degree_prefetch_offset_perceptron_table[ip_vector[i]][degree_vector[i]][prefetch_offset_vector[i]].update(ip_vector[i], coeff, s);;
        }
        
        dram_occupancy_global_vector.push_back(dram_occupancy_vector[i]);

        if (LOGGING) {
            feature_degree_prefetch_offset_perceptron_table[ip_vector[i]][degree_vector[i]][prefetch_offset_vector[i]].print_weights();
        }

        // if (result_vector[i] == result) {
        //     feature_degree_prefetch_offset_perceptron_table[ip_vector[i]][degree_vector[i]][prefetch_offset_vector[i]].positive_count++;
        //     accurate_predictions_interval_counter++;
        //     accurate_predictions_count++;
        // }
        // total_predictions_interval_counter++;
        // total_predictions_count++;

        if (PERCEPTRON_PCA) {
            uint64_t ip_1 = history_vector[i].size() > 0 ? history_vector[i][0] : 0;
            uint64_t ip_2 = history_vector[i].size() > 1 ? history_vector[i][1] : 0;
            uint64_t ip_3 = history_vector[i].size() > 2 ? history_vector[i][2] : 0;
            uint64_t hashed_history = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
            pca_out << ip_vector[i] << ",";
            pca_out << degree_vector[i] << ",";
            pca_out << prefetch_offset_vector[i] << ",";
            pca_out << ip_1 << ","; 
            pca_out << ip_2 << ","; 
            pca_out << ip_3 << ","; 
            pca_out << dram_occupancy_vector[i] << ",";
            pca_out << pollution_training << ",";
            pca_out << result << endl;
        }

        if (i == 0) {
            if (result && result_vector[i] == result) {
            useful_correct++;
            useful_correct_interval_counter++;
            }
            else if (!result && result_vector[i] == result) {
                useless_correct++;
                useless_correct_interval_counter++;
            }
            else if (result && result_vector[i] != result) {
                useful_wrong++;
                useful_wrong_interval_counter++;
            }
            else if (!result && result_vector[i] != result) {
                useless_wrong++;
                useless_wrong_interval_counter++;
            }

            total_training++;
            total_training_interval_counter++;

            degree_accuracy_map[degree_vector[i]].second++;
        
            if (useful) {
                ip_degree_accuracy_map[ip_vector[i]][degree_vector[i]].first++;
                degree_accuracy_map[degree_vector[i]].first++;
            }
        }
    }
    prev_results_vector.erase(pf_addr);
}

void Prefetch_Filter::train_best_feature(uint64_t pf_addr, bool result, bool reject_cache, bool useful) {
    uint64_t  perc_set[NUM_BEST_FEATURES];
    get_indices_training(best_feature_prev_results[pf_addr].trigger_addr, best_feature_prev_results[pf_addr].ip, best_feature_prev_results[pf_addr].ip_1, best_feature_prev_results[pf_addr].ip_2, best_feature_prev_results[pf_addr].ip_3, 
    best_feature_prev_results[pf_addr].prefetch_offset, best_feature_prev_results[pf_addr].degree, best_feature_prev_results[pf_addr].dram_bucket, best_feature_prev_results[pf_addr].pollution_bucket, best_feature_prev_results[pf_addr].ip_accuracy_bucket, 
    best_feature_prev_results[pf_addr].lateness_bucket, best_feature_prev_results[pf_addr].degree_accuracy_bucket, 0.0, 0.0, 0.0,
    0.0, 0.0, perc_set);
    bool prev_result = best_feature_prev_results[pf_addr].result;
    int sum = 0;
    int coeff = result ? 1 : -1;
    
    for (int i = 0; i < NUM_BEST_FEATURES; i++) {
        std::set<uint64_t> s;
        s.insert(perc_set[i]);
        int pred = features[i].predict(s);
        sum += pred;
        bool pred_result = (pred >= 0);
        if (pred_result == result) {
            features[i].positive_count++;
        }
        features[i].total_count++;
      // Calculate Sum
    }
    bool sum_result = (sum >= 0);
    if (sum_result != result || std::abs(sum) < training_threshold) {
        for (int i = 0; i < NUM_BEST_FEATURES; i++) {
            std::set<uint64_t> s;
            s.insert(perc_set[i]);
            features[i].update(prev_results[pf_addr].ip, coeff, s);
        }
    }
    // TODO: update all weights
    degree_accuracy_map[best_feature_prev_results[pf_addr].degree].second++;

    if (useful) {
        // ip_degree_accuracy_map[ip][degree].first++;
        degree_accuracy_map[best_feature_prev_results[pf_addr].degree].first++;
    }

    best_feature_prev_results.erase(pf_addr);
    return;
}

void Prefetch_Filter::train_best_feature_vector(uint64_t pf_addr, bool result, bool reject_cache, bool useful) {
    vector<uint64_t> ip_vector;
    vector<int> degree_vector;
    vector<int> prefetch_offset_vector;
    vector<vector<uint64_t>> history_vector;
    vector<bool> result_vector;
    vector<int> dram_bucket_vector;
    vector<int> lateness_bucket_vector;
    vector<int> pollution_bucket_vector;
    vector<int> ip_accuracy_bucket_vector;
    vector<int> degree_accuracy_bucket_vector;
    vector<uint64_t> trigger_addr_vector;
    int count = 0;

    if (best_feature_results_vector.find(pf_addr) != best_feature_results_vector.end()) {
        ip_vector = best_feature_results_vector[pf_addr].ip_vector;
        degree_vector = best_feature_results_vector[pf_addr].degree_vector;
        prefetch_offset_vector = best_feature_results_vector[pf_addr].prefetch_offset_vector;
        history_vector = best_feature_results_vector[pf_addr].history_vector;
        result_vector = best_feature_results_vector[pf_addr].result_vector;
        trigger_addr_vector = best_feature_results_vector[pf_addr].trigger_addr_vector;
        dram_bucket_vector = best_feature_results_vector[pf_addr].dram_bucket_vector;
        lateness_bucket_vector = best_feature_results_vector[pf_addr].lateness_bucket_vector;
        pollution_bucket_vector = best_feature_results_vector[pf_addr].pollution_bucket_vector;
        ip_accuracy_bucket_vector = best_feature_results_vector[pf_addr].ip_accuracy_bucket_vector;
        degree_accuracy_bucket_vector = best_feature_results_vector[pf_addr].degree_accuracy_bucket_vector;
        count = best_feature_results_vector[pf_addr].count;
    }

    if (LOGGING) {
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
        cout << "result vector" << endl;
        for (int i = 0; i < count; i++) {
            cout << result_vector[i] << ", ";
        }
        cout << endl;
        cout << "dram bucket vector" << endl;
        for (int i = 0; i < count; i++) {
            cout << dram_bucket_vector[i] << ", ";
        }
        cout << endl;
        cout << "pollution vector" << endl;
        for (int i = 0; i < count; i++) {
            cout << pollution_bucket_vector[i] << ", ";
        }
        cout << endl;
        cout << "ip accuracy vector" << endl;
        for (int i = 0; i < count; i++) {
            cout << ip_accuracy_bucket_vector[i] << ", ";
        }
        cout << endl;
    }

    int coeff = result ? 1 : -1;
    for (int i = 0; i < count; i++) {
        uint64_t  perc_set[NUM_BEST_FEATURES];
        uint64_t ip_1 = history_vector[i].size() > 0 ?  history_vector[i][0] : 0;
        uint64_t ip_2 = history_vector[i].size() > 1 ?  history_vector[i][1] : 0;
        uint64_t ip_3 = history_vector[i].size() > 2 ?  history_vector[i][2] : 0;
        get_indices_training(trigger_addr_vector[i], ip_vector[i], ip_1, ip_2, ip_3,
        prefetch_offset_vector[i], degree_vector[i], dram_bucket_vector[i], pollution_bucket_vector[i], ip_accuracy_bucket_vector[i], 
        lateness_bucket_vector[i], degree_accuracy_bucket_vector[i], 0.0, 0.0, 0.0,
        0.0, 0.0, perc_set);
        bool prev_result = result_vector[i];
        int sum = 0;


        for (int i = 0; i < NUM_BEST_FEATURES; i++) {
            std::set<uint64_t> s;
            s.insert(perc_set[i]);
            int pred = features[i].predict(s);
            sum += pred;
            bool pred_result = (pred >= 0);
            if (pred_result == result) {
                features[i].positive_count++;
            }
            features[i].total_count++;
        // Calculate Sum
        }
        bool sum_result = (sum >= 0);
        if (sum_result != result || std::abs(sum) < training_threshold) {
            for (int i = 0; i < NUM_BEST_FEATURES; i++) {
                std::set<uint64_t> s;
                s.insert(perc_set[i]);
                features[i].update(0, coeff, s);
            }
        }

        if (PERCEPTRON_PCA) {
            uint64_t ip_1 = history_vector[i].size() > 0 ? history_vector[i][0] : 0;
            uint64_t ip_2 = history_vector[i].size() > 1 ? history_vector[i][1] : 0;
            uint64_t ip_3 = history_vector[i].size() > 2 ? history_vector[i][2] : 0;
            uint64_t hashed_history = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
            pca_out << ip_vector[i] << ",";
            pca_out << degree_vector[i] << ",";
            pca_out << prefetch_offset_vector[i] << ",";
            pca_out << hashed_history << ","; 
            pca_out << result << endl;
        }

        degree_accuracy_map[degree_vector[i]].second++;

        if (useful) {
            // ip_degree_accuracy_map[ip][degree].first++;
            degree_accuracy_map[degree_vector[i]].first++;
        }
    }

    best_feature_results_vector.erase(pf_addr);
    return;
}

void Prefetch_Filter::train(uint64_t pf_addr, bool result, bool reject_cache, bool useful, bool pollution_training, bool late, bool erase, int reward) {
    if (PERCEPTRON_HISTORY) {
        bool prev_result = true;
        int degree = 1;
        uint64_t ip = 0;
        uint64_t prefetch_trigger_addr_page = 0;
        uint64_t prefetch_offset= 0;
        int dram_availability_category = LOW_DRAM_AVAILABILITY;
        int dram_availability = 0;
        int accuracy_category = LOW_PREFETCHER_ACCURACY;
        bool harmony_pred = false;
        uint64_t score = 0;
        uint64_t score_category = LOW_SCORE;
        std::vector<uint64_t> history;

        if (BEST_FEATURE_TEST_IMPL && PERCEPTRON_VECTOR_TRAINING) {
            train_best_feature_vector(pf_addr, result, reject_cache, useful);
            return;
        }
        else if (PERCEPTRON_VECTOR_TRAINING) {
            train_vector(pf_addr, result, reject_cache, useful, pollution_training, late, erase, reward);
            return;
        }
        else if (BEST_FEATURE_TEST_IMPL) {
            train_best_feature(pf_addr, result, reject_cache, useful);
            return;
        }
        if (prev_results.find(pf_addr) != prev_results.end()) {
            prev_result = prev_results[pf_addr].result;
            degree = prev_results[pf_addr].degree;
            history = prev_results[pf_addr].history;
            ip = prev_results[pf_addr].ip;
            prefetch_trigger_addr_page = prev_results[pf_addr].trigger_addr_page;
            prefetch_offset = prev_results[pf_addr].prefetch_offset;
            dram_availability_category = prev_results[pf_addr].dram_availability_category;
            dram_availability = prev_results[pf_addr].dram_availability_category;
            accuracy_category = prev_results[pf_addr].prefetcher_accuracy_category;
            harmony_pred = prev_results[pf_addr].harmony_prediction;
            score = prev_results[pf_addr].score;
        }

        set<uint64_t> s(history.begin(), history.end());
        if (PERCEPTRON_COMBINED) {
            s.insert(degree);
            s.insert(prefetch_offset);
            // s.insert(PAGE(pf_addr));

            int pred = feature_perceptron_table[ip].predict(s);
            int coeff = result ? 1 : -1;

            if (LOGGING) {
                cout << "training with result: " << result << endl;
                cout << "degree: " << degree << endl;
                for (int i = 0; i < history.size(); i++) {
                    cout << history[i] << ", ";
                }
                cout << endl;
                feature_perceptron_table[ip].print_weights();
            }

            if ((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold) {
            feature_perceptron_table[ip].update(ip, coeff, s);
            }

            if (LOGGING) {
                feature_perceptron_table[ip].print_weights();
            }

            if (prev_result == result) {
                feature_perceptron_table[ip].positive_count++;
            }
        }
        else if (PERCEPTRON_DEGREE) {
            // s.insert(prefetch_trigger_addr_page);
            s.insert(prefetch_offset);
            // s.insert(dram_availability_category);
            // s.insert(accuracy_category);
            int pred = feature_degree_perceptron_table[ip][degree].predict(s);
            int coeff = result ? 1 : -1;

            if (LOGGING) {
                cout << "training with result: " << result << endl;
                cout << "degree: " << degree << endl;
                for (int i = 0; i < history.size(); i++) {
                    cout << history[i] << ", ";
                }
                cout << endl;
                feature_degree_perceptron_table[ip][degree].print_weights();
            }

            if ((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold) {
            feature_degree_perceptron_table[ip][degree].update(ip, coeff, s);
            }

            if (LOGGING) {
                feature_degree_perceptron_table[ip][degree].print_weights();
            }

            if (prev_result == result) {
                feature_degree_perceptron_table[ip][degree].positive_count++;
            }
        }
        else if (PERCEPTRON_DEGREE_PREFETCH_OFFSET) {
            // s.insert(harmony_pred);
            int pred = feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].predict(s);

            int coeff = result ? 1 : -1;

            if (LOGGING) {
                cout << "training with result: " << result << endl;
                cout << "degree: " << degree << endl;
                cout << "offset: " << prefetch_offset << endl;
                for (int i = 0; i < history.size(); i++) {
                    cout << history[i] << ", ";
                }
                cout << endl;
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
            }

            if ((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold) {
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].update(ip, coeff, s);
            }

            if (LOGGING) {
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
            }

            if (prev_result == result) {
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].positive_count++;
                accurate_predictions_interval_counter++;
                accurate_predictions_count++;
            }
            total_predictions_interval_counter++;
            total_predictions_count++;
            
            if (useful) {
                ip_degree_accuracy_map[ip][degree].first++;
                degree_accuracy_map[degree].first++;
            }

            if (result && prev_result == result) {
                useful_correct++;
                useful_correct_interval_counter++;
            }
            else if (!result && prev_result == result) {
                useless_correct++;
                useless_correct_interval_counter++;
            }
            else if (result && prev_result != result) {
                useful_wrong++;
                useful_wrong_interval_counter++;
            }
            else if (!result && prev_result != result) {
                useless_wrong++;
                useless_wrong_interval_counter++;
            }

        }
        else if (PERCEPTRON_DEGREE_PREFETCH_OFFSET_DRAM_OCCUPANCY_WEIGHTS) {
            std::set<uint64_t> dram_occupancy_set;
            dram_occupancy_set.insert(dram_availability);
            int pred = feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].predict(s);
            int dram_pred = dram_availability_weights.predict(dram_occupancy_set);

            int coeff = result ? 1 : -1;

            if (LOGGING) {
                cout << "training with result: " << result << endl;
                cout << "degree: " << degree << endl;
                cout << "offset: " << prefetch_offset << endl;
                cout << "dram_availability: " << dram_availability << endl;
                for (int i = 0; i < history.size(); i++) {
                    cout << history[i] << ", ";
                }
                cout << endl;
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
            }

            if ((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold) {
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].update(ip, coeff, s);
            }
            if ((dram_pred <= 0 && result) || (dram_pred > 0 && !result) || std::abs(dram_pred) < training_threshold) {
                dram_availability_weights.update(ip, coeff, dram_occupancy_set);
            }

            if (LOGGING) {
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
            }

            if (prev_result == result) {
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].positive_count++;
            }

            bool dram_result = (dram_pred > 0);
            if (dram_result == result) {
                dram_availability_weights.positive_count++;
            }
            
            if (useful) {
                ip_degree_accuracy_map[ip][degree].first++;
                degree_accuracy_map[degree].first++;
            }

            dram_availability_weights.total_count++;
            feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].total_count++;
        }
        else if (PERCEPTRON_DEGREE_XOR_PREFETCH_OFFSET) {
            uint32_t index = (degree ^ prefetch_offset) % NUM_DEGREE_XOR_OFFSET_ENTRIES;
            int pred = pc_degree_xor_prefetch_offset_perceptron_table[ip][index].predict(s);

            int coeff = result ? 1 : -1;

            if (LOGGING) {
                cout << "training with result: " << result << endl;
                cout << "degree: " << degree << endl;
                cout << "offset: " << prefetch_offset << endl;
                for (int i = 0; i < history.size(); i++) {
                    cout << history[i] << ", ";
                }
                cout << endl;
                pc_degree_xor_prefetch_offset_perceptron_table[ip][index].print_weights();
            }

            if ((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold) {
                pc_degree_xor_prefetch_offset_perceptron_table[ip][index].update(ip, coeff, s);
            }

            if (LOGGING) {
                pc_degree_xor_prefetch_offset_perceptron_table[ip][index].print_weights();
            }

            if (prev_result == result) {
                pc_degree_xor_prefetch_offset_perceptron_table[ip][index].positive_count++;
            }
            pc_degree_xor_prefetch_offset_perceptron_table[ip][index].total_count++;
            
            if (useful) {
                degree_accuracy_map[degree].first++;
            }
            degree_accuracy_map[degree].second++;
        }
        else if (PERCEPTRON_PC_XOR_DEGREE_PREFETCH_OFFSET) {
            uint32_t index = (ip ^ degree) % NUM_PC_DEGREE_ENTRIES;
            int pred = pc_xor_degree_prefetch_offset_perceptron_table[index][prefetch_offset].predict(s);

            int coeff = result ? 1 : -1;

            if (LOGGING) {
                cout << "training with result: " << result << endl;
                cout << "degree: " << degree << endl;
                cout << "offset: " << prefetch_offset << endl;
                for (int i = 0; i < history.size(); i++) {
                    cout << history[i] << ", ";
                }
                cout << endl;
                pc_xor_degree_prefetch_offset_perceptron_table[index][prefetch_offset].print_weights();
            }

            if ((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold) {
                pc_xor_degree_prefetch_offset_perceptron_table[index][prefetch_offset].update(ip, coeff, s);
            }

            if (LOGGING) {
                pc_xor_degree_prefetch_offset_perceptron_table[index][prefetch_offset].print_weights();
            }

            if (prev_result == result) {
                pc_xor_degree_prefetch_offset_perceptron_table[index][prefetch_offset].positive_count++;
            }
            pc_xor_degree_prefetch_offset_perceptron_table[index][prefetch_offset].total_count++;
            
            if (useful) {
                degree_accuracy_map[degree].first++;
            }
            degree_accuracy_map[degree].second++;
        }
        else if (PERCEPTRON_DEGREE_PREFETCH_OFFSET_SCORE_MAP) {
            std::set<uint64_t> score_set;
            score_set.insert(score);
            int pred_score = score_weights_map[ip][prefetch_offset].predict(score_set);

            int pred = feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].predict(s);

            int coeff = result ? 1 : -1;

            if (LOGGING) {
                cout << "training with result: " << result << endl;
                cout << "degree: " << degree << endl;
                cout << "offset: " << prefetch_offset << endl;
                for (int i = 0; i < history.size(); i++) {
                    cout << history[i] << ", ";
                }
                cout << endl;
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
            }

            if ((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold) {
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].update(ip, coeff, s);
            }

            if ((pred_score <= 0 && result) || (pred_score > 0 && !result) || std::abs(pred_score) < training_threshold) {
                score_weights_map[ip][prefetch_offset].update(ip, coeff, score_set);
            }

            if (LOGGING) {
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
            }

            if (prev_result == result) {
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].positive_count++;
            }
            
            if (useful) {
                ip_degree_accuracy_map[ip][degree].first++;
                degree_accuracy_map[degree].first++;
            }
        }
        else if (PERCEPTRON_DEGREE_PREFETCH_OFFSET_SCORE) {
            std::set<uint64_t> score_set;
            score_set.insert(score);
            int pred_score = score_weights.predict(score_set);

            int pred = feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].predict(s);

            int coeff = result ? 1 : -1;

            if (LOGGING) {
                cout << "training with result: " << result << endl;
                cout << "degree: " << degree << endl;
                cout << "offset: " << prefetch_offset << endl;
                for (int i = 0; i < history.size(); i++) {
                    cout << history[i] << ", ";
                }
                cout << endl;
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
            }

            if ((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold) {
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].update(ip, coeff, s);
            }

            if ((pred_score <= 0 && result) || (pred_score > 0 && !result) || std::abs(pred_score) < training_threshold) {
                score_weights.update(ip, coeff, score_set);
            }

            if (LOGGING) {
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
            }

            if (prev_result == result) {
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].positive_count++;
            }
            
            if (useful) {
                ip_degree_accuracy_map[ip][degree].first++;
                degree_accuracy_map[degree].first++;
            }

        }
        else if (PERCEPTRON_DEGREE_PREFETCH_OFFSET_SCORE_BUCKET) {
            s.insert(score_category);
            int pred = feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].predict(s);

            int coeff = result ? 1 : -1;

            if (LOGGING) {
                cout << "training with result: " << result << endl;
                cout << "degree: " << degree << endl;
                cout << "offset: " << prefetch_offset << endl;
                for (int i = 0; i < history.size(); i++) {
                    cout << history[i] << ", ";
                }
                cout << endl;
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
            }

            if ((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold) {
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].update(ip, coeff, s);
            }

            if (LOGGING) {
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
            }

            if (prev_result == result) {
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].positive_count++;
            }
            
            if (useful) {
                ip_degree_accuracy_map[ip][degree].first++;
                degree_accuracy_map[degree].first++;
            }
        }
        else if (PERCEPTRON_DEGREE_PREFETCH_OFFSET_PLUS_PAGE) {
            std::set<uint64_t> page;
            page.insert(PAGE(prefetch_trigger_addr_page));
            int pred = feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].predict(s);
            int page_pred = trigger_addr_page_weights.predict(page);

            int coeff = result ? 1 : -1;

            if (LOGGING) {
                cout << "training with result: " << result << endl;
                cout << "degree: " << degree << endl;
                cout << "offset: " << prefetch_offset << endl;
                cout << "pred: " << pred << endl;
                cout << "page: " << PAGE(prefetch_trigger_addr_page) << endl;
                cout << "page pred: " << page_pred << endl;
                for (int i = 0; i < history.size(); i++) {
                    cout << history[i] << ", ";
                }
                cout << endl;
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
            }

            if ((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold) {
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].update(ip, coeff, s);
            }

            if ((page_pred <= 0 && result) || (page_pred > 0 && !result) || std::abs(page_pred) < training_threshold) {
                trigger_addr_page_weights.update(ip, coeff, page);
            }

            if (LOGGING) {
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].print_weights();
                trigger_addr_page_weights.print_weights();
            }

            if (prev_result == result) {
                feature_degree_prefetch_offset_perceptron_table[ip][degree][prefetch_offset].positive_count++;
                trigger_addr_page_weights.positive_count++;
            }
            
            if (useful) {
                ip_degree_accuracy_map[ip][degree].first++;
                degree_accuracy_map[degree].first++;
            }
        }
        else if (PERCEPTRON_DEGREE_PREFETCH_OFFSET_PRED) {
            int pred = feature_degree_prefetch_offset_pred_perceptron_table[ip][degree][prefetch_offset][harmony_pred].predict(s);

            int coeff = result ? 1 : -1;

            if (LOGGING) {
                cout << "training with result: " << result << endl;
                cout << "degree: " << degree << endl;
                for (int i = 0; i < history.size(); i++) {
                    cout << history[i] << ", ";
                }
                cout << endl;
                feature_degree_prefetch_offset_pred_perceptron_table[ip][degree][prefetch_offset][harmony_pred].print_weights();
            }

            if ((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold) {
                feature_degree_prefetch_offset_pred_perceptron_table[ip][degree][prefetch_offset][harmony_pred].update(ip, coeff, s);
            }

            if (LOGGING) {
                feature_degree_prefetch_offset_pred_perceptron_table[ip][degree][prefetch_offset][harmony_pred].print_weights();
            }

            if (prev_result == result) {
                feature_degree_prefetch_offset_pred_perceptron_table[ip][degree][prefetch_offset][harmony_pred].positive_count++;
            }
            
            if (useful) {
                ip_degree_accuracy_map[ip][degree].first++;
                degree_accuracy_map[degree].first++;
            }

        }
        else if (PERCEPTRON_GLOBAL_WEIGHTS) {
            s.insert(degree);
            // s.insert(PAGE(pf_addr));
            int pred = weight_table.predict(s);
            int coeff = result ? 1 : -1;

            if (LOGGING) {
                cout << "training with result: " << result << endl;
                cout << "degree: " << degree << endl;
                for (int i = 0; i < history.size(); i++) {
                    cout << history[i] << ", ";
                }
                cout << endl;
                weight_table.print_weights();
            }
            if ((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold) {
                weight_table.update(ip, coeff, s);
            }

            if (LOGGING) {
                weight_table.print_weights();
            }

            if (prev_result == result) {
                weight_table.positive_count++;
            }
        }
    }
}

void Prefetch_Filter::train(uint64_t ip, bool result, uint64_t pf_addr) {
    std::array<double, NUM_FEATURES> features;


    if (LOGGING) {
        cout << "train ip: " << ip << " result: " << result << endl;
    }

    bool prev_result = true;
    double accuracy = 0.0;
    double lateness = 0.0;
    double pollution = 0.0;
    double dram_availability = 0.0;
    int degree = 0;
    int prefetch_offset = 0;
    if (results.find(pf_addr) != results.end()) {
        accuracy = results[pf_addr].accuracy;
        lateness = results[pf_addr].lateness;
        pollution = results[pf_addr].pollution;
        dram_availability = results[pf_addr].dram_availability;
        degree = results[pf_addr].degree;
        prefetch_offset = results[pf_addr].prefetch_offset;
    }

    features[0] = accuracy;
    features[1] = lateness;
    features[2] = pollution;
    features[3] = dram_availability;

    if (LOGGING) {
        perceptron_table[ip].print_weights();
    }

    int pred = perceptron_table[ip].predict(features);

    if ((pred <= 0 && result) || (pred > 0 && !result) || std::abs(pred) < training_threshold) {
        perceptron_table[ip].update(result, features);
    }

    if (LOGGING) {
        perceptron_table[ip].print_weights();
    } 

    if (PERCEPTRON_PCA) {
        pca_out << ip << ",";
        pca_out << features[0] << ",";
        pca_out << features[1] << ",";
        pca_out << features[2] << ",";
        pca_out << features[3] << ",";
        pca_out << degree << ",";
        pca_out << prefetch_offset << ",";
        pca_out << result << endl;
    }        
    
}

// void Prefetch_Filter::train(uint64_t ip, bool result) {
//     std::array<double, NUM_FEATURES> features;
//     bool train = false;

//     if (LOGGING) {
//         cout << "train ip: " << ip << " result: " << result << endl;
//     }
            
//     if (past_features.find(ip) != past_features.end()) {
//         features = past_features[ip];
//         if (LOGGING) {
//             perceptron_table[ip].print_weights();
//         }

//         perceptron_table[ip].update(result, features);

//         if (LOGGING) {
//             perceptron_table[ip].print_weights();
//         }
//     }
// }

bool Prefetch_Filter::find_past_degrees_not_four(void) {
    for (int i = 0; i < past_degrees.size(); i++) {
        if (past_degrees[i] != 4) {
            return true;
        }
    }
    if (past_degrees.size() == 0) {
        return true;
    }
    return false;
}

bool Prefetch_Filter::is_interval_complete(void) {
    return (num_useful_blocks_evicted >= num_useful_blocks_evicted_threshold);
}

int Prefetch_Filter::reset_interval(uint64_t current_cycle, int current_degree) {
    num_useful_blocks_evicted = 0;

    double pollution = (double) pollution_interval_counter / (double) demand_miss_interval_counter;
    double degree_accuracy = 0.0;
    double accuracy = 0.0;
    double dram_availability = 0.0;
    double lateness = 0.0;
    double bank_occupancy = 0.0;
    int num_demand_miss_interval_counter = demand_miss_interval_counter;
    int num_prefetch_miss_interval_counter = prefetch_miss_interval_counter;
    double demand_miss_latency_avg = (double) demand_miss_latency_interval_counter / (double) demand_miss_interval_counter;
    prefetch_miss_latency_avg = (double) prefetch_miss_latency_interval_counter / (double) prefetch_miss_interval_counter;
    double degree_accuracy_2 = 0.0;
    // if (degree_accuracy_map[current_degree].second > 0) {
        
    // }
    if (degree_accuracy_map[1].second > 0) {
        degree_accuracy = (double) degree_accuracy_map[1].first / (double) degree_accuracy_map[1].second;
    }
    if (degree_accuracy_map[2].second > 0) {
        degree_accuracy_2 = (double) degree_accuracy_map[2].first / (double) degree_accuracy_map[2].second;
    }
    if (prefetch_miss_interval_counter > 0) {
        accuracy = (double) useful_prefetch_interval_counter / (double) prefetch_miss_interval_counter;
    }
    if (prefetch_miss_interval_counter > 0) {
        dram_availability = (double) dram_availability_at_prefetch_issue_interval / (double) prefetch_fill_interval_counter / (double) 64.0;
    }
    if (degree_accuracy_map[current_degree].first > 0) {
        lateness = (double) late_prefetch_interval_counter / (double) useful_prefetch_interval_counter;
    }

    bank_occupancy = (double) bank_occupancy_at_prefetch_issue_interval / (double) prefetch_fill_interval_counter;

    // cout << "degree pollution counter: " << degree_pollution_map[current_degree] << endl;
    // cout << "degree pollution counter: " << degree_pollution_map[3] << endl;
    // cout << "degree pollution counter: " << degree_pollution_map[2] << endl;
    // cout << "degree pollution counter: " << degree_pollution_map[1] << endl;
    cout << "pollution_interval_counter: " << pollution_interval_counter << endl;
    cout << "demand_miss_interval_counter: " << demand_miss_interval_counter << endl;
    cout << "prefetch_miss_interval_counter: " << prefetch_miss_interval_counter << endl;
    cout << "dram availability: " << dram_availability << endl;
    cout << "bank occupancy: " << bank_occupancy << endl;
    cout << "late prefetch interval counter: " << late_prefetch_interval_counter << endl;
    cout << "useful prefetch interval counter: " << useful_prefetch_interval_counter << endl;
    cout << "current_demand_miss_latency_avg: " << current_demand_miss_latency_avg << endl;
    cout << "demand_miss_latency_avg: " << demand_miss_latency_avg << endl;
    cout << "prefetch_miss_latency_avg: " << prefetch_miss_latency_avg << endl;

    cout << "pf filter pollution: " << pollution << endl;
    cout << "pf filter degree accuracy: " << degree_accuracy << endl;
    cout << "pf filter degree accuracy 2: " << degree_accuracy_2 << endl;
    cout << "pf filter prefetcher accuracy: " << accuracy << endl;
    cout << "pf filter lateness: " << lateness << endl;
    cout << "pf filter accuracy: " << (double) accurate_predictions_interval_counter / (double) total_predictions_interval_counter << endl;
    double pf_filter_accuracy = (double) accurate_predictions_interval_counter / (double) total_predictions_interval_counter;

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
    accurate_predictions_interval_counter = 0;
    total_predictions_interval_counter = 0;
    prefetch_fill_interval_counter = 0;
    bank_occupancy_at_prefetch_issue_interval = 0;

    // if (low_degree_accuracy.size() < current_degree) {
    //     for (int i = 1; i <= current_degree; i++) {
    //         low_degree_accuracy.push_back(false);
    //     }
    // }

    // if (degree_accuracy < 0.08 && pf_filter_accuracy < 0.55) {
    //     lo_threshold = -5;
    // }
    

    // for (int i = 1; i <= current_degree; i++) {
    //     double degree_accuracy = (double) degree_accuracy_map[i].first / (double) degree_accuracy_map[i].second;
    //     if (degree_accuracy < 0.03) {
    //         low_degree_accuracy[i-1] = LOW;
    //     }
    //     else if (degree_accuracy < 0.05) {
    //         low_degree_accuracy[i-1] = MIDDLE;
    //     }
    //     else if (degree_accuracy > 0.3) {
    //         low_degree_accuracy[i-1] = HIGH;
    //     }
    // }
    high_lateness = (lateness > 0.03);
    uint64_t useless_correct_temp_interval_counter = useless_correct_interval_counter + (total_predictions_interval_counter_2 - total_training_interval_counter);
    cout << "high_lateness: " << high_lateness << endl;
    cout << "total false: " << total_false << endl;
    cout << "num_false_with_negative_impact: " << num_false_with_negative_impact << endl;
    cout << "percent negative: " << (double) num_false_with_negative_impact / (double) total_false << endl;
    cout << "filter accuracy: " << (double) accurate_predictions_count / (double) total_predictions_count << endl;
    cout << "useful_correct: " << useful_correct_interval_counter << endl;
    cout << "useful_wrong: " << useful_wrong_interval_counter << endl;
    cout << "useless_correct: " << useless_correct_temp_interval_counter << endl;
    cout << "useless_wrong: " << useless_wrong_interval_counter << endl;
    cout << "total_predictions: " << total_predictions_interval_counter_2 << endl;

    cout << "useful_correct_percent: " << (double) useful_correct_interval_counter / (double) total_predictions_interval_counter_2 << endl;
    cout << "useful_wrong_percent: " << (double) useful_wrong_interval_counter / (double) total_predictions_interval_counter_2 << endl;
    cout << "useless_correct_percent: " << (double) useless_correct_temp_interval_counter / (double) total_predictions_interval_counter_2 << endl;
    cout << "useless_wrong_percent: " << (double) useless_wrong_interval_counter / (double) total_predictions_interval_counter_2 << endl;

    // double useless_correct_stat = (double) useless_correct_temp / (double) total_predictions;
    // normal_false_training = (useless_correct_stat > 0.7);

    double useless_misprediction = (double) useless_wrong_interval_counter / (double) (useless_correct_temp_interval_counter + useless_wrong_interval_counter);
    double useful_misprediction = (double) useful_wrong_interval_counter / (double) (useful_wrong_interval_counter + useful_correct_interval_counter);
    double useful_accuracy = (double) useful_correct_interval_counter / (double) useful_wrong_interval_counter;
    double useful_overall = (double) useful_correct_interval_counter / (double) useless_wrong_interval_counter;

    // if (useful_overall < 1.0 && percent_negative > 0.25) {
    //     disable_filter = true;
    //     cout << "Disable filtering!" << endl;

    // }
    // else {
    //     disable_filter = false;
    //     cout << "enable filtering!" << endl;
    // }


    // if (useful_misprediction > (useless_misprediction + 0.05) && dram_availability > 0.8) {
    //     training_type = Training_Type::LOW;
    // }
    // else if (useless_misprediction > useful_misprediction && dram_availability < 0.4) {
    //     training_type = Training_Type::HIGH;
    // }
    // else {
    //     training_type = Training_Type::NORMAL;
    // }

    cout << "new training type: " << training_type << endl;

    // if (!width_testing_done && width) {
    //     width_good_ratio = (double) useful_correct_interval_counter / (double) useless_wrong_interval_counter;
    //     width_testing_done = true;
    //     width = false;
    //     depth = true;
    // }
    // else if (!depth_testing_done && depth) {
    //     depth_good_ratio = (double) useful_correct_interval_counter / (double) useless_wrong_interval_counter;
    //     depth_testing_done = true;
    //     depth = false;
    // }


    num_false_with_negative_impact = 0;
    total_false = 0;
    useless_correct_interval_counter = 0;
    useless_wrong_interval_counter = 0;
    useful_correct_interval_counter = 0;
    useful_wrong_interval_counter = 0;
    total_training_interval_counter = 0;
    total_predictions_interval_counter_2 = 0;


    // if (!decided && width_testing_done && depth_testing_done) {
    //     cout << "width_good_ratio: " << width_good_ratio << endl;
    //     cout << "depth_good_ratio: " << depth_good_ratio << endl;
    //     if (width_good_ratio > depth_good_ratio) {
    //         width = true;
    //         depth = false;
    //         cout << "doing width!" << endl;
    //     }
    //     else {
    //         depth = true;
    //         width = false;
    //         cout << "doing depth!" << endl;
    //     }
    //     decided = true;
    // }
    // if ((degree_accuracy - degree_accuracy_2) > 0.05) {
    //     width_cont = false;
    //     cout << "disabling width cont" << endl;
    // }   

    // degree_accuracy_map[1].first = 0;
    // degree_accuracy_map[1].second = 0;
    // degree_accuracy_map[2].first = 0;
    // degree_accuracy_map[2].second = 0;


    

    // print_ip_degree_accuracy_map();


    if (LOGGING) {
        // print_ip_accuracy_map();
        print_ip_degree_accuracy_map();
        for (auto& item : ip_pollution_map) {
            cout << "ip: " << item.first;
            cout << " pollution: " << item.second;
            cout << " demand miss total: " << demand_miss_total;
            cout << " pollution contribution: " << (double) item.second / (double) demand_miss_total << endl;
        }
    }

    if (PERCEPTRON_DYNAMIC_DEGREE_CONTROL) {

        if ((accuracy > 0.25 && pollution < 0.2 && dram_availability > 0.5 && num_prefetch_miss_interval_counter > 0) || (accuracy > 0.25 && dram_availability > 0.82) || 
            (accuracy > 0.25 && pollution > 0.2 && num_prefetch_miss_interval_counter > 0 && num_prefetch_miss_interval_counter < num_demand_miss_interval_counter)) {
            int new_degree = current_degree;
            if (find_past_degrees_not_four()) {
                new_degree = std::min(6, current_degree+1);
            }
            cout << "new degree: " << new_degree << endl;
            past_degrees.push_back(new_degree);
            return new_degree;
        }
        else {
            int new_degree = std::max(4, current_degree - 1);
            cout << "new degree: " << new_degree << endl;
            past_degrees.push_back(new_degree);
            return new_degree;
        }
    }
    return current_degree;
}

void Prefetch_Filter::print_ip_accuracy_map(void) {
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

void Prefetch_Filter::print_ip_degree_accuracy_map(void) {
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

void Prefetch_Filter::print_degree_accuracy_map(void) {
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
    cout << "prefetcher accuracy: " << (double) useful_prefetch_total / (double) prefetch_miss_total << endl;
}

void Prefetch_Filter::verify_stats(uint64_t useful_prefetch_total, uint64_t prefetch_fill_total, uint64_t late_total, uint64_t pollution_total) {
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

void Prefetch_Filter::print_perceptrons(void) {
    for (auto& item : perceptron_table) {
        cout << "ip: " << item.first << " ";
        item.second.print_weights();
    }

    for (auto& item: feature_perceptron_table) {
        cout << "ip: " << item.first << endl;
        cout << "   accurate: " << item.second.positive_count << " total: " << item.second.total_count << endl;
        cout << "   accuracy: " << (double) item.second.positive_count / (double) item.second.total_count << endl;
    }

    for (auto& item : feature_degree_perceptron_table) {
        cout << "ip: " << item.first << endl;
        for (auto& degree : item.second) {
            cout << "   degree: " << degree.first << endl;
            cout << "   accurate: " << degree.second.positive_count << " total: " << degree.second.total_count << endl;
            cout << "   accuracy: " << (double) degree.second.positive_count / (double) degree.second.total_count << endl;
        }
    }

    // for (auto& item : feature_degree_prefetch_offset_perceptron_table) {
    //     cout << "ip: " << item.first << endl;
    //     for (auto& degree : item.second) {
    //         cout << "degree: " << degree.first << endl;
    //         for (auto& offset : degree.second) {
    //             cout << "offset: " << offset.first << endl;
    //             cout << "   accurate: " << offset.second.positive_count << " total: " << offset.second.total_count << endl;
    //             cout << "   accuracy: " << (double) offset.second.positive_count / (double) offset.second.total_count << endl;
    //         }
    //     }
    // }

    if (PERCEPTRON_PRINT_WEIGHTS) {
        // for (auto& item : feature_degree_prefetch_offset_pred_perceptron_table) {
        //     cout << "ip: " << item.first << endl;
        //     for (auto& degree : item.second) {
        //         cout << "degree: " << degree.first << endl;
        //         for (auto& offset : degree.second) {
        //             cout << "offset: " << offset.first << endl;
        //             for (auto& pred : offset.second) {
        //                 cout << "pred: " << pred.first << endl;
        //                 pred.second.print_weights();
        //                 cout << "accuracy to ground truth: " << (double) pred.second.positive_count / (double) pred.second.total_count << endl;
        //             }
        //         }
        //     }
        // }
        if (PERCEPTRON_DEGREE_PREFETCH_OFFSET_DRAM_OCCUPANCY_IP_HISTORY_WEIGHTS) {
            for (auto& item : dram_occupancy_ip_history_weights) {
                cout << "dram occupancy: " << item.first << endl;
                item.second.print_weights();
                cout << "accuracy to ground truth: " << (double) item.second.positive_count / (double) item.second.total_count;
            }
        }
        if (PERCEPTRON_DEGREE_PREFETCH_OFFSET_DRAM_OCCUPANCY_WEIGHTS) {
            dram_availability_weights.print_weights();
            cout << "accuracy to ground truth: " << (double) dram_availability_weights.positive_count / (double) dram_availability_weights.total_count;
        }
        if (PERCEPTRON_DEGREE_XOR_PREFETCH_OFFSET) {
            for (auto& item : pc_degree_xor_prefetch_offset_perceptron_table) {
                cout << "ip: " << item.first << endl;
                for (auto& degree_xor_offset : item.second) {
                    cout << "index: " << degree_xor_offset.first << endl;
                    degree_xor_offset.second.print_weights();
                    cout << "accuracy to ground truth: " << (double) degree_xor_offset.second.positive_count / (double) degree_xor_offset.second.total_count << endl;
                }
            }
        }
        if (PERCEPTRON_DEGREE_PREFETCH_OFFSET) {
            for (auto& item : feature_degree_prefetch_offset_perceptron_table) {
            cout << "ip: " << item.first << endl;
            for (auto& degree : item.second) {
                cout << "degree: " << degree.first << endl;
                for (auto& offset : degree.second) {
                    cout << "offset: " << offset.first << endl;
                    offset.second.print_weights();
                    cout << "accuracy to ground truth: " << (double) offset.second.positive_count / (double) offset.second.total_count << endl;
                }
            }
        }
        }
        
        // trigger_addr_page_weights.print_weights();
        // for (auto& item : score_weights_map) {
        //     cout << "score weight IP: " << item.first << endl;
        //     for (auto& offset : item.second) {
        //         cout << "prefetch offset: " << offset.first << endl;
        //         offset.second.print_weights();
        //     }
        // }
        if (BEST_FEATURE_TEST_IMPL) {
            for (int i = 0; i < NUM_BEST_FEATURES; i++) {
            cout << "feature: " << i << endl;
            features[i].print_weights();
            cout << "accuracy to ground truth: " << (double) features[i].positive_count / (double) features[i].total_count << endl;
        }
        }

        // if (DRAM_EXTRA_TRAINING) {
        //     cout << "dram occupancy vector" << endl;
        //     for (int i = 0; i < dram_occupancy_global_vector.size(); i++) {
        //         cout << dram_occupancy_global_vector[i] << ", ";
        //     }
        //     cout << endl;
        // }
    }

    // print_ip_degree_accuracy_map();
    print_degree_accuracy_map();
    useless_correct += total_predictions - total_training;
    cout << "high_lateness: " << high_lateness << endl;
    cout << "filter accuracy: " << (double) accurate_predictions_count / (double) total_predictions_count << endl;
    cout << "useful_correct: " << useful_correct << endl;
    cout << "useful_wrong: " << useful_wrong << endl;
    cout << "useless_correct: " << useless_correct << endl;
    cout << "useless_wrong: " << useless_wrong << endl;
    cout << "total_predictions: " << total_predictions << endl;

    cout << "useful_correct_percent: " << (double) useful_correct / (double) total_predictions << endl;
    cout << "useful_wrong_percent: " << (double) useful_wrong / (double) total_predictions << endl;
    cout << "useless_correct_percent: " << (double) useless_correct / (double) total_predictions << endl;
    cout << "useless_wrong_percent: " << (double) useless_wrong / (double) total_predictions << endl;
}

bool Prefetch_Filter::check_reject_table(uint64_t addr) {
    if (reject_table.find(addr) != reject_table.end()) {
        return true;
    }
    return false;
}

void Prefetch_Filter::erase_reject_table(uint64_t addr) {
        reject_table.erase(addr);
}

bool Prefetch_Filter::check_direct_mapped_reject_table(uint64_t addr) {
    int hash = addr % 8192;
    if (direct_mapped_reject_table[hash] > 0) {
        return true;
    }
    return false;
}

void Prefetch_Filter::erase_direct_mapped_reject_table(uint64_t addr) {
    int hash = addr % 8192;
    direct_mapped_reject_table[hash] = 0;
}


bool Prefetch_Filter::check_reject_cache(uint64_t set, uint64_t pf_addr) {
    if (reject_table_cache.find(set) != reject_table_cache.end()) {
        for (auto iter = reject_table_cache[set].begin(); iter != reject_table_cache[set].end(); iter++) {
            if (iter->pf_addr == pf_addr) {
                iter->hit = true;
                return true;
            }
        }
    }
    return false;
}

void Prefetch_Filter::erase_reject_cache(uint64_t set, uint64_t pf_addr) {
    if (reject_table_cache.find(set) != reject_table_cache.end()) {
        for (auto iter = reject_table_cache[set].begin(); iter != reject_table_cache[set].end(); iter++) {
            if (iter->pf_addr == pf_addr) {
                reject_table_cache[set].erase(iter);
                return;
            }
        }
    }
}

void Prefetch_Filter::update_reject_cache(uint64_t set) {
    if (reject_table_cache.find(set) == reject_table_cache.end()) {
        return;
    }
    for (int i = 0; i < reject_table_cache[set].size(); i++) {
        if (reject_table_cache[set][i].rrpv < max_rrpv - 1) {
            reject_table_cache[set][i].rrpv++;
        }
    }
}

void Prefetch_Filter::evict_victim_reject_cache(uint64_t set) {
    if (reject_table_cache.find(set) == reject_table_cache.end()) {
        return;
    }
    for (auto iter = reject_table_cache[set].begin(); iter != reject_table_cache[set].end(); iter++) {
        if (iter->rrpv == max_rrpv) {
            reject_table_cache[set].erase(iter);
            return;
        }
    }

    int max = 0;
    bool found = false;
    auto iter_to_erase = reject_table_cache[set].begin();
    for (auto iter = reject_table_cache[set].begin(); iter != reject_table_cache[set].end(); iter++) {
        if (iter->rrpv >= max ) {
            max = iter->rrpv;
            iter_to_erase = iter;
            found = true;
        }
    }
    if (found) {
        // if (!iter_to_erase->hit) {
        //     train(iter_to_erase->pf_addr, false);
        // }
        reject_table_cache[set].erase(iter_to_erase);
    }
    

}

void Prefetch_Filter::print_reject_cache(uint64_t set) {
    if (reject_table_cache.find(set) == reject_table_cache.end()) {
        return;
    }
    cout << "reject table for set: " << set << endl;
    for (auto iter = reject_table_cache[set].begin(); iter != reject_table_cache[set].end(); iter++) {
        cout << "   pf_addr: " << iter->pf_addr << ", rrpv: " << iter->rrpv << endl;
    }

}

int Prefetch_Filter::get_perceptron_ip_size(void) {
    return feature_degree_perceptron_table.size();
}

void Prefetch_Filter::add_prefetch_victim_addr_pair(uint64_t prefetch_cache_line, uint64_t victim_cache_line) {
    prefetch_victim_pair_map[prefetch_cache_line] = victim_cache_line;
    victim_prefetch_pair_map[victim_cache_line] = prefetch_cache_line;
}

void Prefetch_Filter::invalidate_prefetch_victim_addr_pair(uint64_t prefetch_cache_line) {
    uint64_t victim_addr = prefetch_victim_pair_map[prefetch_cache_line];
    prefetch_victim_pair_map.erase(prefetch_cache_line);
    victim_prefetch_pair_map.erase(victim_addr);
}

PV_Type Prefetch_Filter::check_prefetch_victim(uint64_t cache_line) {
    if (victim_prefetch_pair_map.find(cache_line) != victim_prefetch_pair_map.end()) {
        return VHIT;
    }
    else if (prefetch_victim_pair_map.find(cache_line) != prefetch_victim_pair_map.end()) {
        return PHIT;
    }
    return NO;

}

bool Prefetch_Filter::get_apf_prediction(uint64_t prefetch_cache_line) {
    int index = (prefetch_cache_line >> 23) & 7;
    int pred = FC[index];
    if (pred > 0) {
        return true;
    }
    return false;
}

void Prefetch_Filter::update_apf_with_demand_access(uint64_t demand_access) {
    PV_Type check = check_prefetch_victim(demand_access);
    if (check == PHIT) {
        int index = (demand_access >> 23) & 7;
        FC[index] = std::max(FC[index] - 1, -4);
        invalidate_prefetch_victim_addr_pair(demand_access);
    }
    else if (check == VHIT) {
        uint64_t prefetch_cache_line = victim_prefetch_pair_map[demand_access];
        int index = (prefetch_cache_line >> 23) & 7;
        FC[index] = std::min(FC[index] + 1, 3);
        invalidate_prefetch_victim_addr_pair(prefetch_cache_line);
    }
}

void Prefetch_Filter::update_apf_with_victim_cache_miss(uint64_t victim) {
    PV_Type check = check_prefetch_victim(victim);

    if (check == PHIT) {
        int index = (victim >> 23) & 7;
        FC[index] = std::min(FC[index] + 1, 3);
        invalidate_prefetch_victim_addr_pair(victim);
    }

}

void Prefetch_Filter::add_filtered_prefetch(uint64_t prefetch_cache_line) {
    prefetch_victim_pair_map[prefetch_cache_line] = 0;
}

void Prefetch_Filter::print_FC(void) {
    for (auto& item : FC) {
        cout << "index: " << item.first << " counter: " << item.second << endl;
    }
}

void Prefetch_Filter::get_indices(uint64_t base_addr, uint64_t ip, uint64_t ip_1, uint64_t ip_2, uint64_t ip_3, int32_t prefetch_offset, uint32_t degree, 
double dram_availability, uint64_t perc_set[NUM_BEST_FEATURES], int* dram_bucket, int* pollution_bucket, int* ip_accuracy_bucket, int* lateness_bucket, int* degree_accuracy_bucket,
    double* ip_accuracy, double* lateness, double* pollution, double* degree_accuracy) {
    uint64_t  pre_hash[NUM_BEST_FEATURES];

    // get features
    uint64_t ip_history = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
    *ip_accuracy = (double) ip_accuracy_map[ip].first / (double) ip_accuracy_map[ip].second;
    double ip_lateness = (double) ip_late_map[ip] / (double) ip_accuracy_map[ip].first;
    *lateness = (double) late_prefetch_interval_counter / (double) useful_prefetch_interval_counter;
    *pollution = (double) pollution_interval_counter / (double) demand_miss_interval_counter;
    *degree_accuracy = (double) degree_accuracy_map[degree].first / (double) degree_accuracy_map[degree].second;

    // get buckets for stat features
    *dram_bucket = floor(dram_availability * 100.0);
    *lateness_bucket = floor(*lateness * 100.0);
    *pollution_bucket = floor(*pollution * 100.0);
    *ip_accuracy_bucket = floor(*ip_accuracy * 100.0);
    *degree_accuracy_bucket = floor(*degree_accuracy * 100.0);

    if (LOGGING) {
        cout << "ip: " << ip << endl;
        cout << "ip_accuracy: " << ip_accuracy << endl;
        cout << "ip_lateness: " << ip_lateness << endl;
        cout << "lateness: " << lateness << endl;
        cout << "pollution: " << pollution << endl;
        cout << "dram availability: " << dram_availability << endl;
        cout << "ip_1: " << ip_1 << endl;
        cout << "ip_2: " << ip_2 << endl;
        cout << "ip_3: " << ip_3 << endl;
        cout << "ip_accuracy_bucket: " << *ip_accuracy_bucket << endl;
    } 

    // get pre-hash indices
    pre_hash[0] = ip ^ degree ^ prefetch_offset ^ ip_history;
    // pre_hash[1] = ip_history ^ *dram_bucket;
    // pre_hash[2] = ip_history ^ *pollution_bucket;
    // pre_hash[3] = ip_history ^ *lateness_bucket;
    pre_hash[1] = ip ^ *ip_accuracy_bucket;
    pre_hash[2] = degree ^ *degree_accuracy_bucket;
    // pre_hash[6] = degree ^ *dram_bucket;

    for (int i = 0; i < NUM_BEST_FEATURES; i++) {
        perc_set[i] = pre_hash[i] % FEATURE_DEPTH;
    }
}

void Prefetch_Filter::get_indices_training(uint64_t base_addr, uint64_t ip, uint64_t ip_1, uint64_t ip_2, uint64_t ip_3, int32_t prefetch_offset, uint32_t degree,
        int dram_bucket, int pollution_bucket, int ip_accuracy_bucket, int lateness_bucket, int degree_accuracy_bucket, double dram_availability, double ip_accuracy, 
        double lateness, double pollution, double degree_accuracy, uint64_t perc_set[NUM_BEST_FEATURES]) {
    // get features
    uint64_t  pre_hash[NUM_BEST_FEATURES];

    // get pre-hash indices
    uint64_t ip_history = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
    pre_hash[0] = ip ^ degree ^ prefetch_offset ^ ip_history;
    // pre_hash[1] = PAGE(base_addr);
    // pre_hash[2] = CACHE_LINE(base_addr);
    // pre_hash[1] = ip_history ^ dram_bucket;
    // pre_hash[2] = ip_history ^ pollution_bucket;
    // pre_hash[3] = ip_history ^ lateness_bucket;
    pre_hash[1] = ip ^ ip_accuracy_bucket;
    pre_hash[2] = degree ^ degree_accuracy_bucket;
    // pre_hash[6] = degree ^ dram_bucket;


    for (int i = 0; i < NUM_BEST_FEATURES; i++) {
        perc_set[i] = pre_hash[i] % FEATURE_DEPTH;
    }
}

// void PERCEPTRON_::get_perc_index(uint64_t base_addr, uint64_t ip, uint64_t ip_1, uint64_t ip_2, uint64_t ip_3, int32_t cur_delta, uint32_t depth, uint64_t perc_set[PERC_FEATURES])
//   {
//     // Returns the imdexes for the perceptron tables
//     uint64_t cache_line = base_addr >> LOG2_BLOCK_SIZE,
//              page_addr  = base_addr >> LOG2_PAGE_SIZE;

//     int sig_delta = (cur_delta < 0) ? (((-1) * cur_delta) + (1 << (SIG_DELTA_BIT - 1))) : cur_delta;
//     uint64_t  pre_hash[PERC_FEATURES];

//     pre_hash[0] = base_addr;
//     pre_hash[1] = cache_line;
//     pre_hash[2] = page_addr;
//     pre_hash[3] = ip_1 ^ (ip_2>>1) ^ (ip_3>>2);
//     pre_hash[4] = ip ^ depth;
//     pre_hash[5] = ip ^ sig_delta;

//     for (int i = 0; i < PERC_FEATURES; i++) {
//       perc_set[i] = (pre_hash[i]) % PERC_DEPTH[i]; // Variable depths
//       SPP_DP (
//           cout << "  Perceptron Set Index#: " << i << " = " <<  perc_set[i];
//           );
//     }
//     SPP_DP (
//         cout << endl;
//         );		
//   }

//   int32_t	PERCEPTRON_::perc_predict(uint64_t base_addr, uint64_t ip, uint64_t ip_1, uint64_t ip_2, uint64_t ip_3, int32_t cur_delta, uint32_t depth)
//   {
//     SPP_DP (
//         int sig_delta = (cur_delta < 0) ? (((-1) * cur_delta) + (1 << (SIG_DELTA_BIT - 1))) : cur_delta;
//         cout << "[PERC_PRED] Current IP: " << ip << "  and  Memory Adress: " << hex << base_addr << endl;
//         // cout << " Last Sig: " << last_sig << " Curr Sig: " << curr_sig << dec << endl;
//         // cout << " Cur Delta: " << cur_delta << " Sign Delta: " << sig_delta << " Confidence: " << confidence<< endl;
//         cout << " ";
//         );

//     uint64_t perc_set[PERC_FEATURES];
//     // Get the indexes in perc_set[]
//     get_perc_index(base_addr, ip, ip_1, ip_2, ip_3, cur_delta, depth, perc_set);

//     int32_t sum = 0;
//     for (int i = 0; i < PERC_FEATURES; i++) {
//       sum += perc_weights[perc_set[i]][i];	
//       // Calculate Sum
//     }
//     // cout << "sum: " << sum << ((sum >= PERC_THRESHOLD_LO) ?  ((sum >= PERC_THRESHOLD_HI) ? " fill L2" : " fill LLC") : " suppress")  << endl;
//     SPP_DP (
//         cout << " Sum of perceptrons: " << sum << " Prediction made: " << ((sum >= PERC_THRESHOLD_LO) ?  ((sum >= PERC_THRESHOLD_HI) ? "fill L2" : "fill LLC") : "suppress")  << endl;
//         );
//     // Return the sum
//     return sum;
//   }

//   void 	PERCEPTRON_::perc_update(uint64_t base_addr, uint64_t ip, uint64_t ip_1, uint64_t ip_2, uint64_t ip_3, int32_t cur_delta, uint32_t depth, bool direction, int32_t perc_sum)
//   {
//     SPP_DP (
//         int sig_delta = (cur_delta < 0) ? (((-1) * cur_delta) + (1 << (SIG_DELTA_BIT - 1))) : cur_delta;
//         cout << "[PERC_UPD] (Recorded) IP: " << ip << "  and  Memory Adress: " << hex << base_addr << endl;
//         cout << " Cur Delta: " << cur_delta << " Sign Delta: " << sig_delta << " Update Direction: " << direction << endl;
//         cout << " ";
//         );

//     uint64_t perc_set[PERC_FEATURES];
//     // Get the perceptron indexes
//     get_perc_index(base_addr, ip, ip_1, ip_2, ip_3, cur_delta, depth, perc_set);

//     int32_t sum = 0;
//     for (int i = 0; i < PERC_FEATURES; i++) {
//       // Marking the weights as touched for final dumping in the csv
//       perc_touched[perc_set[i]][i] = 1;	
//     }
//     // Restore the sum that led to the prediction
//     sum = perc_sum;

//     if (!direction) { // direction = 1 means the sum was in the correct direction, 0 means it was in the wrong direction
//       // Prediction wrong
//       for (int i = 0; i < PERC_FEATURES; i++) {
//         if (sum >= PERC_THRESHOLD_HI) {
//           // Prediction was to prefectch -- so decrement counters
//           if (perc_weights[perc_set[i]][i] > -1*(PERC_COUNTER_MAX+1) )
//             perc_weights[perc_set[i]][i]--;
//         }
//         if (sum < PERC_THRESHOLD_HI) {
//           // Prediction was to not prefetch -- so increment counters
//           if (perc_weights[perc_set[i]][i] < PERC_COUNTER_MAX)
//             perc_weights[perc_set[i]][i]++;
//         }
//       }
//       SPP_DP (
//           int differential = (sum >= PERC_THRESHOLD_HI) ? -1 : 1;
//           cout << " Direction is: " << direction << " and sum is:" << sum;
//           cout << " Overall Differential: " << differential << endl;
//           );
//     }
//     if (direction && sum > NEG_UPDT_THRESHOLD && sum < POS_UPDT_THRESHOLD) {
//       // Prediction correct but sum not 'saturated' enough
//       for (int i = 0; i < PERC_FEATURES; i++) {
//         if (sum >= PERC_THRESHOLD_HI) {
//           // Prediction was to prefetch -- so increment counters
//           if (perc_weights[perc_set[i]][i] < PERC_COUNTER_MAX)
//             perc_weights[perc_set[i]][i]++;
//         }
//         if (sum < PERC_THRESHOLD_HI) {
//           // Prediction was to not prefetch -- so decrement counters
//           if (perc_weights[perc_set[i]][i] > -1*(PERC_COUNTER_MAX+1) )
//             perc_weights[perc_set[i]][i]--;
//         }
//       }
//       SPP_DP (
//           int differential = 0;
//           if (sum >= PERC_THRESHOLD_HI) differential =  1;
//           if (sum  < PERC_THRESHOLD_HI) differential = -1;
//           cout << " Direction is: " << direction << " and sum is:" << sum;
//           cout << " Overall Differential: " << differential << endl;
//           );
//     }
//   }

// void PERCEPTRON_::perc_train_from_prefetch_table(uint64_t addr, bool result) {
//     uint64_t cache_line = addr >> LOG2_BLOCK_SIZE,
//     hash = get_hash(cache_line);

//     //MAIN FILTER
//     uint64_t quotient = (hash >> REMAINDER_BIT) & ((1 << QUOTIENT_BIT) - 1),
//             remainder = hash % (1 << REMAINDER_BIT);
//     assert(quotient >= 0 && quotient <= 1023);
    
//     if (prefetch_table[quotient].valid && CACHE_LINE(addr) == CACHE_LINE(prefetch_table[quotient].addr)) {
//         perc_update(prefetch_table[quotient].base_addr, prefetch_table[quotient].ip, prefetch_table[quotient].ip_1, prefetch_table[quotient].ip_2, prefetch_table[quotient].ip_3, prefetch_table[quotient].delta, prefetch_table[quotient].depth, result, prefetch_table[quotient].sum);
//     }
// }

// void PERCEPTRON_::perc_train_from_reject_table(uint64_t addr, bool result) {
//     uint64_t cache_line = addr >> LOG2_BLOCK_SIZE,
//     hash = get_hash(cache_line);

//     //REJECT FILTER
//     uint64_t quotient_reject = (hash >> REMAINDER_BIT_REJ) & ((1 << QUOTIENT_BIT_REJ) - 1),
//     remainder_reject = hash % (1 << REMAINDER_BIT_REJ);
//     assert(quotient_reject >= 0 && quotient_reject <= 1023);
    
//     if (reject_table[quotient_reject].valid) {
//         perc_update(reject_table[quotient_reject].base_addr, reject_table[quotient_reject].ip, reject_table[quotient_reject].ip_1, reject_table[quotient_reject].ip_2, reject_table[quotient_reject].ip_3, reject_table[quotient_reject].delta, reject_table[quotient_reject].depth, 0, reject_table[quotient_reject].sum);
//     }

//     reject_table[quotient_reject].valid = false;
// }

// PPF_PRED PERCEPTRON_::get_perc_prediction(uint64_t base_addr, uint64_t pf_addr, uint64_t ip, uint64_t ip_1, uint64_t ip_2, uint64_t ip_3, int32_t offset, uint32_t degree) {
//     int sum = perc_predict(base_addr, ip, ip_1, ip_2, ip_3, offset, degree);
//     bool do_pf = (sum >= PERC_THRESHOLD_LO) ? 1 : 0;
//     bool fill_l2 = (sum >= PERC_THRESHOLD_HI) ? 1 : 0;

//     uint64_t cache_line = pf_addr >> LOG2_BLOCK_SIZE,
//     hash = get_hash(cache_line);

//     if (do_pf) {
//         //MAIN FILTER
//         uint64_t quotient = (hash >> REMAINDER_BIT) & ((1 << QUOTIENT_BIT) - 1),
//         remainder = hash % (1 << REMAINDER_BIT);

//         prefetch_table[quotient].valid = true;
//         prefetch_table[quotient].addr = pf_addr;
//         prefetch_table[quotient].base_addr = base_addr;
//         prefetch_table[quotient].delta = offset;
//         prefetch_table[quotient].depth = degree;
//         prefetch_table[quotient].ip = ip;
//         prefetch_table[quotient].ip_1 = ip_1;
//         prefetch_table[quotient].ip_2 = ip_2;
//         prefetch_table[quotient].ip_3 = ip_3;
//         prefetch_table[quotient].sum = sum;
//     }
//     else {
//         //REJECT FILTER
//         uint64_t quotient_reject = (hash >> REMAINDER_BIT_REJ) & ((1 << QUOTIENT_BIT_REJ) - 1),
//         remainder_reject = hash % (1 << REMAINDER_BIT_REJ);
    
//         reject_table[quotient_reject].valid = true;
//         reject_table[quotient_reject].addr = pf_addr;
//         reject_table[quotient_reject].base_addr = base_addr;
//         reject_table[quotient_reject].delta = offset;
//         reject_table[quotient_reject].depth = degree;
//         reject_table[quotient_reject].ip = ip;
//         reject_table[quotient_reject].ip_1 = ip_1;
//         reject_table[quotient_reject].ip_2 = ip_2;
//         reject_table[quotient_reject].ip_3 = ip_3;
//         reject_table[quotient_reject].sum = sum;
//     }

//     if (do_pf && fill_l2) {
//         return PREFETCH_L2;
//     }
//     else if (do_pf) {
//         return PREFETCH_LLC;
//     }
//     else {
//         return SUPPRESS;
//     }
// }

// bool PERCEPTRON_::check_reject_table(uint64_t pf_addr) {
//     uint64_t cache_line = pf_addr >> LOG2_BLOCK_SIZE,
//     hash = get_hash(cache_line);

//     //REJECT FILTER
//     uint64_t quotient_reject = (hash >> REMAINDER_BIT_REJ) & ((1 << QUOTIENT_BIT_REJ) - 1),
//     remainder_reject = hash % (1 << REMAINDER_BIT_REJ);

    

//     if (reject_table[quotient_reject].valid) {
//         return cache_line == CACHE_LINE(reject_table[quotient_reject].addr);
//     }
//     return false;


// }

// void PERCEPTRON_::update_prefetch_and_reject_table(uint64_t pf_addr) {
//     uint64_t cache_line = pf_addr >> LOG2_BLOCK_SIZE,
//     hash = get_hash(cache_line);

//     //MAIN FILTER
//     uint64_t quotient = (hash >> REMAINDER_BIT) & ((1 << QUOTIENT_BIT) - 1),
//     remainder = hash % (1 << REMAINDER_BIT);

//     //REJECT FILTER
//     uint64_t quotient_reject = (hash >> REMAINDER_BIT_REJ) & ((1 << QUOTIENT_BIT_REJ) - 1),
//     remainder_reject = hash % (1 << REMAINDER_BIT_REJ);

//     if (CACHE_LINE(pf_addr) == CACHE_LINE(prefetch_table[quotient].addr)) {
//         prefetch_table[quotient].valid = false;
//     }

//     if (CACHE_LINE(pf_addr) == CACHE_LINE(reject_table[quotient_reject].addr)) {
//         reject_table[quotient].valid = false;
//     }
    
// }



// uint64_t PERCEPTRON_::get_hash(uint64_t key)
// {
//   // Robert Jenkins' 32 bit mix function
//   key += (key << 12);
//   key ^= (key >> 22);
//   key += (key << 4);
//   key ^= (key >> 9);
//   key += (key << 10);
//   key ^= (key >> 2);
//   key += (key << 7);
//   key ^= (key >> 12);

//   // Knuth's multiplicative method
//   key = (key >> 3) * 2654435761;

//   return key;
// }

// void PERCEPTRON_::print_perceptron_weights(void) {
//     cout << "base_addr weights" << endl;
//     for (int j = 0; j < PERC_ENTRIES; j++) {
//         cout << perc_weights[j][0] << endl;
//     }
//     cout << endl;

//     cout << "cache line weights" << endl;
//     for (int j = 0; j < PERC_ENTRIES; j++) {
//         cout << perc_weights[j][1] << endl;
//     }
//     cout << endl;

//     cout << "page addr weights" << endl;
//     for (int j = 0; j < PERC_ENTRIES; j++) {
//         cout << perc_weights[j][2] << endl;
//     }
//     cout << endl;

//     cout << "ip_1 ^ ip_2 ^ ip_3 weights" << endl;
//     for (int j = 0; j < PERC_ENTRIES; j++) {
//         cout << perc_weights[j][3] << endl;
//     }

//     cout << "ip ^ degree weights" << endl;
//     for (int j = 0; j < PERC_ENTRIES; j++) {
//         cout << perc_weights[j][4] << endl;
//     }
//     cout << endl;

//     cout << "ip ^ offset weights" << endl;
//     for (int j = 0; j < PERC_ENTRIES; j++) {
//         cout << perc_weights[j][5] << endl;
//     }
//     cout << endl;
// }