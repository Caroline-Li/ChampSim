#include "cache.h"
#include "sisb.h"

void CACHE::prefetcher_initialize() {
    sisb_l2c_prefetcher_initialize();
}

void CACHE::prefetcher_cycle_operate() {
}


uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in) {
    sisb_l2c_prefetcher_operate(addr, ip, cache_hit, type, this);
    return 0;
}

uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in) {
    sisb_l2c_prefetcher_cache_fill(addr, set, way, prefetch, evicted_addr);
    return 0;
}

void CACHE::prefetcher_final_stats() {
    sisb_l2c_prefetcher_final_stats();
}

//void CACHE::complete_metadata_req(uint64_t meta_data_addr) {}
