#include <stdio.h>
#include <stdlib.h>
#include "cache.h"

void CACHE::prefetcher_initialize() {

}

uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in) {
    return 0;
}

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in) {
    static uint64_t start_address = 0xabcdef1234;
    cache->prefetch_line(start_address, FILL_LLC, 0);
    start_address += 0x100;
    if (start_address & 0x0F00 == 0) {
        start_address = 0xabcdef1234;
    }
}

void CACHE::prefetcher_cycle_operate() {
}

void CACHE::prefetcher_final_stats() {

}

