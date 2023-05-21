#ifndef DECISION_TREE_H
#define DECISION_TREE_H
#include <stdint.h>
#include <array>
#include <iostream>
#include <set>
#include <map>

using namespace std;

#define DECISION_TREE_FILTERING false

class Decision_Tree_Predictor {
    int positive_count;
    int total_count;

    public:

    Decision_Tree_Predictor() {}

    bool predict(uint64_t ip, int degree, int prefetch_offset, uint64_t hashed_history, int dram_bank_occupancy);

    void update_result(bool result);

    void print_accuracy() {
        cout << "accuracy: " << (double) positive_count / (double) total_count << endl;
    }

};

#endif
