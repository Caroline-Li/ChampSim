//Hawkeye Cache Replacement Tool v2.0 //UT AUSTIN RESEARCH LICENSE (SOURCE CODE)
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
#include "tracer.h"
#include "prefetch_reduction_predictor.h"
#include <cassert>
#include <utility> 
//#define NUM_CORE 1
#define LLC_SETS 2048
// #define LLC_WAYS NUM_WAY
//#define LLC_WAYS 16

//3-bit RRIP counters or all lines
#define maxRRPV 7
// uint32_t rrpv[LLC_SETS][LLC_WAYS];


//Per-set timers; we only use 64 of these
//Budget = 64 sets * 1 timer per set * 10 bits per timer = 80 bytes
#define TIMER_SIZE 1024
uint64_t perset_mytimer[LLC_SETS];

// Signatures for sampled sets; we only use 64 of these
// Budget = 64 sets * 16 ways * 12-bit signature per line = 1.5B
// uint64_t signatures[LLC_SETS][LLC_WAYS];
// bool prefetched[LLC_SETS][LLC_WAYS];

// Hawkeye Predictors for demand and prefetch requests
// Predictor with 2K entries and 5-bit counter per entry
// Budget = 2048*5/8 bytes = 1.2KB
#define MAX_SHCT 31
#define SHCT_SIZE_BITS 14
//#define SHCT_SIZE_BITS 11
#define SHCT_SIZE (1<<SHCT_SIZE_BITS)
#include "hawkeye_predictor.h"
HAWKEYE_PC_PREDICTOR* demand_predictor;  //Predictor
HAWKEYE_PC_PREDICTOR* prefetch_predictor;  //Predictor

#define OPTGEN_VECTOR_SIZE 128
#include "optgen.h"
OPTgen perset_optgen[LLC_SETS]; // per-set occupancy vectors; we only use 64 of these

#include <math.h>
#define bitmask(l) (((l) == 64) ? (unsigned long long)(-1LL) : ((1LL << (l))-1LL))
#define bits(x, i, l) (((x) >> (i)) & bitmask(l))
//Sample 64 sets per core
//#define SAMPLED_SET(set) (bits(set, 0 , 6) == bits(set, ((unsigned long long)log2(LLC_SETS) - 6), 6) )
#define SAMPLED_SET(set) (bits(set, 0 , 8) == bits(set, ((unsigned long long)log2(LLC_SETS) - 8), 8) )
//#define SAMPLED_SET(set) true

// Sampler to track 8x cache history for sampled sets
// 2800 entris * 4 bytes per entry = 11.2KB
#define SAMPLED_CACHE_SIZE 2800*NUM_CPUS
#define SAMPLER_WAYS 8
#define SAMPLER_SETS SAMPLED_CACHE_SIZE/SAMPLER_WAYS

#define CACHE_LINE(a) (a >> LOG2_BLOCK_SIZE)
vector<map<uint64_t, ADDR_INFO> > addr_history; // Sampler

Tracer tracer;
Prefetch_Reduction_Predictor k_predictor;

vector<uint64_t> dd_interval_distribution;
vector<uint64_t> dp_interval_distribution;
vector<uint64_t> pd_interval_distribution;
vector<uint64_t> pp_interval_distribution;
uint64_t dd_intervals, dp_intervals, pd_intervals, pp_intervals;
uint64_t dd_accuracy, dp_accuracy, pd_accuracy, pp_accuracy;
uint64_t dd_cached, dp_cached, pd_cached, pp_cached;

uint64_t threshold[NUM_CPUS];
#define EPOCH_LENGTH 100000000
#define TRAINING_LENGTH 50000000

double accuracy_threshold = 0.0;
bool training_complete = false;
struct EPOCH
{
    uint64_t last_epoch_cycle[NUM_CPUS];
    uint64_t supply[NUM_CPUS];
    uint64_t supply_count[NUM_CPUS];
    uint64_t demand[NUM_CPUS];
    uint64_t demand_count[NUM_CPUS];
    uint64_t supply_total, demand_total;
    
    void reset()
    {
        for(unsigned int i=0; i<NUM_CPUS; i++)
        {
            last_epoch_cycle[i] = current_core_cycle[i];
            supply[i] = 0;
            demand[i] = 0;
            supply_count[i] = 0;
            demand_count[i] = 0;
        }
        supply_total = 0;
        demand_total = 0;
    }

    bool is_complete()
    {
        bool complete = true;
        
        for(unsigned int i=0; i<NUM_CPUS; i++)
        {
            uint64_t cycles_elapsed = (current_core_cycle[i] - last_epoch_cycle[i]);
            if(cycles_elapsed < EPOCH_LENGTH)
            {
                complete = false;
                break;
            }
        }

        return complete;
    }

    bool training_complete()
    {
        bool complete = true;
        
        for(unsigned int i=0; i<NUM_CPUS; i++)
        {
            uint64_t cycles_elapsed = (current_core_cycle[i] - last_epoch_cycle[i]);
            if(cycles_elapsed < TRAINING_LENGTH)
            {
                complete = false;
                break;
            }
        }

        return complete;
    }

    void update_supply(int rd, uint32_t cpu)
    {
        assert(rd >= 0);
        supply_total += rd;
        supply[cpu] += rd;
        supply_count[cpu]++;
    }

    void update_demand(int rd, uint32_t cpu)
    {
        assert(rd >= 0);
        demand_total += rd;
        demand[cpu] += rd;
        demand_count[cpu]++;
    }

    void new_epoch()
    {
        uint64_t supply_rd[NUM_CPUS] = {0};
        uint64_t demand_rd[NUM_CPUS] = {0};
        double ratio[NUM_CPUS] = {0.0};
        for(unsigned int i=0; i<NUM_CPUS; i++)
        {
            if(supply_count[i] != 0)
                supply_rd[i] = (double)supply[i]/(double)supply_count[i];
            if(demand_count[i] != 0)
                demand_rd[i] = (double)demand[i]/(double)demand_count[i];

            if(supply_rd[i] != 0)
                ratio[i] = (double)demand_rd[i]/(double)supply_rd[i];
        }

        cout << "Demand/Supply: " << demand_total << " " << supply_total << endl;
        cout << "RD: ";
        for(unsigned int i=0; i<NUM_CPUS; i++)
            cout << supply_rd[i] << " ";
        cout << endl;
        for(unsigned int i=0; i<NUM_CPUS; i++)
            cout << demand_rd[i] << " ";
        cout << endl;

        for(unsigned int i=0; i<NUM_CPUS; i++)
            cout << ratio[i] << " ";
        cout << endl;
/*        
        if(demand_total >= supply_total)
        {
            for(unsigned int i=0; i<NUM_CPUS; i++)
                threshold[i] = (uint64_t)(5*NUM_CPUS*(ratio[i]/2));
        }
        else
        {
            for(unsigned int i=0; i<NUM_CPUS; i++)
                threshold[i] = 100000;

            int leftover_demand = demand_total;

            while((leftover_demand > 0) && (leftover_demand < supply_total))
            {
                uint64_t max_supply_length = 0;
                uint64_t max_supply_index = 0;
                for(unsigned int k=0; k< NUM_CPUS; k++)
                {
                    if(supply_rd[k] > max_supply_length)
                    {
                        max_supply_length = supply_rd[k];
                        max_supply_index = k;
                    }
                }

                double max_supply = supply[max_supply_index];
                threshold[max_supply_index] = (uint64_t)(5*NUM_CPUS*(ratio[max_supply_index]/2));
                leftover_demand = leftover_demand - max_supply;

                supply_rd[max_supply_index] = 0;
            }
        }
*/

        for(unsigned int i=0; i<NUM_CPUS; i++)
            threshold[i] = (uint64_t)(5*NUM_CPUS*(ratio[i]/2));

        cout << "Final: " << threshold[0] << " " << threshold[1] << " " << threshold[2] << " " << threshold[3] << endl;
    }
};

EPOCH myepoch;


// initialize replacement state
void CACHE::initialize_replacement()
{
    // initialize rrpv 
    for (auto& blk : block)
        // signature and pefetch already set to default values
        blk.lru = maxRRPV;

    for (int i=0; i<LLC_SETS; i++) {
       // for (int j=0; j<LLC_WAYS; j++) {
       //     rrpv[i][j] = maxRRPV;
       //     signatures[i][j] = 0;
       //     prefetched[i][j] = false;
       // }
        perset_mytimer[i] = 0;
        perset_optgen[i].init(NUM_WAY-2);
    }

    addr_history.resize(SAMPLER_SETS);
    for (int i=0; i<SAMPLER_SETS; i++) 
        addr_history[i].clear();

    demand_predictor = new HAWKEYE_PC_PREDICTOR();
    prefetch_predictor = new HAWKEYE_PC_PREDICTOR();

    pp_intervals = 0;
    dp_intervals = 0;
    pd_intervals = 0;
    dd_intervals = 0;
    
    pp_accuracy = 0;
    dp_accuracy = 0;
    pd_accuracy = 0;
    dd_accuracy = 0;

    pp_cached = 0;
    dp_cached = 0;
    pd_cached = 0;
    dd_cached = 0;
    cout << "Initialize Hawkeye state" << endl;

    myepoch.reset();
    myepoch.new_epoch();
    for(unsigned int i=0; i<NUM_CPUS; i++)
        threshold[i] = 5*NUM_CPUS;

}

// find replacement victim
// return value should be 0 ~ 15 or 16 (bypass)
uint32_t CACHE::find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
    // cout << "start find victim" << endl;
    // look for the maxRRPV line
    uint32_t start = set*NUM_WAY;
    uint32_t end = start + NUM_WAY;
    for (uint32_t i=start; i<end; i++)
        if (block[i].lru == maxRRPV) {
            tracer.update_eviction(CACHE_LINE(block[i].address), this->current_cycle, ip);
            // tracer.update_eviction(ip, this->current_cycle);
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
    {
        if(block[lru_victim_index].prefetch)
            prefetch_predictor->decrement(block[lru_victim_index].signature);
        else
            demand_predictor->decrement(block[lru_victim_index].signature);
    }
    tracer.update_eviction(CACHE_LINE(block[lru_victim_index].address), this->current_cycle, ip);
    // tracer.update_eviction(ip, this->current_cycle);
    return lru_victim;

    // WE SHOULD NOT REACH HERE
    assert(0);
    return 0;
}

void replace_addr_history_element(unsigned int sampler_set)
{
    uint64_t lru_addr = 0;
    
    for(map<uint64_t, ADDR_INFO>::iterator it=addr_history[sampler_set].begin(); it != addr_history[sampler_set].end(); it++)
    {
   //     uint64_t timer = (it->second).last_quanta;

        if((it->second).lru == (SAMPLER_WAYS-1))
        {
            //lru_time =  (it->second).last_quanta;
            lru_addr = it->first;
            break;
        }
    }

    addr_history[sampler_set].erase(lru_addr);
}

void update_addr_history_lru(unsigned int sampler_set, unsigned int curr_lru)
{
    for(map<uint64_t, ADDR_INFO>::iterator it=addr_history[sampler_set].begin(); it != addr_history[sampler_set].end(); it++)
    {
        if((it->second).lru < curr_lru)
        {
            (it->second).lru++;
            assert((it->second).lru < SAMPLER_WAYS); 
        }
    }
}

uint64_t average_latency = 0;
uint64_t average_latency_count = 0;

// called on every cache hit and cache fill
void CACHE::update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    // cout << "start update replacement state" << endl;
    uint64_t paddr = (full_addr >> 6) << 6;


    if(type == PREFETCH)
    {
        if (!hit)
            block[set*NUM_WAY + way].prefetch = true;

    }
    else {
        block[set*NUM_WAY + way].prefetch = false;

    }

    //Ignore writebacks
    if (type == WRITEBACK)
        return;

    if(!hit)
    {
        average_latency += 0;
        average_latency_count += 1;
    }
    // update prefetch k map and list
    
    // cout << "after prefetch processing" << endl;

    //cout << hex << paddr << setw(9) << dec << " " << latency << " " << effective_latency << endl;
    //If we are sampling, OPTgen will only see accesses from sampled sets
    if(SAMPLED_SET(set))
    {
        //The current timestep 
        uint64_t curr_quanta = perset_mytimer[set] % OPTGEN_VECTOR_SIZE;
        // cout << "curr quanta: " << endl;
        // cout << curr_quanta << endl;

        uint32_t sampler_set = (paddr >> 6) % SAMPLER_SETS; 
        uint64_t sampler_tag = CRC(paddr >> 12) % 256;
        assert(sampler_set < SAMPLER_SETS);

        unsigned int curr_timer = perset_mytimer[set];
        // cout << "curr timer 1" << endl;
        // cout << curr_timer << endl;
        if(curr_timer < addr_history[sampler_set][sampler_tag].last_quanta)
            curr_timer = curr_timer + TIMER_SIZE;
        // cout << "curr timer 2" << endl;
        // cout << curr_timer << endl;

        // This line has been used before. Since the right end of a usage interval is always 
        //a demand, ignore prefetches
        if((addr_history[sampler_set].find(sampler_tag) != addr_history[sampler_set].end()) && (type != PREFETCH))
        {
            // cout << " line has been used before" << endl;
            bool wrap =  ((curr_timer - addr_history[sampler_set][sampler_tag].last_quanta) > OPTGEN_VECTOR_SIZE);
            // cout << "wrap: " << endl;
            // cout << wrap << endl;
            uint64_t last_quanta = addr_history[sampler_set][sampler_tag].last_quanta % OPTGEN_VECTOR_SIZE;

            //cout << curr_timer << " " << addr_history[sampler_set][sampler_tag].last_quanta << endl;
            assert(curr_timer >= addr_history[sampler_set][sampler_tag].last_quanta);
            uint64_t interval = (curr_timer - addr_history[sampler_set][sampler_tag].last_quanta);
            if(addr_history[sampler_set][sampler_tag].prefetched)
            {
                // cout << "prefetched line" << endl;
                if(pd_interval_distribution.size() < (interval + 1))
                    pd_interval_distribution.resize((interval + 1), 0);
                pd_interval_distribution[interval]++;
                pd_intervals++;
                //cout << "PD: " << hex << paddr << " " << addr_history[sampler_set][sampler_tag].PC << dec;
            }
            else
            {
                // cout << "not prefetched" << endl;
                if(dd_interval_distribution.size() < (interval + 1))
                    dd_interval_distribution.resize((interval + 1), 0);
                dd_interval_distribution[interval]++;
                dd_intervals++;
            }

            //and for prefetch hits, we train the last prefetch trigger PC
            if( !wrap && perset_optgen[set].should_cache(curr_quanta, last_quanta, false, cpu))
            {
                if(addr_history[sampler_set][sampler_tag].prefetched) { 
                    // cout << "prefetch hit" << endl;
                    prefetch_predictor->increment(addr_history[sampler_set][sampler_tag].PC);

                    if(addr_history[sampler_set][sampler_tag].last_prediction) {
                        pd_accuracy++;
                    }
                    pd_cached++;
                }
                else {
                    // cout << "demand hit" << endl;
                    //

                    demand_predictor->increment(addr_history[sampler_set][sampler_tag].PC);
                    // cout  << " after demand predictor" << endl;
                    if(addr_history[sampler_set][sampler_tag].last_prediction)
                        dd_accuracy++;
                }
            }
            else
            {
                // cout << "not prefetch hit" << endl;
                //Train the predictor negatively because OPT would not have cached this line
                if(addr_history[sampler_set][sampler_tag].prefetched) {
                    prefetch_predictor->decrement(addr_history[sampler_set][sampler_tag].PC);

                    if(addr_history[sampler_set][sampler_tag].last_prediction == false)
                        pd_accuracy++;
                }
                else {


                    demand_predictor->decrement(addr_history[sampler_set][sampler_tag].PC);
                    myepoch.update_demand(interval, cpu);
                    if(addr_history[sampler_set][sampler_tag].last_prediction == false)
                        dd_accuracy++;
                }
            }
            // cout << "maintenance operations" << endl;
            //Some maintenance operations for OPTgen
            // cout << "optgen set: " << set << endl;
            perset_optgen[set].add_access(curr_quanta, cpu);
            // cout << "optgen updated" << endl;
            update_addr_history_lru(sampler_set, addr_history[sampler_set][sampler_tag].lru);
            // cout << "update history" << endl;

            //Since this was a demand access, mark the prefetched bit as false
            addr_history[sampler_set][sampler_tag].prefetched = false;
            // cout << "end maintenance operations" << endl;
        }
        // This is the first time we are seeing this line (could be demand or prefetch)
        else if(addr_history[sampler_set].find(sampler_tag) == addr_history[sampler_set].end())
        {
            // cout << "first time seeing line" << endl;
            // Find a victim from the sampled cache if we are sampling
            if(addr_history[sampler_set].size() == SAMPLER_WAYS) 
                replace_addr_history_element(sampler_set);

            assert(addr_history[sampler_set].size() < SAMPLER_WAYS);
            //Initialize a new entry in the sampler
            addr_history[sampler_set][sampler_tag].init(curr_quanta);
            //If it's a prefetch, mark the prefetched bit;
            if(type == PREFETCH)
            {

                addr_history[sampler_set][sampler_tag].mark_prefetch();
                perset_optgen[set].add_prefetch(curr_quanta);
            }
            else {
                perset_optgen[set].add_access(curr_quanta, cpu);

            }
            update_addr_history_lru(sampler_set, SAMPLER_WAYS-1);
        }
        else //This line is a prefetch
        {
            // cout << "line is a prefetch" << endl;
            assert(addr_history[sampler_set].find(sampler_tag) != addr_history[sampler_set].end());
            //if(hit && prefetched[set][way])
            uint64_t last_quanta = addr_history[sampler_set][sampler_tag].last_quanta % OPTGEN_VECTOR_SIZE;

            assert(curr_timer >= addr_history[sampler_set][sampler_tag].last_quanta);
            uint64_t interval = (curr_timer - addr_history[sampler_set][sampler_tag].last_quanta);

            if(addr_history[sampler_set][sampler_tag].prefetched)
            {
                pp_intervals++; 
                if(pp_interval_distribution.size() < (interval + 1))
                    pp_interval_distribution.resize((interval + 1), 0);
                pp_interval_distribution[interval]++;
            }
            else
            {
                dp_intervals++;
                if(dp_interval_distribution.size() < (interval + 1))
                    dp_interval_distribution.resize((interval + 1), 0);
                dp_interval_distribution[interval]++;
            }

            if(perset_optgen[set].should_cache_probe(curr_quanta, last_quanta))
                myepoch.update_supply(interval, cpu);
            if (perset_mytimer[set] - addr_history[sampler_set][sampler_tag].last_quanta < threshold[cpu]) // threshold[cpu]
            {
                // cout << " prefetch length under threshold" << endl;
                if(perset_optgen[set].should_cache(curr_quanta, last_quanta, true, cpu))
                {
                    if(addr_history[sampler_set][sampler_tag].prefetched) // P-P
                    {
                        prefetch_predictor->increment(addr_history[sampler_set][sampler_tag].PC);

                        if(addr_history[sampler_set][sampler_tag].last_prediction)
                            pp_accuracy++;
                    }
                    else //D-P
                    {
                        demand_predictor->increment(addr_history[sampler_set][sampler_tag].PC);
                        if(addr_history[sampler_set][sampler_tag].last_prediction)
                            dp_accuracy++;
                    }
                }
                else
                {
                    if(addr_history[sampler_set][sampler_tag].prefetched) // P-P
                    {
                        prefetch_predictor->decrement(addr_history[sampler_set][sampler_tag].PC);
                    }

            
                    if(addr_history[sampler_set][sampler_tag].last_prediction == false)
                    {
                        if(addr_history[sampler_set][sampler_tag].prefetched) // P-P
                            pp_accuracy++;
                        else
                            dp_accuracy++;
                    }
                }
            }
            else
            {
                // cout << "last prefetch case" << endl;
                if(addr_history[sampler_set][sampler_tag].prefetched) // P-P
                {
                    prefetch_predictor->decrement(addr_history[sampler_set][sampler_tag].PC);
                }
                else
                {
                    demand_predictor->decrement(addr_history[sampler_set][sampler_tag].PC);
                }
                if(addr_history[sampler_set][sampler_tag].last_prediction == false)
                {
                    if(addr_history[sampler_set][sampler_tag].prefetched) // P-P
                        pp_accuracy++;
                    else
                        dp_accuracy++;
                }
            }


            //Mark the prefetched bit
            addr_history[sampler_set][sampler_tag].mark_prefetch(); 
            //Some maintenance operations for OPTgen
            perset_optgen[set].add_prefetch(curr_quanta);
            update_addr_history_lru(sampler_set, addr_history[sampler_set][sampler_tag].lru);
        }
        // cout << "get prediction in training" << endl;

        // Get Hawkeye's prediction for this line
        bool new_prediction = demand_predictor->get_prediction (ip);
        if (type == PREFETCH)
            new_prediction = prefetch_predictor->get_prediction (ip);
        // Update the sampler with the timestamp, PC and our prediction
        // For prefetches, the PC will represent the trigger PC
        addr_history[sampler_set][sampler_tag].update(perset_mytimer[set], ip, new_prediction);
        addr_history[sampler_set][sampler_tag].lru = 0;
        //Increment the set timer
        perset_mytimer[set] = (perset_mytimer[set]+1) % TIMER_SIZE;
    }

    // cout << "after training" << endl;

    bool new_prediction = demand_predictor->get_prediction (ip);
    if (type == PREFETCH)
        new_prediction = prefetch_predictor->get_prediction (ip);
    
    tracer.add_new_access(CACHE_LINE(full_addr), new_prediction, hit, type, this->current_cycle, ip);
    // tracer.add_new_access(ip, new_prediction, hit, type, this->current_cycle, ip);

    // if (type == PREFETCH && tracer.get_current_k(CACHE_LINE(full_addr)) > 25) {
    //     pf_addr_suppression.insert(full_addr);
    // }
    // int accurate = tracer.get_accurate_counter_ip(ip);
    // int total = tracer.get_total_counter_ip(ip);

    // if (training_complete && (type == PREFETCH)) {
    //     double accuracy = (double) accurate/(double)total;

    //     if (accuracy == 0.0) {
    //         ip_suppression.insert(ip);
    //         cout << "ip marked: " << ip << " accuracy: " << accuracy << endl; 
    //     }
    //     // else if ((ip_suppression.find(ip) != ip_suppression.end()) && (accuracy > accuracy_threshold)) {
    //     //     ip_suppression.erase(ip);
    //     //     cout << "ip erased: " << ip << " accuracy: " << accuracy << endl;
    //     // }
    // }
    int accurate = tracer.get_accurate_counter_cacheline(CACHE_LINE(full_addr));
    int total = tracer.get_total_counter_cacheline(CACHE_LINE(full_addr));

    if (training_complete && type == PREFETCH && total > 0) {
        double accuracy = accurate/total;

        if (accuracy == accuracy_threshold) {
            pf_addr_suppression.insert(full_addr);
            cout << "addr marked: " << full_addr << "accuracy: " << accuracy << endl; 
        }
    }

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

    if (!training_complete && myepoch.training_complete()) {
        cout << "training complete!" << endl;
        training_complete = true;
        // cout << "ip accuracy map" << endl;
        // tracer.print_ip_accuracy_map();
        cout << "cacheline accuracy map" << endl;
        tracer.print_cacheline_accuracy_map();
    }


    if(myepoch.is_complete())
    {
        myepoch.new_epoch();
        myepoch.reset();
        training_complete = false;
        // cout << "ip suppressed" << endl;
        // for (auto it = ip_suppression.begin(); it != ip_suppression.end(); ++it) {
        //     cout << "ip: " << *it << endl;
        // }  
        cout << "pf addr suppressed" << endl;
        for (auto it = pf_addr_suppression.begin(); it != pf_addr_suppression.end(); ++it) {
            cout << "ip: " << *it << endl;
        }   
        pf_addr_suppression.clear();
        ip_suppression.clear();
        // cout << "ip accuracy map" << endl;
        // tracer.print_ip_accuracy_map();

        cout << "cacheline accuracy map" << endl;
        tracer.print_cacheline_accuracy_map();
        // tracer.clear_cacheline_accuracy_map();
        // tracer.clear_ip_accuracy_map();

    }
}

// use this function to print out your own stats at the end of simulation
void CACHE::replacement_final_stats()
{
    unsigned int hits = 0;
    unsigned int accesses = 0;
    unsigned int traffic = 0;
    unsigned int per_core_hits[NUM_CPUS] = {0};
    unsigned int per_core_accesses[NUM_CPUS] = {0};

    for(unsigned int i=0; i<LLC_SETS; i++)
    {
        accesses += perset_optgen[i].access;
        hits += perset_optgen[i].get_num_opt_hits();
        traffic += perset_optgen[i].get_traffic();

        for(unsigned int k=0; k<NUM_CPUS; k++) {
            per_core_accesses[k] += perset_optgen[i].per_core_access[k];
            per_core_hits[k] += perset_optgen[i].get_num_opt_hits(k);
        }
    }

    std::cout << "OPTgen accesses: " << accesses << std::endl;
    std::cout << "OPTgen hits: " << hits << std::endl;
    std::cout << "OPTgen hit rate: " << 100*(double)hits/(double)accesses << std::endl;
    std::cout << "Traffic: " << traffic << " " << 100*(double)traffic/(double)accesses << std::endl;
    for(unsigned int k=0; k<NUM_CPUS; k++) 
        std::cout << "OPTgen hit rate for core " << k <<": " << 100*(double)per_core_hits[k]/(double)per_core_accesses[k] << std::endl;

    cout << endl << endl;
    cout << "D-D intervals: " << dd_intervals << " " << 100*((double)dd_accuracy/(double)dd_intervals) << endl;
    for(unsigned int i=0; i<dd_interval_distribution.size(); i++)
        cout << dd_interval_distribution[i] << " ";
    cout << endl;

    cout << "D-P intervals: " << dp_intervals << " " << 100*((double)dp_accuracy/(double)dp_intervals) << endl;
    for(unsigned int i=0; i<dp_interval_distribution.size(); i++)
        cout << dp_interval_distribution[i] << " ";
    cout << endl;

    cout << "P-D intervals: " << pd_intervals << " " << 100*((double)pd_accuracy/(double)pd_intervals) << endl;
    cout << "P-D cached: " << 100*(double)pd_cached/(double)pd_intervals << endl;
    for(unsigned int i=0; i<pd_interval_distribution.size(); i++)
        cout << pd_interval_distribution[i] << " ";
    cout << endl;


    cout << "P-P intervals: " << pp_intervals << " " << 100*((double)pp_accuracy/(double)pp_intervals) << endl;
    for(unsigned int i=0; i<pp_interval_distribution.size(); i++)
        cout << pp_interval_distribution[i] << " ";
    cout << endl;

    cout << "Average latency: " << (double)average_latency/(double)average_latency_count << endl;


    // tracer.print_pc_map_stats();
    // tracer.generate_k_stats();
    // tracer.print_k_stats();
    // tracer.generate_prefetch_use_stats();
    // tracer.ideal_prefetch_traffic_reduction();
    tracer.generate_prefetch_interval();
    // tracer.get_predictor_accuracy();
    // tracer.generate_timeliness_stats();
    // tracer.print_pc_freq_map_prefetch();


    return;
}
