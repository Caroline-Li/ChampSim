#ifndef PREFETCH_REDUCTION_PREDICTOR_H
#define PREFETCH_REDUCTION_PREDICTOR_H
#include <map>
#include <queue>
#include <iostream>
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

    void print_predictor(void) {
        for (auto& item : predictor) {
            cout << "signature: " << item.first;
            for (auto& elem : item.second) {
                cout << " queue: ";
                int queue_size = elem.first.size();
                queue<uint8_t> q = elem.first;
                for (int i = 0; i < queue_size; i++) {
                    uint8_t front = q.front();
                    cout << front;
                    q.push(front);
                    q.pop();
                }
                cout << " prediction: " << elem.second << endl;
            }
        }
    }
};
#endif