#!/bin/bash

# Check that args are correct
if [ "$#" -ne 3 ]; then
    echo "Incorrect # of arguments: expected prefetcher name and experiment name"
    echo "For a file prefetcher/<prefetcher_name>.l2c_pref, usage is \"./run.sh <prefetcher_name> <experiment_name>\""
    exit
fi

# Check that we're on the right host
HOST="$(hostname)"
#if [ "${HOST}" != "darmok" ]; then
#    echo "You should be logged into darmok.cs.utexas.edu when you run this script!"
#    echo "First, run \"ssh <utcsid>@darmok.cs.utexas.edu\", then run the script."
#    exit
#fi

# Simulation parameters
GPU=false
TRACE_PATH="/scratch/cluster/akanksha/CRCRealTraces"
# TRACE_PATH="/u/cli23/thesis_research/ChampSim/test_traces"
CHAMP_PATH="$(pwd)"

BRANCH="perceptron"
L1P="no"
PREFETCH=${2}
LLCP="no"
CACHE=${1}
CORES=1

WARM_INS=0
SIM_INS=200
OUTPUT_DIR="/scratch/cluster/cli23/experiment_current_output/${3}"
BINARY="${BRANCH}-no-${PREFETCH}-${CACHE}-${CORES}core"
echo $BINARY
echo "${CHAMP_PATH}/bin/${BINARY} -warmup_instructions ${WARM_INS}000000 -simulation_instructions ${SIM_INS}000000 -hide_heartbeat --config=${CHAMP_PATH}/default_dyn_degree.ini -traces ${TRACE_PATH}/astar_163B.trace.gz &> astar_163B.txt"
${CHAMP_PATH}/bin/${BINARY} -warmup_instructions ${WARM_INS}000000 -simulation_instructions ${SIM_INS}000000 -hide_heartbeat -traces ${TRACE_PATH}/astar_163B.trace.gz &> astar_163B.txt
