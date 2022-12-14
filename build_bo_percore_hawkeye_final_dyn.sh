#!/bin/bash

# Build
mkdir -p bin
rm -f bin/champsim
./config.sh bo_percore_hawkeye_final_dyn.json
make

# Sanity check
echo ""
if [ ! -f bin/champsim ]; then
    echo "${BOLD}ChampSim build FAILED!${NORMAL}"
    echo ""
    exit
fi

echo "${BOLD}ChampSim is successfully built"
echo "Branch Predictor: Perceptron"
echo "L1D Prefetcher: no"
echo "L2C Prefetcher: bo_percore"
echo "LLC Replacement: hawkeye_final_dyn"
echo "Cores: 1"
BINARY_NAME="perceptron-no-bo_percore-hawkeye_final_dyn-1core"
echo "Binary: bin/${BINARY_NAME}${NORMAL}"
echo ""
mv bin/champsim bin/${BINARY_NAME}

