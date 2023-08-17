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

WARM_INS=50
SIM_INS=200
OUTPUT_DIR="/scratch/cluster/cli23/experiment_current_output/${3}"
BINARY="${BRANCH}-no-${PREFETCH}-${CACHE}-${CORES}core"
echo $BINARY

# Ensure output dir exists
if test ! -d ${OUTPUT_DIR}; then
    mkdir ${OUTPUT_DIR}
fi

# Ensure binary exists
if test ! -f bin/${BINARY} ; then
    echo "No binary found for prefetcher \"${PREFETCH}\""
    echo "Build the simulator with your prefetcher before running it"
    exit
fi

# Run our cache replacement policy on each trace
while read TRACE; do
    SCRIPT_FILE="${OUTPUT_DIR}/${TRACE}.sh"
    CONDOR_FILE="${OUTPUT_DIR}/${TRACE}.condor"
    
    # create script file
    echo "#!/bin/bash" > ${SCRIPT_FILE}
    echo "${CHAMP_PATH}/bin/${BINARY} --config=${CHAMP_PATH}/default_dyn_degree.ini -traces ${TRACE_PATH}/${TRACE}.trace.gz &> ${OUTPUT_DIR}/${TRACE}.txt" >> ${SCRIPT_FILE}
    chmod +x ${SCRIPT_FILE}

    # create condor file
    /u/cli23/thesis_research/ChampSim/condorize.sh ${GPU} ${OUTPUT_DIR} ${TRACE}
    
    # submit the condor file
    /lusr/opt/condor/bin/condor_submit ${CONDOR_FILE}
done < sim_list/traces_21.txt

