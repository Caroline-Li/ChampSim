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

#ifndef PREDICTOR_H
#define PREDICTOR_H

#define MAX_SHCT 31
#define SHCT_SIZE_BITS 14
#define SHCT_SIZE (1<<SHCT_SIZE_BITS)

using namespace std;

#include <iostream>

#include <math.h>
#include <set>
#include <vector>
#include <map>

uint64_t CRC( uint64_t _blockAddress );


class HAWKEYE_PC_PREDICTOR
{
    public:
    map<uint64_t, short unsigned int > SHCT;


    void increment (uint64_t pc);

    void decrement (uint64_t pc);

    bool get_prediction (uint64_t pc);
    /*
    bool CACHE:: prefetch_suppress(uint64_t pc)
    {
        return this.get_prediction();
    }
    */
};

class HAWKEYE_REUSE_PREDICTOR : public HAWKEYE_PC_PREDICTOR
{
    map<uint64_t, bool> negative_prediction_prefetch_pc_map;
    map<uint64_t, uint64_t> negative_prediction_addr_to_pc_map;


    public:

    void mark(uint64_t addr, uint64_t ip);

    bool get_mark(uint64_t addr);

    void set_reuse(uint64_t addr);

    bool get_reuse(uint64_t addr);

    void clear_entry(uint64_t addr);

    void print_SHCT(void);

    void print_SHCT_ip(uint64_t addr);

    bool get_prediction(uint64_t addr);

    void increment (uint64_t addr);

    void decrement (uint64_t addr);

};

#endif
