#ifndef FEATURE_KNOWLEDGE
#define FEATURE_KNOWLEDGE

#include <assert.h>
#include <map>
#include <string>

#include "pythia_helper.h"
#define FK_MAX_TILINGS 32

typedef enum {
  F_PC = 0,                         // 0
  F_Offset,                         // 1
  F_Delta,                          // 2
  F_Address,                        // 3
  F_PC_Offset,                      // 4
  F_PC_Address,                     // 5
  F_PC_Page,                        // 6
  F_PC_Path,                        // 7
  F_Delta_Path,                     // 8
  F_Offset_Path,                    // 9
  F_PC_Delta,                       // 10
  F_PC_Offset_Delta,                // 11
  F_Page,                           // 12
  F_PC_Path_Offset,                 // 13
  F_PC_Path_Offset_Path,            // 14
  F_PC_Path_Delta,                  // 15
  F_PC_Path_Delta_Path,             // 16
  F_PC_Path_Offset_Path_Delta_Path, // 17
  F_Offset_Path_PC,                 // 18
  F_Delta_Path_PC,                  // 19
  F_Action_Bias,                    // 20 (for per-action featureless biases)

  NumFeatureTypes
} FeatureType;

typedef enum {
  W_Global = 0,   // 0
  W_Action,       // 1
  W_State,        // 2
  W_State_Action, // 3

  NumWeightTypes
} WeightType;

class FeatureKnowledge
{
private:
  FeatureType m_feature_type;
  float m_alpha, m_gamma;
  uint32_t m_actions;
  float m_init_weight;
  float m_init_value;
  float m_weight_gradient;
  float m_weight_minimum;
  uint32_t m_hash_type;

  uint32_t m_num_tilings, m_num_tiles;
  float*** m_qtable;
  bool m_enable_tiling_offset;

  WeightType m_weight_type;
  map<uint32_t, float> weights;
  map<uint32_t, float> min_weights;
  map<uint32_t, float> max_weights;

  /* q-value tracing related variables */
  uint32_t trace_interval;
  uint64_t trace_timestamp;
  uint32_t trace_record_count;
  FILE* trace;

private:
  float getQ(uint32_t tiling, uint32_t tile_index, uint32_t action);
  void setQ(uint32_t tiling, uint32_t tile_index, uint32_t action, float value);
  uint32_t get_tile_index(uint32_t tiling, State* state);
  string get_feature_string(State* state);

  /* feature index generators */
  uint32_t process_PC(uint32_t tiling, uint64_t pc);
  uint32_t process_offset(uint32_t tiling, uint32_t offset);
  uint32_t process_delta(uint32_t tiling, int32_t delta);
  uint32_t process_address(uint32_t tiling, uint64_t address);
  uint32_t process_PC_offset(uint32_t tiling, uint64_t pc, uint32_t offset);
  uint32_t process_PC_address(uint32_t tiling, uint64_t pc, uint64_t address);
  uint32_t process_PC_page(uint32_t tiling, uint64_t pc, uint64_t page);
  uint32_t process_PC_path(uint32_t tiling, uint32_t pc_path);
  uint32_t process_delta_path(uint32_t tiling, uint32_t delta_path);
  uint32_t process_offset_path(uint32_t tiling, uint32_t offset_path);
  uint32_t process_PC_delta(uint32_t tiling, uint64_t pc, int32_t delta);
  uint32_t process_PC_offset_delta(uint32_t tiling, uint64_t pc, uint32_t offset, int32_t delta);
  uint32_t process_Page(uint32_t tiling, uint64_t page);
  uint32_t process_PC_Path_Offset(uint32_t tiling, uint32_t pc_path, uint32_t offset);
  uint32_t process_PC_Path_Offset_Path(uint32_t tiling, uint32_t pc_path, uint32_t offset_path);
  uint32_t process_PC_Path_Delta(uint32_t tiling, uint32_t pc_path, int32_t delta);
  uint32_t process_PC_Path_Delta_Path(uint32_t tiling, uint32_t pc_path, uint32_t delta_path);
  uint32_t process_PC_Path_Offset_Path_Delta_Path(uint32_t tiling, uint32_t pc_path, uint32_t offset_path, uint32_t delta_path);
  uint32_t process_Offset_Path_PC(uint32_t tiling, uint32_t offset_path, uint64_t pc);
  uint32_t process_Delta_Path_PC(uint32_t tiling, uint32_t delta_path, uint64_t pc);
  uint32_t process_Action_Bias();

  /* for weight updates */
  inline void initialize_weight(uint32_t index)
  {
    weights[index] = m_init_weight;
    min_weights[index] = m_init_weight;
    max_weights[index] = m_init_weight;
  }

  inline uint32_t state_weight_index(State* state)
  {
    // TODO: Support multi-tiling
    assert(m_num_tilings == 1); // Currently only supports flat tables
    return get_tile_index(0, state);
  }

  inline uint32_t state_action_weight_index(State* state, uint32_t action)
  {
    // TODO: Support multi-tiling
    assert(m_num_tilings == 1); // Currently only supports flat tables
    uint32_t tile_index = get_tile_index(0, state);
    // 16 bits for state, 16 bits for action.
    // Supports up to 65,536 states and 65,536 actions
    return (tile_index << 16) + action;
  }

  /* for q-value tracing */
  void dump_feature_trace(State* state);
  void plot_scores();

public:
  FeatureKnowledge(FeatureType feature_type, float alpha, float gamma, uint32_t actions, float weight, float weight_gradient, float weight_minimum,
                   uint32_t num_tilings, uint32_t num_tiles, bool zero_init, uint32_t hash_type, int32_t enable_tiling_offset, WeightType weight_type);
  ~FeatureKnowledge();
  float retrieveQ(State* state, uint32_t action_index);
  void updateQ(State* state1, uint32_t action1, int32_t reward, State* state2, uint32_t action2);
  static string getFeatureString(FeatureType type);
  static string getWeightTypeString(WeightType type);
  uint32_t getMaxAction(State* state); /* Called by featurewise engine only to get a consensus from all the features */

  /* weight manipulation */
  inline uint32_t get_weight_index(WeightType wt, State* state, uint32_t action)
  {
    switch (wt) {
    case W_Global:
      return 0;
    case W_Action:
      return action;
    case W_State:
      return state_weight_index(state);
    case W_State_Action:
      return state_action_weight_index(state, action);
    default:
      assert(0);
    }
  }

  inline void increase_weight(uint32_t index)
  {
    if (weights.find(index) == weights.end()) // Add starting weight values.
      initialize_weight(index);
    weights[index] = std::max(weights[index] + m_weight_gradient * weights[index], m_weight_minimum);
  }

  inline void decrease_weight(uint32_t index)
  {
    if (weights.find(index) == weights.end()) // Add starting weight values
      initialize_weight(index);
    weights[index] = std::max(weights[index] - m_weight_gradient * weights[index], m_weight_minimum);
  }

  inline void set_weight(uint32_t index, float weight)
  {
    if (weights.find(index) == weights.end()) // Add starting weight values
      initialize_weight(index);
    weights[index] = std::max(weight, m_weight_minimum);
  }

  inline void normalize_weight(uint32_t index, float sum_across_features)
  {
    assert(weights.find(index) != weights.end());
    weights[index] /= sum_across_features;
  }

  inline void track_weight_statistics(uint32_t index)
  {
    if (weights[index] < min_weights[index]) // Min weight
      min_weights[index] = weights[index];
    if (weights[index] > max_weights[index]) // Max weight
      max_weights[index] = weights[index];
  }

  inline float get_weight(uint32_t index)
  {
    if (weights.find(index) == weights.end()) // Add starting weight values.
      initialize_weight(index);
    return weights[index];
  }

  inline float get_min_weight(uint32_t index)
  {
    if (weights.find(index) == weights.end()) // Add starting weight values
      initialize_weight(index);
    return min_weights[index];
  }

  inline float get_max_weight(uint32_t index)
  {
    if (weights.find(index) == weights.end()) // Add starting weight values
      initialize_weight(index);
    return max_weights[index];
  }
};

#endif /* FEATURE_KNOWLEDGE */