//
// Data Prefetching Championship Simulator 2
//
#define DEGREE 2
#include <stdio.h>
#include "cache.h"
#include <map>
#include <set>
#include <cassert>
#include "training_unit.h"
#include "on_chip_info.h"

unsigned int total_access;
unsigned int predictions;
unsigned int prefetch_buffer_added;
unsigned int prefetch_buffer_issue;
unsigned int no_prediction;
unsigned int stream_end;
unsigned int no_translation;
unsigned int reuse;
uint64_t last_address;

#define BUFFER_SIZE 1024
//#define RESTRICT_REGION

struct PrefetchBuffer
{
    uint64_t buffer[BUFFER_SIZE];
    bool valid[BUFFER_SIZE];
    unsigned int next_index;

    void reset(){
        for(unsigned int i=0; i<BUFFER_SIZE; i++)
            valid[i] = false;
        next_index = 0;
    }
    void add(uint64_t addr){
        buffer[next_index] = addr;
        valid[next_index] = true;
        next_index = (next_index + 1)%BUFFER_SIZE;
    }

    void issue(unsigned int i){
        assert(valid[i]);
        valid[i] = false;
    }

    bool get(unsigned int i, uint64_t& addr){
        addr = buffer[i];
        return valid[i];
    }
};

struct ISB_prefetcher_t
{
    TUCache training_unit; 

    OnChipInfo oci;

    uint64_t alloc_counter;
    uint64_t last_page;

    uint64_t stream_divergence_count;
    uint64_t stream_divergence_new_stream;
    uint64_t candidate_tlb_miss;
    uint64_t candidate_diff;
    PrefetchBuffer prefetch_buffer;

    unsigned int isb_train(unsigned int str_addr_A, uint64_t phy_addr_B)
    {
        //Algorithm for training correlated pair (A,B) 
        //Step 2a : If SA(A)+1 does not exist, assign B SA(A)+1
        //Step 2b : If SA(A)+1 exists, copy the stream starting at S(A)+1 and then assign B SA(A)+1

        unsigned int str_addr_B;
        bool str_addr_B_exists = oci.get_structural_address(phy_addr_B, str_addr_B); 
#ifdef DEBUG
        std::cout << "-----S(A) : " << str_addr_A << std::endl;
#endif
        // If S(A) is at a stream boundary return, we don't need to worry about B because it is as good as a stream start
        if((str_addr_A+1) % STREAM_MAX_LENGTH == 0){
            if(!str_addr_B_exists){
                str_addr_B = assign_structural_addr();
                oci.update(phy_addr_B, str_addr_B);
            }
            return str_addr_B;
        }

        bool invalidated = false;
        if(str_addr_B_exists){
            //if(str_addr_B == str_addr_A + 1){
            if(str_addr_B%STREAM_MAX_LENGTH == (str_addr_A + 1)%STREAM_MAX_LENGTH){
#ifdef DEBUG
                std::cout << phy_addr_B << " has a structural address of " << str_addr_B << " conf++ " << std::endl;
#endif
                oci.increase_confidence(phy_addr_B);
                return str_addr_B;
            }
            else{
#ifdef DEBUG
                std::cout << phy_addr_B << " has a structural address of " << str_addr_B << " conf-- " << std::endl;
#endif
                bool ret = oci.lower_confidence(phy_addr_B);
                if(ret)
                    return str_addr_B;
#ifdef DEBUG
                std::cout << "Invalidate " << std::endl;
#endif
                invalidated = true;
                oci.invalidate(phy_addr_B, str_addr_B);
                str_addr_B_exists = false;
            }
        }

        assert(!str_addr_B_exists);

        //Handle stream divergence

        unsigned int i=1;
        uint64_t phy_addr_Aplus1;
        bool phy_addr_Aplus1_exists = oci.get_physical_address(phy_addr_Aplus1, str_addr_A+1);

        if(phy_addr_Aplus1_exists)
            stream_divergence_count++;
//#define CFIX
#ifdef CFIX
        while(phy_addr_Aplus1_exists){
#ifdef DEBUG
            std::cout << "-----S(A)+1 : " << phy_addr_Aplus1 << std::endl;
#endif
            i++;

            if((str_addr_A+i) % STREAM_MAX_LENGTH == 0)
            {
                stream_divergence_new_stream++;
                str_addr_B = assign_structural_addr();
                break;
            }
            phy_addr_Aplus1_exists = oci.get_physical_address(phy_addr_Aplus1, str_addr_A+i);

            // oci.reassign_stream(str_addr_A+1, assign_structural_addr()); //TODO TBD. Should we re-assign??
        }
        if(!phy_addr_Aplus1_exists)
            str_addr_B = str_addr_A + i;

#else
    if(phy_addr_Aplus1_exists)
    {
        //Old solution: Nothing fancy, just assign a new address
        stream_divergence_count++;
        if(invalidated)
            return str_addr_B;
        else
            str_addr_B = assign_structural_addr();
    }
    else
        str_addr_B = str_addr_A + 1;

#endif

#ifdef DEBUG
        std::cout << (void*)phy_addr_B << " allotted a structural address of " << str_addr_B << std::endl;
        std::cout << "-----S(B) : " << str_addr_B << std::endl;
#endif
        oci.update(phy_addr_B, str_addr_B);

        return str_addr_B;

    }

    vector<uint64_t> isb_predict(uint64_t trigger_phy_addr, unsigned int trigger_str_addr, uint64_t ip)
    {
#ifdef DEBUG
        std::cout << "*Trigger Str addr " << trigger_str_addr << std::endl;
#endif
        uint64_t candidate_phy_addr;	
        vector<uint64_t> candidates;
        candidates.clear();

#ifndef RESTRICT_REGION
        unsigned int lookahead = 1;
        unsigned int ideal = 0;
        for(unsigned int i=0; i<STREAM_MAX_LENGTH; i++)
        {
            if(ideal >= DEGREE)
                break;
            uint64_t str_addr_candidate = trigger_str_addr+lookahead+i ;
            if(str_addr_candidate % STREAM_MAX_LENGTH == 0)
            {
                stream_end++;
                break;
            }

            bool ret = oci.get_physical_address(candidate_phy_addr, str_addr_candidate);
            if(ret)
            {
                ideal++;
                candidates.push_back(candidate_phy_addr);
            }
            else
            {
                ideal++;
                prefetch_buffer_added++;
                prefetch_buffer.add(str_addr_candidate);
                no_translation++;
            }
        }
#else
        unsigned int num_prefetched = 0;
        for(unsigned int i=0; i<STREAM_MAX_LENGTH; i++)
        {
            uint64_t str_addr_candidate = ((trigger_str_addr>>STREAM_MAX_LENGTH_BITS) << STREAM_MAX_LENGTH_BITS)+i;

            if(str_addr_candidate == trigger_str_addr)
                continue;

            bool ret = oci.get_physical_address(candidate_phy_addr, str_addr_candidate);

            if(ret)
            {
                candidates.push_back(candidate_phy_addr);
                num_prefetched++;
            }   
            else {
                prefetch_buffer_added++;
                prefetch_buffer.add(str_addr_candidate);
                num_prefetched++;
            }
            if(num_prefetched >= DEGREE)
                break;
        }
#endif

        return candidates;
    }

    bool access_training_unit(uint64_t key, uint64_t& last_phy_addr, unsigned int& last_str_addr, uint64_t next_addr)
    {
        //TrainingUnitEntry* curr_training_entry = training_unit.find(key)->second;
        bool pair_found = true;
        //if(curr_training_entry == NULL)
        if(training_unit.find(key) == training_unit.end())
        {
            //std::cout << "Not found " << std::hex << key << std::endl;
            //TrainingUnitEntry* new_training_entry = training_unit.select(key);
            TrainingUnitEntry* new_training_entry = new TrainingUnitEntry;
            assert(new_training_entry);
            new_training_entry->reset();
            training_unit[key] = new_training_entry;
            pair_found = false;
        }

        assert(training_unit.find(key) != training_unit.end());
        TrainingUnitEntry* curr_training_entry = training_unit.find(key)->second;
        assert(curr_training_entry != NULL);
        last_str_addr = curr_training_entry->str_addr;
        last_phy_addr = curr_training_entry->addr;
        uint64_t last_addr = curr_training_entry->addr;
        if(last_addr == next_addr)
            return false;
#ifdef DEBUG
        // off_chip_corr_matrix.update_neighbor(last_addr, next_addr); 
#endif
        return pair_found;

    }

    void update_training_unit(uint64_t key, uint64_t addr, unsigned int str_addr)
    {
        //std::cout << "Updated " << std::hex << key << " to " << addr << std::dec << std::endl;
        assert(training_unit.find(key) != training_unit.end());
        TrainingUnitEntry* curr_training_entry = training_unit.find(key)->second;
        assert(curr_training_entry);
        curr_training_entry->addr = addr;
        curr_training_entry->str_addr = str_addr;
    }

    public :
    ISB_prefetcher_t()
    {
        //std::cout<<"ISB constructor constructed\n";
        training_unit.clear();
        alloc_counter = 0;
        last_page = 0;

        stream_divergence_count = 0;
        stream_divergence_new_stream = 0;
        candidate_tlb_miss = 0;
        candidate_diff = 0;
        prefetch_buffer.reset(); 
    }

    bool get_structural_address( uint64_t addr, unsigned int& str_addr)
    {
        return oci.get_structural_address(addr, str_addr);
    }


    unsigned int assign_structural_addr()
    {
        alloc_counter += STREAM_MAX_LENGTH;
#ifdef DEBUG
        std::cout << "  ALLOC " << alloc_counter << std::endl;
#endif
        return ((unsigned int) alloc_counter);	

    }
};

ISB_prefetcher_t isb;


void CACHE::prefetcher_initialize()
{
    printf("Ideal ISB Prefetching\n");
    // you can inspect these knob values from your code to see which configuration you're runnig in
    //printf("Knobs visible from prefetcher: %d %d %d\n", knob_scramble_loads, knob_small_llc, knob_low_bandwidth);
    total_access = 0;
    predictions = 0;
    prefetch_buffer_issue = 0;
    prefetch_buffer_added = 0;
    no_prediction = 0;
    stream_end = 0;
    no_translation = 0;
    
    reuse = 0;
    cout << "AMC size: " << AMC_SIZE << endl;
}

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in)
{
    if (type != LOAD)
        return 0;

    if(cache_hit)
        return 0;

    //ip = 0;


  // uncomment this line to see all the information available to make prefetch decisions
 //  if(!cache_hit)
 //    printf("%lld: Access (0x%llx 0x%llx %d %d %d) \n", get_current_cycle(0), addr, ip, cache_hit, get_l2_read_queue_occupancy(0), get_l2_mshr_occupancy(0));

    uint64_t addr_B = (addr >> 6) << 6;
    uint64_t key = ip;

    if(addr_B == last_address)
        return 0;
    last_address = addr_B;

    total_access++;

    #ifdef DEBUG
      std::cout << "**Trigger " << std::hex<< addr_B << " with key " << std::hex<< key << std::endl;
    #endif
      
    unsigned int str_addr_B = 0;
    bool str_addr_B_exists = isb.oci.get_structural_address(addr_B, str_addr_B); 

    if(str_addr_B_exists){
        vector<uint64_t> candidates = isb.isb_predict(addr_B, str_addr_B, ip);
        unsigned int num_prefetched = 0;
        for(unsigned int i=0; i<candidates.size(); i++)
        {
            int ret = prefetch_line(candidates[i], FILL_LLC, i, ip, 0, 0);
            if (ret == 1)
            {
                predictions++;
                num_prefetched++;
            }
            if(num_prefetched >= DEGREE)
                break;
        }
    }
    else
        no_prediction++;
                

    unsigned int str_addr_A;
    uint64_t addr_A;
    if(isb.access_training_unit(key, addr_A, str_addr_A, addr_B)){
#ifdef DEBUG
        std::cout << "Consider pair " << str_addr_A << " and " <<addr_B << " with key as " << key << std::endl;
#endif
        if(str_addr_A == 0){ //TBD, when is this condition true? When this is the 2nd access to the pc
            str_addr_A = isb.assign_structural_addr();
            isb.oci.update(addr_A, str_addr_A);
        }
        str_addr_B = isb.isb_train(str_addr_A, addr_B);
    }

    isb.update_training_unit(key, addr_B, str_addr_B);
    return 0;

}

uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
  // uncomment this line to see the information available to you when there is a cache fill event
//  printf("%lld: Fill 0x%llx %d %d %d 0x%llx %d\n", get_current_cycle(0), addr, set, way, prefetch, evicted_addr, get_l2_mshr_occupancy(0));
}

// Arguments do not include the least significant zero bits. For example, for a page size
// of 4KB, the arguments do not include the last 12 bits that are zero.
void CACHE::inform_tlb_eviction(uint64_t insert_page_addr, uint64_t evict_page_addr)
{
#ifdef TLB_SYNC

    //TODO: This function should be called when a TLB entry is evicte and be modified
    //      appopriately to model the traffic and latency overhead of meta-data accesses.
    //      It is also possible to model the effects of a limited meta-data cache without latency/traffic overheads.
    assert(0);

    //cout << "TLB eviction " << hex << insert_page_addr << " " << evict_page_addr << dec << endl;

    evict_page_addr = (evict_page_addr << LOG2_PAGE_SIZE);
    for(unsigned int i=0; i<PAGE_SIZE; i+=BLOCK_SIZE)
    {
        uint64_t evict_cl_addr = evict_page_addr + i;

        uint32_t str_addr = 0;
        if(isb.oci.get_structural_address(evict_cl_addr, str_addr))
        {
            //isb.oci.invalidate(evict_cl_addr, str_addr);
            isb.oci.mark_not_tlb_resident(evict_cl_addr);
            isb.oci.off_chip_mapping.update_physical(evict_cl_addr, str_addr);
            isb.oci.off_chip_mapping.update_structural(evict_cl_addr, str_addr);

            //cout << "Updating " << hex << str_addr << endl;
        }
    }

    uint64_t base_page_addr = (insert_page_addr << LOG2_PAGE_SIZE);
    for(unsigned int i=0; i<PAGE_SIZE; i+=BLOCK_SIZE)
    {
        uint64_t insert_cl_addr = base_page_addr + i;
        //cout << "Consider cache line " << hex << insert_cl_addr << dec << endl;

        uint32_t str_addr = 0;
        if(isb.oci.off_chip_mapping.get_structural_address(insert_cl_addr, str_addr))
        {
            isb.oci.update(insert_cl_addr, str_addr);
            isb.oci.mark_tlb_resident(insert_cl_addr);
            //cout << "        Updating " << hex << str_addr << endl;
        }
    }

    //cout << "Filter prefetch buffer " << endl;
    for(unsigned int i=0; i<BUFFER_SIZE; i++)
    {
        uint64_t phy_addr;
        if(isb.prefetch_buffer.valid[i] == false)
            continue;

        bool ret = isb.oci.get_physical_address(phy_addr, isb.prefetch_buffer.buffer[i]);
        if(ret){
            uint64_t candidate = phy_addr;
            prefetch_line(candidate, FILL_LLC, i, 0, 0, 0);
            isb.prefetch_buffer.issue(i);
            prefetch_buffer_issue++;
        //    cout << hex << "       " << isb.prefetch_buffer.buffer[i] << " " << phy_addr << dec << " --- " << ret << endl;
        }
    }
#endif
}

void CACHE::prefetcher_cycle_operate() {
}

void CACHE::prefetcher_final_stats()
{
  printf("Prefetcher final stats\n");
    cout << "Training Unit Size: " << isb.training_unit.size() << endl;
    //cout << "Addr table size: " << isb.oci.ps_map.size() << " " << isb.oci.sp_map.size() << endl;;
    cout << endl << endl;
    cout << "Triggers: " << total_access << endl;
    cout << "No Prediction: " << no_prediction << " " << 100*(double)no_prediction/(double)total_access << endl;
    cout << "Stream end: " << stream_end << endl;
    cout << "No translation: " << no_translation << " " << 100*(double)no_translation/(double)total_access << endl;
    cout << "Predictions: " << predictions << " " << 100*(double)predictions/(double)total_access << endl;
    cout << "Prefetch buffer added: " << prefetch_buffer_added << " " << 100*(double)prefetch_buffer_added/(double)total_access << endl;
    cout << "Prefetch buffer predictions: " << prefetch_buffer_issue << " " << 100*(double)prefetch_buffer_issue/(double)total_access << endl;
    cout << endl << endl;
    cout << "Stream divergence: " << isb.stream_divergence_count << " " << 100*(double)(isb.stream_divergence_count)/(total_access) << endl;
    cout << "Stream divergence -- new stream: " << isb.stream_divergence_new_stream << endl;
    cout << endl << endl;

    cout << "PS-AMC evictions: " << isb.oci.ps_amc_evictions << endl;
    cout << "SP-AMC evictions: " << isb.oci.sp_amc_evictions << endl;
}
