#include "tracer.h"
#include <iostream>
#include <utility>
#include <algorithm>

#define PREFETCH 2

bool cmp(std::pair<uint64_t, double>& a,
        std::pair<uint64_t, double>& b)
{
    return a.second > b.second;
}

Tracer::Tracer() {
    num_evicted_before_use_prefetches = 0;
    num_total_prefetches = 0;
    num_total_accesses = 0;
    total_pc_freq = 0;
}

void Tracer::update_pc_map(uint64_t ip, uint64_t cacheline) {
    // update pc freq map
    pc_freq_map[ip]++;
    if (pc_cacheline_map.find(ip) == pc_cacheline_map.end()) {
        std::vector<uint64_t> v;
        pc_cacheline_map[ip] = std::move(v);
    }
    pc_cacheline_map[ip].push_back(cacheline);

    total_pc_freq++;
}

void Tracer::add_new_access(uint64_t signature, bool prediction, bool hit, uint32_t type, uint64_t core_clock_cycle, uint64_t ip) {
    if (interval_map.find(signature) == interval_map.end()) {
        std::vector<Access> v;
        Interval i = {
            .accesses = std::move(v),
            .last_miss_index = -1,
            .current_index = -1,
            .last_demand_index = -1
        };
        interval_map[signature] = i; 
    }
    if (type == PREFETCH) { // prefetch
        Access a = {
            .type = Type::prefetch,
            .prediction = prediction,
            .hit = hit,
            .current_cycle = core_clock_cycle
        };
        interval_map[signature].accesses.push_back(a);
        if (!hit) {
            num_total_prefetch_fills++;
        }
        num_total_prefetches++;
        if (current_k_map[signature] == 0) {
            int predicted_k = k_predictor.get_prediction(signature, pc_history_map[ip]);
            if (predicted_k_map.find(signature) == predicted_k_map.end()) {
                std::vector<Prediction> v;
                predicted_k_map[signature] = std::move(v);
            }
            predicted_k_map[signature].push_back({predicted_k, -1});
        }
        current_k_map[signature]++;
    }
    else {
        Access a = {
            .type = Type::demand,
            .prediction = prediction,
            .hit = hit,
            .current_cycle = core_clock_cycle
        };
        interval_map[signature].accesses.push_back(a);
        if (!hit) {
            num_total_demand_fills++;
        }
        interval_map[signature].last_demand_index = interval_map[signature].current_index + 1;
        num_total_demands++;

        if (current_k_map[signature] > 0) {
            k_predictor.add_training(signature, pc_history_map[ip], current_k_map[signature]);
            if (pc_history_map.find(ip) == pc_history_map.end()) {
                std::queue<uint8_t> history_q;
                pc_history_map[ip] = std::move(history_q); 
            }
            if (pc_history_map[ip].size() == HISTORY_SIZE) {
                pc_history_map[ip].pop();
            }
            pc_history_map[ip].push(current_k_map[signature]);

            if (k_list_map_dyn.find(signature) == k_list_map_dyn.end()) {
                std::vector<int> v;
                k_list_map_dyn[signature] = std::move(v);
            }
            k_list_map_dyn[signature].push_back(current_k_map[signature]);
            // if (predicted_k_map.find(signature) != predicted_k_map.end() && predicted_k_map[signature].size() > 0) {
            //     predicted_k_map[signature].back().actual = current_k_map[signature];
            // }
            predicted_k_map[signature].back().actual = current_k_map[signature];
            current_k_map[signature] = 0;
        }
    }
    interval_map[signature].current_index++;

    if (!hit) {
        interval_map[signature].last_miss_index = interval_map[signature].current_index;
    }
    num_total_accesses++;
}

void Tracer::update_eviction(uint64_t signature, uint64_t current_core_cycle) {
    if (interval_map.find(signature) == interval_map.end()) {
        std::cout << "can't find interval access!" << std::endl;
        return;
    }

    Access a = {
            .type = Type::evicted,
            .prediction = false,
            .hit = false,
            .current_cycle = current_core_cycle
    };
    interval_map[signature].accesses.push_back(a);
    interval_map[signature].current_index++;
}

void Tracer::generate_k_stats(void) {
    for (auto& item : interval_map) {
        int k = 0;
        for (auto& access : item.second.accesses) {
            if (access.type == Type::prefetch) {
                k++;
            }
            else if (access.type != Type::evicted) {
                if (k_list_map.find(item.first) == k_list_map.end()) {
                    std::vector<int> v;
                    k_list_map[item.first] = std::move(v);
                }
                if (k > 0) {
                    k_list_map[item.first].push_back(k);
                    k = 0;
                }
            }
        }
    }
    if (k_list_equals(k_list_map, k_list_map_dyn)) {
        std::cout << "k lists equal!" << std::endl;
    }
    else {
        std::cout << "k lists not equal!" << std::endl;
    }
}

bool Tracer::k_list_equals(std::map<uint64_t, std::vector<int>> a, std::map<uint64_t, std::vector<int>> b) {
    return a.size() == b.size()
        && std::equal(a.begin(), a.end(),
                      b.begin());
}

void Tracer::print_k_stats(void) {
    std::cout << "k_list stats" << std::endl;
    for (auto& item : k_list_map) {
        std::cout << "cacheline: " << item.first << " k_list: ";
        for (auto& k : item.second) {
            std::cout << k << ", ";
        }
        std::cout << std::endl;
    }
}

void Tracer::generate_prefetch_use_stats(void) {
    int num_useful_pd_prefetches = 0;
    int num_useful_pp_prefetches = 0;
    int num_continuing_hit_prefetches = 0;
    int num_useless_prefetches = 0;
    int num_harmony_predict_false = 0;
    int num_total_prefetch_in_interval_map = 0;
    for (auto& item : interval_map) {
        for (int i = 0; i < item.second.current_index + 1; i++) {
            if (item.second.accesses[i].type == Type::prefetch ) {
                num_total_prefetch_in_interval_map++;
            }

            if (item.second.accesses[i].type == Type::prefetch 
                && ((i+1) < (item.second.current_index + 1))
                && item.second.accesses[i+1].type == Type::demand
                && item.second.accesses[i+1].hit) {
                    num_useful_pd_prefetches++;
            }
            else if (item.second.accesses[i].type == Type::prefetch 
                && ((i+1) < (item.second.current_index + 1))
                && item.second.accesses[i+1].type == Type::prefetch
                && item.second.accesses[i+1].hit
                && !item.second.accesses[i].hit) {
                num_useful_pp_prefetches++;
            }
            else if (item.second.accesses[i].type == Type::prefetch 
                && ((i+1) < (item.second.current_index + 1))
                && item.second.accesses[i+1].type == Type::prefetch
                && item.second.accesses[i+1].hit
                && item.second.accesses[i].hit) {
                    num_continuing_hit_prefetches++;
            }
            else if (item.second.accesses[i].type == Type::prefetch ) {
                num_useless_prefetches++;
            }

            if (item.second.accesses[i].type == Type::prefetch 
                && !item.second.accesses[i].prediction) {
                    num_harmony_predict_false++;
            }
        }
    }
    std::cout << "prefetch usefulness stats" << std::endl;
    std::cout << "num total accesses: " << num_total_accesses << std::endl;
    std::cout << "num total prefetches: " << num_total_prefetches << std::endl;
    std::cout << "percent prefetches in accesses: " << num_total_prefetches / (double) num_total_accesses << std::endl;
    std::cout << "num evicted before use prefetches: " << num_evicted_before_use_prefetches << std::endl;
    std::cout << "percent evicted before use prefetches: " << num_evicted_before_use_prefetches / (double)num_total_prefetches << std::endl;
    std::cout << "num useful pd prefetches: " << num_useful_pd_prefetches << std::endl;
    std::cout << "num useful pp prefetches: " << num_useful_pp_prefetches << std::endl;
    std::cout << "num continuing hit prefetches: " << num_continuing_hit_prefetches << std::endl;
    std::cout << "num useless prefetches: " << num_useless_prefetches << std::endl;
    std::cout << "num total prefetches in interval map : " << num_total_prefetch_in_interval_map << std::endl;

    std::cout << "num harmony predict false prefetches: " << num_harmony_predict_false << std::endl;

    std::cout << "percent useful pd prefetches: " << num_useful_pd_prefetches / (double) num_total_prefetches << std::endl;
    std::cout << "percent useful pp prefetches: " << num_useful_pp_prefetches / (double) num_total_prefetches << std::endl;
    std::cout << "percent continuing hit prefetches: " << num_continuing_hit_prefetches / (double) num_total_prefetches << std::endl;
    std::cout << "percent useless prefetches: " << num_useless_prefetches / (double) num_total_prefetches << std::endl;
    std::cout << "percent harmony predict false prefetches: " << num_harmony_predict_false / (double) num_total_prefetches << std::endl;
    
}

void Tracer::print_pc_map_stats(void) {
    std::cout << "PC Freq and cachelines Sorted" << std::endl;
    std::vector<std::pair<uint64_t, double>> sorted_pc_freq; 
    for (const auto& item : pc_freq_map) {
        std::pair<uint64_t, double> p;
        p.first = item.first;
        p.second = (item.second / (double) total_pc_freq) * 100.0;
        sorted_pc_freq.push_back(p);
    }

    std::sort(sorted_pc_freq.begin(), sorted_pc_freq.end(), cmp);

    for (auto& item : sorted_pc_freq) {
        std::cout << "PC: " << item.first << " freq: " << item.second
            << " cachelines: ";
        for (auto& cacheline : pc_cacheline_map[item.first]) {
            std::cout << cacheline << ", " << std::endl;
        }
    }
}

void Tracer::ideal_prefetch_traffic_reduction(void) {
    int prefetch_reduction = 0;
    int useful_prefetch = 0;
    int untimely_prefetch = 0;
    int early_evicted_prefetch = 0;
    int early_prefetch = 0;

    int demand_prefetch_reduction = 0;
    int demand_useful_prefetch = 0;
    int demand_untimely_prefetch = 0;
    int demand_early_evicted_prefetch = 0;
    int demand_early_prefetch = 0;
    int total_prefetch_fills_in_demand = 0;

    for (auto& item : interval_map) {
        bool prefetch_line_filled = false;
        int num_demands_in_line = 0;
        int num_prefetch_hits_in_line = 0;
        for (int i = 0; i < item.second.last_demand_index + 1; i++) {
            if (item.second.accesses[i].type == Type::prefetch
                && !item.second.accesses[i].hit) {
                    total_prefetch_fills_in_demand++;

                    if (!prefetch_line_filled) {
                        prefetch_line_filled = true;
                    } 
            }
            else if (item.second.accesses[i].type == Type::demand
                && item.second.accesses[i].hit) {
                if (prefetch_line_filled) {
                    num_demands_in_line++;
                }
            }
            else if (item.second.accesses[i].type == Type::evicted
                && prefetch_line_filled) {
                if (num_demands_in_line == 0) {
                    prefetch_reduction++;
                    if (num_prefetch_hits_in_line > 0) {
                        early_evicted_prefetch++;
                    }
                }
                else if (num_demands_in_line > 0) {
                    useful_prefetch++;
                    if (num_prefetch_hits_in_line > 0) {
                        early_prefetch++;
                    }
                }
                prefetch_line_filled = false;
                num_demands_in_line = 0;

            }
            else if (item.second.accesses[i].type == Type::prefetch
                && item.second.accesses[i].hit
                && prefetch_line_filled) {
                num_prefetch_hits_in_line++;
            }
        }
    }

    std::cout << "num total prefetch fills: " << num_total_prefetch_fills << std::endl;
    std::cout << "num total prefetch fills with demand: " << total_prefetch_fills_in_demand << std::endl;
    std::cout << "percent prefetch fills with demand: " << total_prefetch_fills_in_demand / (double) num_total_prefetch_fills << std::endl;
    std::cout << "percent prefetch reduction traffic: " << prefetch_reduction / (double) num_total_prefetch_fills  << std::endl;
    std::cout << "percent prefetch reduction total: " << prefetch_reduction / (double) num_total_prefetches  << std::endl;
    std::cout << "prefetch reduction total: " << prefetch_reduction << std::endl;
    std::cout << "percent prefetch reduction demand total: " << prefetch_reduction / (double) total_prefetch_fills_in_demand  << std::endl;

    std::cout << "useful prefetch: " << useful_prefetch << std::endl;
    std::cout << "early prefetch: " << early_prefetch << std::endl;
    std::cout << "early evicted prefetch: " << early_evicted_prefetch << std::endl;
    

    std::cout << "percent useful prefetch traffic: " << useful_prefetch / (double) num_total_prefetch_fills << std::endl;
    std::cout << "percent early prefetch traffic: " << early_prefetch / (double) num_total_prefetch_fills << std::endl;
    std::cout << "percent early evicted prefetch traffic: " << early_evicted_prefetch / (double) num_total_prefetch_fills << std::endl;
}

void Tracer::generate_prefetch_interval(void) {
    int prefetch_miss_evict_interval = 0;
    int prefetch_hit_evict_interval = 0;
    int prefetch_hit_prefetch_hit = 0;
    int prefetch_miss_prefetch_hit = 0;
    int prefetch_miss_demand_hit = 0;
    int prefetch_hit_demand_hit = 0;
    int prefetch_miss_end_interval = 0;
    int prefetch_hit_end_interval = 0;

    int prefetch_miss_demand_miss = 0;

    int num_total_prefetch_interval = 0;
    int num_total_interval = 0;

    int prefetch_traffic_reduction = 0;
    int prefetch_traffic_reduction_all = 0;
    int num_total_prefetch_fills = 0;
    int num_total_prefetch_fills_with_demand = 0;

    int num_prefetch_hits_in_line = 0;

    std::map<uint64_t, std::vector<int>> ph_in_line;

    for (auto& item : interval_map) {
        int num_demands_in_line = 0;
        bool prefetch_line_filled = false;
        for (int i = 0; i < item.second.current_index+1; i++) {
            if (item.second.accesses[i].type != Type::evicted) {
                num_total_interval++;
            }
            if (item.second.accesses[i].type == Type::prefetch) {
                num_total_prefetch_interval++;
                if (!item.second.accesses[i].hit) {
                    num_total_prefetch_fills++;
                    prefetch_line_filled = true;
                    if (i < item.second.last_demand_index + 1) {
                        num_total_prefetch_fills_with_demand++;
                    }
                    if ((i+1) < (item.second.current_index + 1)) {
                        if (item.second.accesses[i+1].type == Type::prefetch
                        && item.second.accesses[i+1].hit) {
                            prefetch_miss_prefetch_hit++;
                            num_prefetch_hits_in_line++;
                    }
                        else if (item.second.accesses[i+1].type == Type::evicted) {
                            prefetch_miss_evict_interval++;
                    }
                        else if (item.second.accesses[i+1].type == Type::demand
                            && item.second.accesses[i+1].hit) {
                                prefetch_miss_demand_hit++;
                    }
                    else if (item.second.accesses[i+1].type == Type::demand
                            && !item.second.accesses[i+1].hit) {
                                prefetch_miss_demand_miss++;
                    }
                    }
                    else {
                        prefetch_miss_end_interval++;
                        prefetch_traffic_reduction_all++;
                        if (i < item.second.last_demand_index + 1) {
                            prefetch_traffic_reduction++;
                        }
                        prefetch_line_filled = false;
                    }

                }
                else {
                    if ((i+1) < (item.second.current_index + 1)) {
                        if (item.second.accesses[i+1].type == Type::prefetch
                        && item.second.accesses[i+1].hit) {
                            prefetch_hit_prefetch_hit++;
                        }
                        else if (item.second.accesses[i+1].type == Type::evicted) {
                            prefetch_hit_evict_interval++;
                        }
                        else if (item.second.accesses[i+1].type == Type::demand
                            && item.second.accesses[i+1].hit) {
                                prefetch_hit_demand_hit++;
                        }
                    }
                    else {
                        prefetch_hit_end_interval++;
                        if (prefetch_line_filled) {
                            if (num_demands_in_line == 0) {
                                prefetch_traffic_reduction_all++;
                                if (i < item.second.last_demand_index + 1) {
                                    prefetch_traffic_reduction++;
                                }
                            }
                            prefetch_line_filled = false;
                            num_demands_in_line = 0;
                        }
                    }
                }
            }
            else if (item.second.accesses[i].type == Type::demand
                && item.second.accesses[i].hit) {
                    if (prefetch_line_filled) {
                        num_demands_in_line++;
                        if (num_prefetch_hits_in_line > 0) {
                            if (ph_in_line.find(item.first) == ph_in_line.end()) {
                                std::vector<int> v;
                            }
                            ph_in_line[item.first].push_back(num_prefetch_hits_in_line);
                        }
                        num_prefetch_hits_in_line = 0;
                    }
            }
            else if (item.second.accesses[i].type == Type::evicted) {
                if (prefetch_line_filled) {
                    if (i < item.second.last_demand_index + 1) {
                        if (num_demands_in_line == 0) {
                            prefetch_traffic_reduction++;
                        }
                    }
                    if (num_demands_in_line == 0) {
                        prefetch_traffic_reduction_all++;
                    }
                    prefetch_line_filled = false;
                    num_demands_in_line = 0;
                }
            }
        }
    }

    std::cout << "num total interval: " << num_total_interval << std::endl;
    std::cout << "num total prefetch intervals: " << num_total_prefetch_interval << std::endl;
    std::cout << "percent prefetch interval: " << num_total_prefetch_interval / (double) num_total_interval << std::endl;
    std::cout << "num prefetch_miss_evict_interval: " << prefetch_miss_evict_interval << std::endl;
    std::cout << "num prefetch_hit_evict_interval: " << prefetch_hit_evict_interval << std::endl;
    std::cout << "num prefetch_miss_prefetch_hit: " << prefetch_miss_prefetch_hit << std::endl;
    std::cout << "num prefetch_hit_prefetch_hit: " << prefetch_hit_prefetch_hit << std::endl;
    std::cout << "num prefetch_miss_demand_hit: " << prefetch_miss_demand_hit << std::endl;
    std::cout << "num prefetch_hit_demand_hit: " << prefetch_hit_demand_hit << std::endl;
    std::cout << "num prefetch_miss_end_interval: " << prefetch_miss_end_interval << std::endl;
    std::cout << "num prefetch_hit_end_interval: " << prefetch_hit_end_interval << std::endl;
    std::cout << "num prefetch_miss_demand_miss: " << prefetch_miss_demand_miss << std::endl;

    double percent_prefetch_miss_evict_interval = prefetch_miss_evict_interval / (double) num_total_prefetch_interval;
    double percent_prefetch_hit_evict_interval = prefetch_hit_evict_interval / (double) num_total_prefetch_interval;
    double percent_prefetch_miss_prefetch_hit = prefetch_miss_prefetch_hit / (double) num_total_prefetch_interval;
    double percent_prefetch_hit_prefetch_hit = prefetch_hit_prefetch_hit / (double) num_total_prefetch_interval;
    double percent_prefetch_miss_demand_hit = prefetch_miss_demand_hit / (double) num_total_prefetch_interval;
    double percent_prefetch_hit_demand_hit = prefetch_hit_demand_hit / (double) num_total_prefetch_interval;
    double percent_prefetch_miss_end_interval = prefetch_miss_end_interval / (double) num_total_prefetch_interval;
    double percent_prefetch_hit_end_interval = prefetch_hit_end_interval / (double) num_total_prefetch_interval;
    double percent_prefetch_miss_demand_miss = prefetch_miss_demand_miss / (double) num_total_prefetch_interval;
    std::cout << "percent prefetch_miss_evict_interval: " << percent_prefetch_miss_evict_interval << std::endl;
    std::cout << "percent prefetch_hit_evict_interval: " << percent_prefetch_hit_evict_interval << std::endl;
    std::cout << "percent prefetch_miss_prefetch_hit: " << percent_prefetch_miss_prefetch_hit << std::endl;
    std::cout << "percent prefetch_hit_prefetch_hit: " << percent_prefetch_hit_prefetch_hit << std::endl;
    std::cout << "percent prefetch_miss_demand_hit: " << percent_prefetch_miss_demand_hit << std::endl;
    std::cout << "percent prefetch_hit_demand_hit: " << percent_prefetch_hit_demand_hit << std::endl;
    std::cout << "percent prefetch_miss_end_interval: " << percent_prefetch_miss_end_interval << std::endl;
    std::cout << "percent prefetch_hit_end_interval: " << percent_prefetch_hit_end_interval << std::endl;
    std::cout << "percent prefetch_miss_demand_miss: " << percent_prefetch_miss_demand_miss << std::endl;

    double sum = percent_prefetch_miss_evict_interval + percent_prefetch_hit_evict_interval + 
        percent_prefetch_miss_prefetch_hit + percent_prefetch_hit_prefetch_hit + percent_prefetch_miss_demand_hit + 
        percent_prefetch_hit_demand_hit + percent_prefetch_miss_end_interval + percent_prefetch_hit_end_interval;
    int sum_interval = prefetch_miss_evict_interval + prefetch_hit_evict_interval + prefetch_miss_prefetch_hit
        + prefetch_hit_prefetch_hit + prefetch_miss_demand_hit + prefetch_hit_demand_hit + prefetch_miss_end_interval + prefetch_hit_end_interval;
    std::cout << "prefetch interval percent sum: " << sum << std::endl;
    std::cout << "prefetch interval sum: " << sum_interval << std::endl;

    std::cout << "num total prefetch fills: " << num_total_prefetch_fills << std::endl;
    std::cout << "num total prefetch fills with demand: " << num_total_prefetch_fills_with_demand << std::endl;
    std::cout << "percent prefetch fills with demand: " << num_total_prefetch_fills_with_demand / (double) num_total_prefetch_fills << std::endl;

    std::cout << "prefetch traffic reduction total: " << prefetch_traffic_reduction << std::endl;
    std::cout << "prefetch traffic reduction all total: " << prefetch_traffic_reduction_all << std::endl;
    std::cout << "percent prefetch reduction traffic: " << prefetch_traffic_reduction / (double) num_total_prefetch_fills  << std::endl;
    std::cout << "percent prefetch reduction traffic all: " << prefetch_traffic_reduction_all / (double) num_total_prefetch_fills  << std::endl;
    std::cout << "percent prefetch reduction traffic all total prefetch interval: " << prefetch_traffic_reduction_all / (double) num_total_prefetch_interval  << std::endl;
    std::cout << "percent prefetch reduction demand: " << prefetch_traffic_reduction / (double) num_total_prefetch_fills_with_demand  << std::endl;

    // std::cout << "ph in line distribution" << std::endl;
    // for (auto& item : ph_in_line) {
    //     std::cout << "cacheline: " << item.first << " prefetch_hit_k: ";
    //     for (auto& k : item.second) {
    //         std::cout << k << " ";
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << std::endl;
    
}

void Tracer::get_predictor_accuracy(void) {
    int total = 0;
    int accurate = 0;
    std::cout << "in get predictor accuracy" << std::endl;
    for (auto& item : predicted_k_map) {
        for (int i = 0; i < item.second.size(); i++) {
            if (item.second[i].actual > 0 && item.second[i].k_prediction == item.second[i].actual) {
                accurate++;
                total++;
            }
            else if (item.second[i].actual > 0) {
                total++;
            }
        }
    }
    std::cout << "accuracy: " << (double)accurate / (double) total << std::endl;
}

void Tracer::generate_timeliness_stats(void) {
    std::map<uint64_t, std::vector<uint64_t>> timeliness_map;
    for (auto& item : interval_map) {
        for (int i = 0; i < item.second.current_index+1; i++) {
            if (item.second.accesses[i].type == Type::prefetch
                && i < item.second.last_demand_index + 1
                && item.second.accesses[i+1].type == Type::demand
                && item.second.accesses[i+1].hit) {
                    if (timeliness_map.find(item.first) == timeliness_map.end()) {
                        std::vector<uint64_t> v;
                        timeliness_map[item.first] = std::move(v);
                    }
                    uint64_t time = item.second.accesses[i+1].current_cycle - item.second.accesses[i].current_cycle;
                    timeliness_map[item.first].push_back(time);
                }
        }
    }

    // print timeliness map
    for (auto& item : timeliness_map) {
        std::cout << "cacheline: " << item.first << " timeliness: ";
        for (int i = 0; i < item.second.size(); i++) {
            std::cout << item.second[i] << ", ";
        }
        std::cout << std::endl;
    }
}




