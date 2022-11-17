#ifndef PREFETCH_REDUCTION_PREDICTOR_H
#define PREFETCH_REDUCTION_PREDICTOR_H
#include <map>
#include <queue>
#include <stdint.h>

using namespace std;

class Prefetch_Reduction_Predictor {
    map<uint64_t, map<queue<uint8_t>, uint8_t>> predictor;
    public:

    Prefetch_Reduction_Predictor() {}

    void add_training(uint64_t signature, queue<uint8_t> pattern, uint8_t k) {
        predictor[signature][pattern] = k;
    }

    uint8_t get_prediction(uint64_t signature, queue<uint8_t> pattern) {
        if (predictor[signature].find(pattern) != predictor[signature].end()) {
            return predictor[signature][pattern];
        }
        return 0;
    }
};
#endif