#include "fdp_control.h"
#include <iostream>

using namespace std;
Control_Response FDP_Control::prefetcher_control(uint64_t current_cycle, uint64_t useful_prefetch_count, uint64_t prefetch_total) {
    if (MISS_LATENCY_FDP) {
        double accuracy = (double) useful_prefetch_count / (double) prefetch_total;
        double demand_miss_latency_avg = (double) demand_miss_latency_interval_counter / (double) demand_miss_interval_counter;
        double prefetch_miss_latency_avg = (double) prefetch_miss_latency_interval_counter / (double) prefetch_miss_interval_counter;

        cout << "current_demand_miss_latency_avg: " << current_demand_miss_latency_avg << endl;
        cout << "demand_miss_latency_avg: " << demand_miss_latency_avg << endl;
        cout << "prefetch_miss_latency_avg: " << prefetch_miss_latency_avg << endl;

        if (accuracy > 0.25 && current_demand_miss_latency_avg == 0.0) {
            return INCREMENT;
        }
        else if (demand_miss_latency_avg > current_demand_miss_latency_avg) {
            return DECREMENT;
        }
        else if (accuracy > 0.25 && demand_miss_latency_avg < current_demand_miss_latency_avg) {
            return INCREMENT;
        }
        else {
            return NO_CHANGE;
        }
    }

    double accuracy = 0.0;
    double lateness = 0.0;
    double pollution = 0.0;
    double dram_availability = 0.0;
    double coverage = 0.0;
    if (FDP) {
        if (prefetch_fill_total > 0) {
        accuracy = (double) useful_prefetch_total / (double) prefetch_fill_total;
        double dram_availability_avg = (double) dram_availability_at_prefetch_issue / (double) prefetch_fill_total;
        dram_availability = dram_availability_avg / (double) dram_rq_size;
        
        }
        if (useful_prefetch_total > 0) {
            lateness = (double) late_prefetch_total / (double) useful_prefetch_total;
        }
        if (demand_miss_total > 0) {
            pollution = (double) pollution_total / (double) demand_miss_total;
        }

        if (num_demand_LLC_misses_total > 0) {
            coverage = (double) coverage_total / (double) num_demand_LLC_misses_total;
        }
    }
    // 
    if (FDP_INTERVAL) {
        // double accuracy_total = 0.5 * ( (double) useful_prefetch_total ) + 0.5 * ( (double) useful_prefetch_interval_counter );
        // double lateness_total = 0.5 * ( (double) late_prefetch_total ) + 0.5 * ( (double) late_prefetch_interval_counter );
        // double pollution_total = 0.5 * ( (double) pollution_total ) + 0.5 * ( (double) pollution_interval_counter );
        // double demand_m_total = 0.5 * ( (double) demand_miss_total ) + 0.5 * ( (double) demand_miss_interval_counter );
        // double prefetch_f_total = 0.5 * ( (double) prefetch_fill_total ) + 0.5 * ( (double) prefetch_fill_interval_counter );

        double accuracy_total;
        double lateness_total;
        double pollution_total;
        double demand_m_total;
        double prefetch_f_total;
        double dram_availability_total;
        
        double accuracy_interval;
        double lateness_interval;
        double pollution_interval;
        double dram_availability_interval;
        double coverage_interval;
        if (prefetch_fill_interval_counter > 0) {
        accuracy_interval = (double) useful_prefetch_count / (double) prefetch_total;
        double dram_availability_avg = (double) dram_availability_at_prefetch_issue_interval / (double) prefetch_fill_interval_counter;
        dram_availability_interval = dram_availability_avg / (double) dram_rq_size;
        }
        if (useful_prefetch_interval_counter > 0) {
            lateness_interval = (double) late_prefetch_interval_counter / (double) useful_prefetch_interval_counter;
        }
        if (demand_miss_interval_counter > 0) {
            pollution_interval = (double) pollution_interval_counter / (double) demand_miss_interval_counter;
        }
        if (num_demand_LLC_misses_interval_counter) {
            coverage_interval = (double) coverage_interval_counter / (double) num_demand_LLC_misses_interval_counter;
        }

        if (prefetch_fill_total > 0) {
        accuracy = (double) useful_prefetch_total / (double) prefetch_fill_total;
        double dram_availability_avg = (double) dram_availability_at_prefetch_issue / (double) prefetch_fill_total;
        dram_availability_total = dram_availability_avg / (double) dram_rq_size;
        
        }
        if (useful_prefetch_total > 0) {
            lateness = (double) late_prefetch_total / (double) useful_prefetch_total;
        }
        if (demand_miss_total > 0) {
            pollution = (double) pollution_total / (double) demand_miss_total;
        }


        // accuracy = (0.5 * accuracy_interval + 0.5 * accuracy_total);
        // lateness = (0.5 * lateness_interval + 0.5 * lateness_total);
        // pollution = (0.5 * pollution_interval + 0.5 * pollution_total);
        // dram_availability = (0.5 * dram_availability_interval + 0.5 * dram_availability_total);

        accuracy = accuracy_interval;
        lateness = lateness_interval;
        pollution = pollution_interval;
        dram_availability = dram_availability_interval;
        coverage = coverage_interval;
    }

    if (FDP_LOGGING) {
        cout << "useful prefetch total: " << useful_prefetch_total << endl;
        cout << "late prefetch total: " << late_prefetch_total << endl;
        cout << "prefetch fill total: " << prefetch_fill_total << endl;
        cout << "pollution total: " << pollution_total << endl;
        cout << "demand miss total: " << demand_miss_total << endl;
        cout << "dram availability at prefetch issue: " << dram_availability_at_prefetch_issue << endl;
        cout << "num demand LLC misses total: " << num_demand_LLC_misses_total << endl;
        cout << "coverage total: " << coverage_total << endl;

        cout << "useful prefetch interval counter: " << useful_prefetch_interval_counter << endl;
        cout << "late_prefetch_interval_counter: " << late_prefetch_interval_counter << endl;
        cout << "pollution interval late counter: " << pollution_interval_counter << endl;
        cout << "demand miss interval counter: " << demand_miss_interval_counter << endl;
        cout << "prefetch_fill_interval_counter: " << prefetch_fill_interval_counter << endl;
        cout << "dram availability at prefetch issue interval: " << dram_availability_at_prefetch_issue_interval << endl;
        cout << "num demand LLC misses interval counter: " << num_demand_LLC_misses_interval_counter << endl;
        cout << "coverage interval counter: " << coverage_interval_counter << endl;

        cout << "accuracy: " << accuracy << endl;
        cout << "lateness: " << lateness << endl;
        cout << "pollution: " << pollution << endl;
        cout << "dram availability: " << dram_availability << endl;
        cout << "coverage: " << coverage << endl;

        if (prefetch_fill_total > 0) {
            cout << "accuracy with total counters: " << (double) useful_prefetch_total / (double) prefetch_fill_total << endl;
        }
        if (useful_prefetch_total > 0) {
            cout << "lateness with total counters: " << (double) late_prefetch_total / (double) useful_prefetch_total << endl;
        }
        if (demand_miss_total > 0) {
            cout << "pollution with total counters: " << (double) pollution_total / (double) demand_miss_total << endl;
        }
        cout << "cycles elapsed: " << (current_cycle - interval_start) << endl;
        cout << "memory requests per thousand cycles: " << ((double) memory_request_interval_counter / (double) (current_cycle - interval_start)) * 1000.0 << endl;

        cout << "accuracy with interval counters: " << (double) useful_prefetch_interval_counter / (double) prefetch_fill_interval_counter << endl;
        cout << "lateness with interval counters: " << (double) late_prefetch_interval_counter / (double) useful_prefetch_interval_counter << endl;
        cout << "pollution with interval counters: " << (double) pollution_interval_counter / (double) demand_miss_interval_counter << endl;
        
    }

    if (COST_MODEL || COST_MODEL_INTERVAL) {
        cout << "net_cost_total: " << net_cost_total << endl;
        cout << "net_cost_interval: " << net_cost_interval << endl;
        int64_t cost = net_cost_total;
        if (COST_MODEL_INTERVAL) {
            cost = (0.5 * net_cost_total + 0.5 * net_cost_interval);
        }
        if (cost > 10) {
            return INCREMENT;
        }
        if (cost < 0) {
            return DECREMENT;
        }
        else {
            return NO_CHANGE;
        }
    }

    if (ALTERNATIVE_FDP) {
        if ((accuracy >= accuracy_high_threshold && lateness >= lateness_threshold) || (accuracy >= accuracy_high_threshold && pollution < pollution_high_threshold) || (accuracy >= accuracy_high_threshold && dram_availability > dram_availability_threshold)) {
            return INCREMENT;
        }
        else {
            return DECREMENT;
        }
    }
    
    if (accuracy >= accuracy_high_threshold && lateness >= lateness_threshold && pollution < pollution_high_threshold) {
        // to increase timeliness
        return INCREMENT;
    }
    else if (accuracy >= accuracy_high_threshold && lateness >= lateness_threshold && pollution >= pollution_high_threshold) {
        // to increase timeliness
        return NO_CHANGE;
    }
    else if (accuracy >= accuracy_high_threshold && lateness < lateness_threshold && pollution < pollution_high_threshold) {
        // no change, best configuration
        return INCREMENT;
    }
    else if (accuracy >= accuracy_high_threshold && lateness < lateness_threshold && pollution >= pollution_high_threshold) {
        // to decrease pollution
        return DECREMENT;
    }
    else if (accuracy >= accuracy_low_threshold && accuracy < accuracy_high_threshold && lateness >= lateness_threshold && pollution < pollution_high_threshold) {
        // to increase timeliness
        return INCREMENT;
    }
    else if (accuracy >= accuracy_low_threshold && accuracy < accuracy_high_threshold && lateness >= lateness_threshold && pollution >= pollution_high_threshold) {
        // to reduce pollution
        if (DRAM_AVAILABILITY) {
            if (dram_availability >= dram_availability_threshold) {
                return INCREMENT;
            }
        }
        return DECREMENT;
    }
    else if (accuracy >= accuracy_low_threshold && accuracy < accuracy_high_threshold && lateness < lateness_threshold && pollution < pollution_high_threshold) {
        // keep benefits of timely prefetches
        return INCREMENT;
    }
    else if (accuracy >= accuracy_low_threshold && accuracy < accuracy_high_threshold && lateness < lateness_threshold && pollution >= pollution_high_threshold) {
        // to reduce pollution
        if (DRAM_AVAILABILITY) {
            if (dram_availability >= dram_availability_threshold) {
                return INCREMENT;
            }
        }
        return DECREMENT;
    }
    else if (accuracy < accuracy_low_threshold && lateness >= lateness_threshold && pollution < pollution_high_threshold) {
        // to save bandwidth
        // if (DRAM_AVAILABILITY) {
        //     if (dram_availability >= dram_availability_threshold) {
        //         return INCREMENT;
        //     }
        // }
        return DECREMENT;
    }
    else if (accuracy < accuracy_low_threshold && lateness >= lateness_threshold && pollution >= pollution_high_threshold) {
        // to reduce pollution
        // if (DRAM_AVAILABILITY) {
        //     if (dram_availability >= dram_availability_threshold) {
        //         return INCREMENT;
        //     }
        // }
        return DECREMENT;
    }
    else if (accuracy < accuracy_low_threshold && lateness < lateness_threshold && pollution < pollution_high_threshold) {
        // keep benefits of timely prefetches
        // if (DRAM_AVAILABILITY) {
        //     if (dram_availability >= dram_availability_threshold) {
        //         return INCREMENT;
        //     }
        // }
        return NO_CHANGE;
    }
    else if (accuracy < accuracy_low_threshold && lateness < lateness_threshold && pollution >= pollution_high_threshold) {
        // reduce pollution and save bandwidth
        // if (DRAM_AVAILABILITY) {
        //     if (dram_availability >= dram_availability_threshold) {
        //         return INCREMENT;
        //     }
        // }
        return DECREMENT;
    }
    return NO_CHANGE;

}

bool FDP_Control::is_interval_complete(void) {
    return (num_useful_blocks_evicted >= num_useful_blocks_evicted_threshold);
}

void FDP_Control::reset_interval(uint64_t current_cycle) {
    num_useful_blocks_evicted = 0;

    interval_start = current_cycle;


    if (FDP_INTERVAL) {
        useful_prefetch_total += useful_prefetch_interval_counter;
        late_prefetch_total += late_prefetch_interval_counter;
        prefetch_fill_total += prefetch_fill_interval_counter;
        demand_miss_total += demand_miss_interval_counter;
        pollution_total += pollution_interval_counter;
        dram_availability_at_prefetch_issue += dram_availability_at_prefetch_issue_interval;
        net_cost_total += net_cost_interval;
        // if (current_demand_miss_latency_avg == 0.0) {
        //     current_demand_miss_latency_avg = (double) demand_miss_latency_interval_counter / (double) demand_miss_interval_counter;
        // }
        // else {
        //     current_demand_miss_latency_avg = 0.5 * ((double) demand_miss_latency_interval_counter / (double) demand_miss_interval_counter) + 0.5*current_demand_miss_latency_avg;
        // }
        current_demand_miss_latency_avg = (double) demand_miss_latency_interval_counter / (double) demand_miss_interval_counter;

        useful_prefetch_interval_counter = 0;
        late_prefetch_interval_counter = 0;
        prefetch_fill_interval_counter = 0;
        demand_miss_interval_counter = 0;
        pollution_interval_counter = 0;
        memory_request_interval_counter = 0;
        dram_availability_at_prefetch_issue_interval = 0;
        demand_miss_latency_interval_counter = 0;
        prefetch_miss_interval_counter = 0;
        prefetch_miss_latency_interval_counter = 0;

    }

    if (COST_MODEL_INTERVAL) {
        net_cost_total += net_cost_interval;
        net_cost_interval = 0;
    }


}

void FDP_Control::print_stats(void) {
    cout << "useful prefetch total: " << useful_prefetch_total << endl;
    cout << "late prefetch total: " << late_prefetch_total << endl;
    cout << "prefetch fill total: " << prefetch_fill_total << endl;
    cout << "pollution total: " << pollution_total << endl;
    cout << "demand miss total: " << demand_miss_total << endl;

    cout << "fdp accuracy: " << (double) useful_prefetch_total / (double) prefetch_fill_total << endl;
    cout << "fdp lateness: " << (double) late_prefetch_total / (double) useful_prefetch_total << endl;
    cout << "fdp pollution: " << (double) pollution_total / (double) demand_miss_total << endl;

    for (int i = 0; i < fdp_interval_vec.size(); i++) {
        cout << "control: " << fdp_interval_vec[i].r << " degree: " << fdp_interval_vec[i].degree << endl;
    }
}

uint64_t FDP_Control::get_interval_start_cycle(void) {
    return interval_start;
}

void FDP_Control::demand_hit_prefetched_block(double cycles) {
    if (COST_MODEL) {
        net_cost_total += cycles;
    }
    else if (COST_MODEL_INTERVAL) {
        net_cost_interval += cycles;
    }
}

void FDP_Control::late_prefetch(double cycles) {
    if (COST_MODEL) {
        net_cost_total += cycles;
    }
    else if (COST_MODEL_INTERVAL) {
        net_cost_interval += cycles;
    }
}
void FDP_Control::cache_pollution(double cycles) {
    if (COST_MODEL) {
        net_cost_total -= cycles;
    }
    else if (COST_MODEL_INTERVAL) {
        net_cost_interval -= cycles;
    }
}

void FDP_Control::useless_prefetch(double cycles) {
    if (COST_MODEL) {
        net_cost_total -= cycles;
    }
    else if (COST_MODEL_INTERVAL) {
        net_cost_interval -= cycles;
    }
}
