#ifndef FDP_CONTROL_H
#define FDP_CONTROL_H

#include <stdint.h>
#include <map>
#include <vector>

using namespace std;

enum Control_Response { 
    INCREMENT, 
    DECREMENT,
    NO_CHANGE
};

struct FDP_Interval {
    Control_Response r;
    int degree;
};

#define FDP false
#define FDP_INTERVAL false
#define FDP_LOGGING false
#define DRAM_AVAILABILITY false
#define FDP_COVERAGE false
#define COST_MODEL false
#define COST_MODEL_INTERVAL false
#define ALTERNATIVE_FDP false
#define MISS_LATENCY_FDP false


class FDP_Control {
    double accuracy_low_threshold = 0.20;
    double accuracy_high_threshold = 0.25;

    double lateness_threshold = 0.15;

    double pollution_low_threshold = 0.005;
    double pollution_high_threshold = 0.20;

    int num_useful_blocks_evicted_threshold = 16384*5;

    double dram_availability_threshold = 0.80;

    public:
    FDP_Control() {
        interval_start = 0;
    }

    uint64_t useful_prefetch_total;
    uint64_t prefetch_fill_total;

    uint64_t late_prefetch_total;

    // uint64_t llc_miss_counter;
    // uint64_t total_llc_misses;
    uint64_t pollution_total;
    // uint64_t pollution_interval_counter;
    // uint64_t demand_miss_interval_counter;
    uint64_t demand_miss_total;
    uint64_t memory_request_total;
    uint64_t interval_start;
    map<uint64_t, bool> pollution_map;


    uint64_t useful_prefetch_interval_counter;
    uint64_t late_prefetch_interval_counter;
    uint64_t prefetch_fill_interval_counter;
    uint64_t demand_miss_interval_counter;
    uint64_t pollution_interval_counter;
    uint64_t memory_request_interval_counter;
    uint64_t dram_occupancy_at_prefetch_issue;
    uint64_t dram_availability_at_prefetch_issue;
    uint64_t dram_availability_at_prefetch_issue_interval;
    uint64_t dram_occupancy_at_prefetch_issue_interval;
    uint64_t demand_miss_latency_interval_counter;
    uint64_t prefetch_miss_latency_interval_counter;
    uint64_t prefetch_miss_interval_counter;
    double current_demand_miss_latency_avg = 0.0;
    int dram_rq_size;
    bool get_dram_rq_size = false;

    int num_useful_blocks_evicted;

    double net_cost_interval = 0;
    double net_cost_total = 0;

    uint64_t num_demand_LLC_misses_total;
    uint64_t num_demand_LLC_misses_interval_counter;
    uint64_t coverage_interval_counter;
    uint64_t coverage_total;



    vector<FDP_Interval> fdp_interval_vec;

    Control_Response prefetcher_control(uint64_t current_cycle, uint64_t useful_prefetch_count, uint64_t prefetch_total);

    void print_stats(void);

    bool is_interval_complete(void);
    void reset_interval(uint64_t current_cycle);

    uint64_t get_interval_start_cycle(void);

    void demand_hit_prefetched_block(double cycles);

    void late_prefetch(double cycles);

    void cache_pollution(double cycles);

    void useless_prefetch(double cycles);

    


};

#endif