#include "hawkeye_predictor.h"

uint64_t CRC( uint64_t _blockAddress )
{
    static const unsigned long long crcPolynomial = 3988292384ULL;
    unsigned long long _returnVal = _blockAddress;
    for( unsigned int i = 0; i < 32; i++ )
        _returnVal = ( ( _returnVal & 1 ) == 1 ) ? ( ( _returnVal >> 1 ) ^ crcPolynomial ) : ( _returnVal >> 1 );
    return _returnVal;
}

void HAWKEYE_PC_PREDICTOR::increment(uint64_t pc) 
{
    uint64_t signature = CRC(pc) % SHCT_SIZE;
    if(SHCT.find(signature) == SHCT.end())
        SHCT[signature] = (1+MAX_SHCT)/2;

    SHCT[signature] = (SHCT[signature] < MAX_SHCT) ? (SHCT[signature]+1) : MAX_SHCT;

}
void HAWKEYE_PC_PREDICTOR::decrement(uint64_t pc) 
{
        uint64_t signature = CRC(pc) % SHCT_SIZE;
        if(SHCT.find(signature) == SHCT.end())
            SHCT[signature] = (1+MAX_SHCT)/2;

        if(SHCT[signature] != 0)
        {
            SHCT[signature] = SHCT[signature]-1;
        }
}

bool HAWKEYE_PC_PREDICTOR::get_prediction(uint64_t pc) 
{
    uint64_t signature = CRC(pc) % SHCT_SIZE;
    if(SHCT.find(signature) != SHCT.end() && SHCT[signature] < ((MAX_SHCT+1)/2)) {
        //printf("PC: %d Counter value: %d\n", pc, SHCT[signature]);
        return false;
    }
//        else if (SHCT.find(signature) != SHCT.end())
//        {
//            //printf("PC: %d Counter value: %d\n", pc, SHCT[signature]);
//        }

    return true;
}

void HAWKEYE_REUSE_PREDICTOR::mark(uint64_t cache_line_address, uint64_t ip) {
    negative_prediction_prefetch_pc_map[ip] = false;
    negative_prediction_addr_to_pc_map[cache_line_address] = ip;
}

bool HAWKEYE_REUSE_PREDICTOR::get_mark(uint64_t addr) {
    if (negative_prediction_addr_to_pc_map.find(addr) != negative_prediction_addr_to_pc_map.end()) {
        uint64_t ip = negative_prediction_addr_to_pc_map[addr];
        if (negative_prediction_prefetch_pc_map.find(ip) != negative_prediction_prefetch_pc_map.end()) {
            return true;
        }
    }
    return false;
}

void HAWKEYE_REUSE_PREDICTOR::set_reuse(uint64_t addr) {
    uint64_t ip = negative_prediction_addr_to_pc_map[addr];
    negative_prediction_prefetch_pc_map[ip] = true;
}

bool HAWKEYE_REUSE_PREDICTOR::get_reuse(uint64_t addr) {
    uint64_t ip = negative_prediction_addr_to_pc_map[addr];
    return negative_prediction_prefetch_pc_map[ip];
}

void HAWKEYE_REUSE_PREDICTOR::clear_entry(uint64_t addr) {
    uint64_t ip = negative_prediction_addr_to_pc_map[addr];
    negative_prediction_prefetch_pc_map.erase(ip);
}

void HAWKEYE_REUSE_PREDICTOR::print_SHCT(void) {
    cout << "reuse predictor SHCT" << endl;
    for (auto& item : SHCT) {
        cout << "ip: " << item.first << " ";
        cout << "reuse prediction: " << item.second << endl;
    }
}

void HAWKEYE_REUSE_PREDICTOR::print_SHCT_ip(uint64_t addr) {
    uint64_t ip = negative_prediction_addr_to_pc_map[addr];
    uint64_t signature = CRC(ip) % SHCT_SIZE;
    cout << "SHCT counter value: " << SHCT[signature] << endl;
    
}

bool HAWKEYE_REUSE_PREDICTOR::get_prediction(uint64_t addr) {
    uint64_t ip = negative_prediction_addr_to_pc_map[addr];
    uint64_t signature = CRC(ip) % SHCT_SIZE;
    if(SHCT.find(signature) != SHCT.end() && SHCT[signature] == 0) {
        //printf("PC: %d Counter value: %d\n", pc, SHCT[signature]);
        return false;
    }

    return true;
}

void HAWKEYE_REUSE_PREDICTOR::increment(uint64_t addr) {
    uint64_t ip = negative_prediction_addr_to_pc_map[addr];
    uint64_t signature = CRC(ip) % SHCT_SIZE;
    if(SHCT.find(signature) == SHCT.end())
        SHCT[signature] = (1+MAX_SHCT)/2;

    SHCT[signature] = (SHCT[signature] < MAX_SHCT) ? (SHCT[signature]+1) : MAX_SHCT;
}

void HAWKEYE_REUSE_PREDICTOR::decrement(uint64_t addr) {
    uint64_t ip = negative_prediction_addr_to_pc_map[addr];
    uint64_t signature = CRC(ip) % SHCT_SIZE;
    if(SHCT.find(signature) == SHCT.end())
        SHCT[signature] = (1+MAX_SHCT)/2;

    if(SHCT[signature] != 0)
    {
        SHCT[signature] = SHCT[signature]-1;
    }
}