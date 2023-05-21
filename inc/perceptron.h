#ifndef PERCEPTRON_H
#define PERCEPTRON_H
#include <stdint.h>
#include <array>
#include <iostream>
#include <set>

template <typename T, std::size_t NUM_FEATURES, std::size_t BITS>
class Perceptron {
    int bias = 0;
    double learning_rate = 1.0;
    std::array<double, NUM_FEATURES> weights;
    
    public:

    Perceptron() {
        for (int i = 0; i < NUM_FEATURES; i++) {
            weights[i] = 1.0;
        }
    }
    // maximum and minimum weight values
    constexpr static double max_weight = (1 << (BITS - 1)) - 1;
    constexpr static double min_weight = -(max_weight + 1);

    double predict(std::array<T, NUM_FEATURES> features) {
        double output = 0;
        for (int i = 0; i < NUM_FEATURES; i++) {
            output += features[i] * weights[i];
        }

        return output;
    }

    void update(bool result, std::array<T, NUM_FEATURES> features) {
        for (int i = 0; i < NUM_FEATURES; i++) {
            if (result) {
                weights[i] = std::min(weights[i] + learning_rate*features[i], max_weight);
            }
            else {
                weights[i] = std::max(weights[i] - learning_rate*features[i], min_weight);
            }
        }
    }

    void print_weights(void) {
        std::cout << "weights: ";
        for (int i = 0; i < NUM_FEATURES; i++) {
            std::cout << weights[i] << ", ";
        }
        std::cout << std::endl;
    }
};

template<std::size_t BITS, bool ENABLE_BIAS, int NUM_ENTRIES>
class Feature_Perceptron {
    int bias = 0;

    std::map<uint64_t, int> feature_weights;
    std::map<uint64_t, int> feature_freq;

    public:
    uint64_t positive_count = 0;
    uint64_t total_count = 0;
    // maximum and minimum weight values
    constexpr static int max_weight = (1 << (BITS - 1)) - 1;
    constexpr static int min_weight = -(max_weight + 1);

    Feature_Perceptron() {}

    int predict(std::set<uint64_t>& features) {
        int output = bias;
        for (auto iter = features.begin(); iter != features.end(); iter++) {
            int index = *iter % NUM_ENTRIES;
            if (feature_weights.find(index) == feature_weights.end()) {
                feature_weights[index] = 0;
            }
            output += feature_weights[index];
        }

        return output;
    }

    void update(uint64_t ip, int coeff, std::set<uint64_t>& features) {
        if (ENABLE_BIAS) {
            bias += coeff;
        }
        for (auto iter = features.begin(); iter != features.end(); iter++) {
            uint64_t feature = *iter % NUM_ENTRIES;
            feature_weights[feature] += coeff;
            feature_freq[feature]++;
            if (feature_weights[feature] > max_weight) {
                feature_weights[feature] = max_weight;
            }
            if (feature_weights[feature] < min_weight) {
                feature_weights[feature] = min_weight;
            }
        }
    }

    void print_weights(void) {
        std::cout << "weights: ";
        for (auto item : feature_weights) {
            std::cout << "feature: " << item.first << ", ";
            std::cout << "weight: " << item.second << ", ";
            std::cout << "frequency: " << feature_freq[item.first] << std::endl;
        }
        std::cout << std::endl;
    }


};

template<std::size_t BITS>
class Counter_Predictor {

    std::map<uint64_t, int> counter_table;

    public:
    uint64_t positive_count = 0;
    uint64_t total_count = 0;
    // maximum and minimum weight values
    constexpr static int max_weight = (1 << (BITS - 1)) - 1;
    constexpr static int min_weight = -(max_weight + 1);

    Counter_Predictor() {}

    int predict(uint64_t ip) {
        if (counter_table.find(ip) != counter_table.end()) {
            if (counter_table[ip] > 0) {
                return true;
            }
            return false;
        }

        return true;
    }

    void update(uint64_t ip, bool result) {
        if (result) {
            counter_table[ip] = std::min(counter_table[ip] + 1, max_weight);
        }
        else {
            counter_table[ip] = std::max(counter_table[ip] - 1, min_weight);
        }
    }

    void print_weights(void) {
        for (auto& item : counter_table) {
            std::cout << "ip: " << item.first;
            std::cout << " counter: " << item.second << std::endl;
        }
    }


};
#endif