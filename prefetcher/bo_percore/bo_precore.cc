#include "bo_percore.h" 
#include "prefetch_ip_suppress_filter.h"

#define DEGREE 8
#define WIDTH_DEGREE 1

extern Prefetch_Filter pf_filter;
uint64_t queued_prefetches = 0;
uint64_t total_prefetches = 0;
extern vector<uint64_t> history[NUM_CPUS];

void CACHE::prefetcher_initialize() {
    cout << "CPU " << cpu << " L2C BO prefetcher" << endl;
    // cout << "BO starting degree: " << START_DEGREE << endl;
    cout << "BO starting degree: " << DEGREE << endl;
	bo_l2c_prefetcher_initialize();
    bo[cpu].offset_index = 0;
}
void CACHE::prefetcher_cycle_operate() {
}

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in, uint64_t instr_id, uint64_t curr_cycle) {

    uint64_t bo_trigger_addr = 0;
    uint64_t bo_target_offset = 0;
    uint64_t bo_target_addr = 0;
    uint64_t bo_score = 0;
    bo_l2c_prefetcher_operate(addr, ip, cache_hit, type, this, &bo_trigger_addr, &bo_target_offset, &bo_score, 0);

    // if (bo_trigger_addr && bo_target_offset) {

    //     for(unsigned int i=1; i<=DEGREE; i++) {
    //         bo_target_addr = bo_trigger_addr + (i*bo_target_offset);
    //         bo_issue_prefetcher(this, ip, bo_trigger_addr, bo_target_addr, FILL_LLC, i, bo_target_offset, bo_score);
    //     }
    // }

    // if (bo_trigger_addr && bo_target_offset) {

    //     for(unsigned int i=1; i<=DEGREE; i++) {
    //         bo_target_addr = bo_trigger_addr + (i*bo_target_offset);
    //         bo_issue_prefetcher(this, ip, bo_trigger_addr, bo_target_addr, FILL_LLC, i, bo_target_offset, bo_score);
    //     }
    // }
    // if (bo_trigger_addr) {
    //     for (int i = 1; i <= WIDTH_DEGREE; i++) {
    //         for (int j = 0; j < NOFFSETS; j++) {
    //             if (bo[cpu].os.score[j] > BAD_SCORE) {
    //                 bo_target_addr = bo_trigger_addr + (i*OFFSET[j]);
    //                 bo_issue_prefetcher(this, ip, bo_trigger_addr, bo_target_addr, FILL_LLC, i, OFFSET[j], bo_score);
    //             }  
    //         }
    //     }
    // }
    // int count = 0;
    // if (bo_trigger_addr) {
    //     for (int i = 1; i <= WIDTH_DEGREE; i++) {
    //         for (int j = 0; j < NOFFSETS; j++) {
    //             if (warmup_complete[cpu] && NAME == L2_NAME && PERCEPTRON_IP_FILTER && PERCEPTRON_HISTORY) {
    //                 bo_target_addr = bo_trigger_addr + (i*OFFSET[j]);
    //                 int pred = pf_filter.get_int_prediction(ip, CACHE_LINE(bo_target_addr), i, bo_trigger_addr, OFFSET[j], history[cpu]);
    //                 if (pred >= -80) {
    //                     int result = bo_issue_prefetcher(this, ip, bo_trigger_addr, bo_target_addr, FILL_LLC, i, OFFSET[j], bo_score);
                        
    //                 }
    //                 // else {
    //                 //     pf_filter.add_to_reject_table(ip, CACHE_LINE(bo_target_addr), i, bo_trigger_addr, OFFSET[j], false, history[cpu]);
    //                 // }
    //             }
    //             else {
    //                 bo_target_addr = bo_trigger_addr + (i*OFFSET[j]);
    //                 bo_issue_prefetcher(this, ip, bo_trigger_addr, bo_target_addr, FILL_LLC, i, OFFSET[j], bo_score);
    //             }
    //         }  
    //     }
    // }
    // if (count > 0 && count < 16) {
    //     pf_filter.lo_threshold = -5;
    // }
    // else {
    //     pf_filter.lo_threshold = 1;
    // }

    if (bo_trigger_addr) {
        for (int i = 1; i <= WIDTH_DEGREE; i++) {
            for (int j = 0; j < NOFFSETS; j++) {
                bo_target_addr = bo_trigger_addr + (i*OFFSET[j]);
                bo_issue_prefetcher(this, ip, bo_trigger_addr, bo_target_addr, FILL_LLC, i, OFFSET[j], bo_score);
            }  
        }
    }
    // int PQ_available = get_size(3, 0) - get_occupancy(3, 0);
    // if (bo_trigger_addr && PQ_available >= 8) {
    //     for (int i = 1; i <= WIDTH_DEGREE; i++) {
    //         for (int j = 0; j < NOFFSETS; j++) {
    //             if (warmup_complete[cpu] && NAME == L2_NAME && PERCEPTRON_IP_FILTER && PERCEPTRON_HISTORY) {
    //                 bo_target_addr = bo_trigger_addr + (i*OFFSET[j]);
    //                 int pred = pf_filter.get_int_prediction(ip, CACHE_LINE(bo_target_addr), i, bo_trigger_addr, OFFSET[j], history[cpu]);
    //                 if (pred >= -5 && pred <= 0) {
    //                     bo_issue_prefetcher(this, ip, bo_trigger_addr, bo_target_addr, FILL_LLC, i, OFFSET[j], bo_score);
    //                 }
    //             }
    //             else if (!warmup_complete[cpu]) {
    //                 bo_target_addr = bo_trigger_addr + (i*OFFSET[j]);
    //                 bo_issue_prefetcher(this, ip, bo_trigger_addr, bo_target_addr, FILL_LLC, i, OFFSET[j], bo_score);
    //             }
    //         }  
    //     }
    // }
    // else {
    //     for (int i = 1; i <= WIDTH_DEGREE; i++) {
    //         for (int j = 0; j < NOFFSETS; j++) {
    //             if (warmup_complete[cpu] && NAME == L2_NAME && PERCEPTRON_IP_FILTER && PERCEPTRON_HISTORY) {
    //                 bo_target_addr = bo_trigger_addr + (i*OFFSET[j]);
    //                 int pred = pf_filter.get_int_prediction(ip, CACHE_LINE(bo_target_addr), i, bo_trigger_addr, OFFSET[j], history[cpu]);
    //                 if (pred >= -5 && pred <= 0) {
    //                     pf_filter.add_to_reject_table(ip, CACHE_LINE(bo_target_addr), i, bo_trigger_addr, OFFSET[j], false, history[cpu]);
    //                 }
    //             }
    //         }  
    //     }
    // }
    // int PQ_available = get_size(3, 0) - get_occupancy(3, 0);
    // int MSHR_available = get_size(0, 0) - get_occupancy(0, 0);
    // if (PQ_available > 0) {
    //     if (bo_trigger_addr) {
    //     for (int i = 1; i <= WIDTH_DEGREE; i++) {
    //         for (int j = 0; j < NOFFSETS; j++) {
    //             if (NAME == L2_NAME && PERCEPTRON_IP_FILTER && PERCEPTRON_HISTORY) {
    //                 bo_target_addr = bo_trigger_addr + (i*OFFSET[j]);
    //                 int pred = pf_filter.get_prediction(ip, CACHE_LINE(bo_target_addr), i, 0, get_set(bo_target_addr), bo_trigger_addr, OFFSET[j], lower_level->dram_occupancy, false, 0, false, false, warmup_complete[cpu], history[cpu]);
    //                 if (pred > 0) {
    //                     bo_issue_prefetcher(this, ip, bo_trigger_addr, bo_target_addr, FILL_LLC, i, OFFSET[j], bo_score);
    //                 }
    //             }
    //         }  
    //     }
    //     }
    // }
    // if (PQ_available )
    // bool stop = false;
    // if (bo_trigger_addr) {
    //     for (int i = 1; i <= WIDTH_DEGREE; i++) {
    //         for (int j = bo[cpu].offset_index; j < NOFFSETS; j++) {
    //             bo_target_addr = bo_trigger_addr + (i*OFFSET[j]);
    //             bo_issue_prefetcher(this, ip, bo_trigger_addr, bo_target_addr, FILL_LLC, i, OFFSET[j], bo_score);
    //             int PQ_available = get_size(3, 0) - get_occupancy(3, 0);
    //             if (PQ_available == 0) {
    //                 bo[cpu].offset_index = j;
    //                 stop = true;
    //                 break;
    //             }
    //         }
    //     }
    // }

    // if (!stop && bo_trigger_addr) {
    //     for (int i = 1; i <= WIDTH_DEGREE; i++) {
    //         for (int j = 0; j < bo[cpu].offset_index; j++) {
    //             bo_target_addr = bo_trigger_addr + (i*OFFSET[j]);
    //             bo_issue_prefetcher(this, ip, bo_trigger_addr, bo_target_addr, FILL_LLC, i, OFFSET[j], bo_score);
    //             int PQ_available = get_size(3, 0) - get_occupancy(3, 0);
    //             if (PQ_available == 0) {
    //                 bo[cpu].offset_index = j;
    //                 break;
    //             }
    //         }
    //     }
    // }
    // int PQ_available = get_size(3, 0) - get_occupancy(3, 0);
    // // cout << "PQ_available: " << PQ_available << endl;
    // if (bo_trigger_addr) {
    //     for (int i = 1; i <= WIDTH_DEGREE; i++) {
    //         for (int j = 0; j < NOFFSETS; j++) {
    //             if (bo[cpu].os.score[j] < LOW_SCORE) {
    //                 bo_target_addr = bo_trigger_addr + (i*OFFSET[j]);
    //                 bo_issue_prefetcher(this, ip, bo_trigger_addr, bo_target_addr, FILL_LLC, 2, OFFSET[j], bo_score);
    //             }
                
    //         }
    //     }
    // }

    return 0;
}

uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in, uint64_t curr_cycle) {
	bo_l2c_prefetcher_cache_fill(addr, set, way, prefetch, evicted_addr, this, 0);

    return 0;
}

void CACHE::prefetcher_final_stats() {
	bo_l2c_prefetcher_final_stats();
}

void CACHE::set_prefetcher_degree(int d, int cpu) {
    bo[cpu].degree = d;
}

int CACHE::get_prefetcher_degree(int cpu) {
    return bo[cpu].degree;
}

//void CACHE::complete_metadata_req(uint64_t meta_data_addr) {}
