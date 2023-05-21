#ifndef TRACER_H
#define TRACER_H

#include <map>
#include <vector>
#include <queue>
#include <set>
#include "prefetch_reduction_predictor.h"

enum Type { 
    prefetch, 
    demand,
    evicted
};

struct Access {
    Type type;
    bool prediction;
    bool hit;
    uint64_t current_cycle;
};

struct Interval {
    std::vector<Access> accesses;
    int last_miss_index;
    int current_index;
    int last_demand_index;
    int prefetches_in_accesses;
};

struct Prediction {
    int k_prediction;
    int actual;
};

#define HISTORY_SIZE 4

class Tracer {
    public:
        std::map<uint64_t, Interval> interval_map;
        std::map<uint64_t, std::queue<uint8_t>> pc_history_map;
        std::map<uint64_t, std::vector<Prediction>> predicted_k_map;
        Prefetch_Reduction_Predictor k_predictor;
        uint64_t num_evicted_before_use_prefetches;
        uint64_t num_total_prefetches;
        uint64_t num_total_accesses;
        uint64_t num_total_prefetch_fills;
        uint64_t num_total_demand_fills;
        uint64_t num_total_demands;
        std::map<uint64_t, std::vector<int>> k_list_map;
        std::map<uint64_t, std::vector<int>> k_list_map_dyn;

        std::map<uint64_t, uint64_t> pc_freq_map;
        std::map<uint64_t, std::vector<uint64_t>> pc_cacheline_map;
        uint64_t total_pc_freq;

        std::map<uint64_t, pair<uint8_t, uint64_t>> current_k_map;

        std::map<uint64_t, std::vector<int>> ip_k_map;

        std::map<uint64_t, pair<int, int>> cacheline_accuracy_map;
        std::map<uint64_t, pair<int, int>> ip_accuracy_map;
        std::map<uint64_t, int> ip_late_map;

        std::map<uint64_t, pair<bool, uint64_t>> current_k_pmiss_map;

        double accuracy_threshold = 0.1;

        uint64_t late_total;
        uint64_t accurate_prefetch_total;
        uint64_t useful_prefetch_total;
        uint64_t pollution_total;

        uint64_t memory_request_total;

        uint64_t memory_request_interval_counter;

        uint64_t useful_prefetch_interval_counter;
        uint64_t prefetch_fill_interval_counter;
        uint64_t prefetch_fill_total;
        uint64_t late_interval_counter;
        uint64_t prefetch_miss_total;

        uint64_t dram_occupancy_at_prefetch_issue;
        uint64_t num_issued_prefetches_interval;
        uint64_t dram_availability_at_prefetch_issue;
        int dram_rq_size;
        bool get_dram_rq_size = false;

        uint64_t cycle_coverage = 0;
        uint64_t cycle_coverage_interval = 0;

        std::vector<int> dram_latencies_vector;
        
        Tracer();
        void add_new_access(uint64_t signature, bool prediction, bool hit, uint32_t type, uint64_t core_clock_cycle, uint64_t pc);
        void update_eviction(uint64_t signature, uint64_t core_clock_cycle, uint64_t ip);
        void update_pc_map(uint64_t ip, uint64_t cacheline);
        void generate_k_stats(void);
        void print_k_stats(void);
        void generate_prefetch_use_stats(void);
        void print_pc_map_stats(void);
        void ideal_prefetch_traffic_reduction(void);

        void generate_prefetch_interval(void);

        void get_predictor_accuracy(void);

        bool k_list_equals(std::map<uint64_t, std::vector<int>> a, std::map<uint64_t, std::vector<int>> b);

        void generate_timeliness_stats(void);

        void print_pc_freq_map_prefetch(void);

        void print_ip_k_map_stats(void);

        uint8_t get_current_k(uint64_t cacheline);

        int get_accurate_counter_cacheline(uint64_t cacheline);

        int get_total_counter_cacheline(uint64_t cacheline);

        int get_accurate_counter_ip(uint64_t ip);

        int get_total_counter_ip(uint64_t ip);

        void clear_cacheline_accuracy_map(void);

        void clear_ip_accuracy_map(void);

        void print_ip_accuracy_map(void);

        void print_cacheline_accuracy_map(void);

        void generate_prefetcher_accuracy(void);

        void final_prefetcher_accuracy(void);

        void final_prefetcher_lateness(void);

        void print_accesses(std::vector<Access> v);

        void print_memory_request_total(void);

        void print_dram_stats(void);

};
#define CACHE_LINE(a) (a >> LOG2_BLOCK_SIZE)

extern Tracer tracer;
#endif