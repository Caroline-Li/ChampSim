//Hawkeye Cache Replacement Tool v2.0
//UT AUSTIN RESEARCH LICENSE (SOURCE CODE)
//The University of Texas at Austin has developed certain software and documentation that it desires to
//make available without charge to anyone for academic, research, experimental or personal use.
//This license is designed to guarantee freedom to use the software for these purposes. If you wish to
//distribute or make other use of the software, you may purchase a license to do so from the University of
//Texas.
///////////////////////////////////////////////
//                                            //
//     Hawkeye [Jain and Lin, ISCA' 16]       //
//     Akanksha Jain, akanksha@cs.utexas.edu  //
//                                            //
///////////////////////////////////////////////

// Source code for configs 1 and 2

#include "cache.h"
#include <map>
#include <cassert>
#include "tracer.h"

#define NUM_CORE 1
#define LLC_SETS NUM_CORE*2048
#define LLC_WAYS 16

//3-bit RRIP counters or all lines
#define maxRRPV 7
// uint32_t rrpv[LLC_SETS][LLC_WAYS];

Tracer tracer;
uint64_t perset_mytimer[LLC_SETS];

// Signatures for sampled sets; we only use 64 of these
// Budget = 64 sets * 16 ways * 12-bit signature per line = 1.5B
// uint64_t signatures[LLC_SETS][LLC_WAYS];

// Hawkeye Predictors for demand and prefetch requests
// Predictor with 2K entries and 5-bit counter per entry
// Budget = 2048*5/8 bytes = 1.2KB
#define MAX_SHCT 31
#define SHCT_SIZE_BITS 11
#define SHCT_SIZE (1<<SHCT_SIZE_BITS)
#include "hawkeye_predictor.h"
HAWKEYE_PC_PREDICTOR* demand_predictor;  //Predictor

#include "optgen_simple.h"
OPTgen perset_optgen[LLC_SETS]; // per-set occupancy vectors; we only use 64 of these

#include <math.h>
#define bitmask(l) (((l) == 64) ? (unsigned long long)(-1LL) : ((1LL << (l))-1LL))
#define bits(x, i, l) (((x) >> (i)) & bitmask(l))

#define SAMPLING
//Sample 64 sets per core
#ifdef SAMPLING
    #define SAMPLED_SET(set) (bits(set, 0 , 6) == bits(set, ((unsigned long long)log2(LLC_SETS) - 6), 6) )
#else
    #define SAMPLED_SET(set) (true)
#endif

// Sampler to track 8x cache history for sampled sets
// 2800 entris * 4 bytes per entry = 11.2KB
#define SAMPLED_CACHE_SIZE 2800
map<uint64_t, ADDR_INFO> addr_history; // Sampler


// initialize replacement state
void CACHE::initialize_replacement()
{
    for (auto& blk : block)
        // signature and pefetch already set to default values
        blk.lru = maxRRPV;
    for (int i=0; i<LLC_SETS; i++) {
        perset_mytimer[i] = 0;
        perset_optgen[i].init(LLC_WAYS-2);
    }

    addr_history.clear();

    demand_predictor = new HAWKEYE_PC_PREDICTOR();

    cout << "Initialize Hawkeye state" << endl;
}

// find replacement victim
// return value should be 0 ~ 15 or 16 (bypass)
uint32_t CACHE::find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
    // look for the maxRRPV line
    uint32_t start = set*NUM_WAY;
    uint32_t end = start + NUM_WAY;
    for (uint32_t i=start; i<end; i++)
        if (block[i].lru == maxRRPV) {
            tracer.update_eviction(CACHE_LINE(block[i].address), this->current_cycle);
            return i - start;
        }

    //If we cannot find a cache-averse line, we evict the oldest cache-friendly line
    uint32_t max_rrip = 0;
    int32_t lru_victim = -1;
    int32_t lru_victim_index = -1;
    for (uint32_t i=start; i<end; i++)
    {
        if (block[i].lru >= max_rrip)
        {
            max_rrip = block[i].lru;
            lru_victim = i - start;
            lru_victim_index = i;
        }
    }

    assert (lru_victim != -1);
    //The predictor is trained negatively on LRU evictions
    if( SAMPLED_SET(set) )
        demand_predictor->decrement(block[lru_victim_index].signature);
    tracer.update_eviction(CACHE_LINE(block[lru_victim_index].address), this->current_cycle);
    return lru_victim;

    // WE SHOULD NOT REACH HERE
    assert(0);
    return 0;
}

void replace_addr_history_element()
{
    uint64_t lru_addr = 0;
    uint64_t lru_time = 10000000;
    
    for(map<uint64_t, ADDR_INFO>::iterator it=addr_history.begin(); it != addr_history.end(); it++)
    {
        if((it->second).last_quanta < lru_time)
        {
            lru_time =  (it->second).last_quanta;
            lru_addr = it->first;
        }
    }
    assert(lru_addr != 0);
    addr_history.erase(lru_addr);
}

// called on every cache hit and cache fill
void CACHE::update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    uint64_t paddr = (full_addr >> 6) << 6;

    if (type == WRITEBACK)
        return;


    //If we are sampling, OPTgen will only see accesses from sampled sets
    if(SAMPLED_SET(set))
    {
        //The current timestep 
        uint64_t curr_quanta = perset_mytimer[set];

        // This line has been used before. Since the right end of a usage interval is always 
        //a demand, ignore prefetches
        if(addr_history.find(paddr) != addr_history.end())
        {
            uint64_t last_quanta = addr_history[paddr].last_quanta;
            assert(curr_quanta >= addr_history[paddr].last_quanta);

            //Train the predictor positively because OPT would have cached this line
            if( perset_optgen[set].should_cache(curr_quanta, last_quanta))
            {
                demand_predictor->increment(addr_history[paddr].PC);
            }
            else
            {
                //Train the predictor negatively because OPT would not have cached this line
                demand_predictor->decrement(addr_history[paddr].PC);
            }
            //Some maintenance operations for OPTgen
            perset_optgen[set].add_access(curr_quanta);
        }
        // This is the first time we are seeing this line
        else
        {
#ifdef SAMPLING
            // Find a victim from the sampled cache if we are sampling
            assert(addr_history.size() <= SAMPLED_CACHE_SIZE);
            if(addr_history.size() == SAMPLED_CACHE_SIZE) 
                replace_addr_history_element();
            assert(addr_history.size() < SAMPLED_CACHE_SIZE);
#endif
            //Initialize a new entry in the sampler
            addr_history[paddr].init(curr_quanta);
            perset_optgen[set].add_access(curr_quanta);
        }

        // Get Hawkeye's prediction for this line
        bool new_prediction = demand_predictor->get_prediction (ip);

        // Update the sampler with the timestamp, PC and our prediction
        // For prefetches, the PC will represent the trigger PC
        addr_history[paddr].update(perset_mytimer[set], ip, new_prediction);
        addr_history[paddr].lru = 0;

        //Increment the set timer
        perset_mytimer[set] = (perset_mytimer[set]+1);
    }

    bool new_prediction = demand_predictor->get_prediction (ip);

    tracer.add_new_access(CACHE_LINE(full_addr), new_prediction, hit, type, this->current_cycle, ip);

    block[set*NUM_WAY + way].signature = ip;

    //Set RRIP values and age cache-friendly line
    if(!new_prediction)
        block[set*NUM_WAY + way].lru = maxRRPV;
    else
    {
        block[set*NUM_WAY + way].lru = 0;
        if(!hit)
        {
            bool saturated = false;
            for(uint32_t i=0; i<NUM_WAY; i++)
                if (block[set*NUM_WAY + i].lru == maxRRPV-1)
                    saturated = true;

            //Age all the cache-friendly  lines
            for(uint32_t i=0; i<NUM_WAY; i++)
            {
                if (!saturated && block[set*NUM_WAY + i].lru < maxRRPV-1)
                    block[set*NUM_WAY + i].lru++;
            }
        }
        block[set*NUM_WAY + way].lru = 0;
    }
}

// use this function to print out your own stats at the end of simulation
void CACHE::replacement_final_stats()
{
    unsigned int hits = 0;
    unsigned int accesses = 0;
    unsigned int traffic = 0;
    for(unsigned int i=0; i<LLC_SETS; i++)
    {
        accesses += perset_optgen[i].access;
        hits += perset_optgen[i].get_num_opt_hits();
        traffic += perset_optgen[i].get_traffic();
    }

    std::cout << "OPTgen accesses: " << accesses << std::endl;
    std::cout << "OPTgen hits: " << hits << std::endl;
    std::cout << "OPTgen hit rate: " << 100*(double)hits/(double)accesses << std::endl;
    std::cout << "Traffic: " << traffic << " " << 100*(double)traffic/(double)accesses << std::endl;

    tracer.generate_prefetch_interval();

    cout << endl << endl;
    return;
}
