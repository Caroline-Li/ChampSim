#!/bin/bash
/u/cli23/thesis_research/ChampSim/bin/perceptron-no-bo_percore-hawkeye_final_dyn-1core -warmup_instructions 0000000 -simulation_instructions 200000000 -hide_heartbeat -traces /scratch/cluster/akanksha/CRCRealTraces/gobmk_135B.trace.gz &> /u/cli23/thesis_research/ChampSim/output/experiment_tracer_hawkeye_final_dyn_bo_percore_prefetch_traffic_reduction_interval_hit_list/gobmk_135B.txt
